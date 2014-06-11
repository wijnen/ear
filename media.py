import datetime
import gst
import gobject
gobject.threads_init()
import glib
import os

def parsemoment(m):
	return int(float(m) * 1000)

def unparsemoment(m):
	return '%.3f' % (m / 1000.)

def parseinterval(i):
	return int(float(m) * 1000)

def mktimestr(t):
	t = int(t / 100)
	frac = t % 10
	s = t / 10
	m = s / 60
	s -= m * 60
	h = m / 60
	m -= h * 60
	ret = ''
	if h > 0:
		ret += '%d:' % h
	ret += '%02d:' % m
	ret += '%02d.%d' % (s, frac)
	return ret

def mkmomentstr(t):
	return '[' + mktimestr(t) + ']'

def mkintervalstr(t):
	return '(' + mktimestr(t) + ')'

class Media:
	def __init__(self, status, set_range, set_endtarget):
		self.pipeline = None
		self.set_pos = status
		self.set_range = set_range
		self.set_endtarget = set_endtarget
		self.endtarget = None
		self.speed = 1.
		self.offset = 0.
		self.media_duration = 0.
		self.updater = None
		self.cb = None
		self.send_pixbuf = None
		self.pipeline = gst.Pipeline('pipeline')
		self.player = gst.element_factory_make('playbin2')
		self.pipeline.add(self.player)
		self.player.set_property('flags', 0x03)
		self.sink = gst.element_factory_make('gdkpixbufsink')
		self.bus = self.pipeline.get_bus()
		self.bus.add_watch(self.new_pixbuf)
		self.player.set_property('video-sink', self.sink)
		self.audiobin = gst.Bin()
		self.convert = gst.element_factory_make('audioconvert')
		self.pitch = gst.element_factory_make('pitch', 'pitch')
		self.audiosink = gst.element_factory_make('autoaudiosink')
		self.audiobin.add(self.convert)
		self.audiobin.add(self.pitch)
		self.audiobin.add(self.audiosink)
		self.convert.link(self.pitch)
		self.pitch.link(self.audiosink)
		pad = gst.GhostPad('sink', self.convert.get_pad('sink'))
		pad.set_active(True)
		self.audiobin.add_pad(pad)
		self.player.set_property('audio-sink', self.audiobin)
	@classmethod
	def get_duration(cls, filename):
		cls.prober.set_property('uri', 'file://' + os.path.realpath(filename))
		cls.probe_pipeline.set_state(gst.STATE_PAUSED)
		cls.probe_pipeline.get_state()
		try:
			ret = cls.probe_pipeline.query_duration(gst.Format(gst.FORMAT_TIME))[0] / gst.MSECOND
		except:
			#import traceback
			#traceback.print_exc()
			print('Not using file "%s"' % os.path.realpath(filename))
			ret = None
		cls.probe_pipeline.set_state(gst.STATE_NULL)
		return ret
	def new_pixbuf(self, bus, message):
		if message.type != gst.MESSAGE_ELEMENT:
			return True
		if self.send_pixbuf:
			self.send_pixbuf(self.sink.get_property('last-pixbuf'))
		return True
	def load(self, track, index):
		assert index is None or index == -1 or len(track.files) > index
		# Discard old cb.
		self.cb = None
		track.media = index
		self.track = track
		if self.pipeline is not None:
			self.pause(True)
			self.pipeline.set_state(gst.STATE_NULL)
		if index is None or index < 0:
			filename, self.offset, self.media_duration = None, None, None
			self.set_range(0, 1)
			return
		filename, self.offset, self.media_duration = track.files[index]
		self.offset *= 1000.
		filename = os.path.join(os.path.dirname(track.dbname), filename)
		self.player.set_property('uri', 'file://' + os.path.realpath(filename))
		self.pipeline.set_state(gst.STATE_PAUSED)
		self.set_range(-self.media_duration, track.duration)
	def set_speed(self, speed):
		pos = self.get_pos()
		self.speed = speed
		self.play(pos, play = None)
	def play(self, start = None, end = None, cb = (), play = True): # For cb, None means no cb, so use something else for "no change".
		self.offset = self.track.files[self.track.media][1] * 1000.
		if cb != ():
			self.cb = cb
		if end is None:
			end = self.endtarget
		if end is not None and end < start:
			end = None
		self.endtarget = end
		if play is None:
			play = self.playing()
		if end is not None and end < self.offset + self.media_duration:
			if self.set_endtarget:
				self.set_endtarget(end)
			end -= self.offset
		elif self.set_endtarget:
			self.set_endtarget(end)
		if start is not None and start < self.offset:
			start = self.offset
		self.pitch.set_property('tempo', self.speed)
		if start is not None and start < self.offset + self.media_duration:
			start -= self.offset
			self.pause()
			self.pipeline.get_state()
			if end > start:
				self.pipeline.seek(1.0, gst.Format(gst.FORMAT_TIME), gst.SEEK_FLAG_ACCURATE | gst.SEEK_FLAG_FLUSH, gst.SEEK_TYPE_SET, start * gst.MSECOND, gst.SEEK_TYPE_SET, end * gst.MSECOND)
			else:
				self.pipeline.seek(1.0, gst.Format(gst.FORMAT_TIME), gst.SEEK_FLAG_ACCURATE | gst.SEEK_FLAG_FLUSH, gst.SEEK_TYPE_SET, start * gst.MSECOND, gst.SEEK_TYPE_END, 0)
		if play:
			self.pause(False)
	def seek(self, delta):
		self.play(self.get_pos() + delta, play = None)
	def pause(self, pausing = True):
		if pausing:
			self.pipeline.set_state(gst.STATE_PAUSED)
			if self.updater:
				glib.source_remove(self.updater)
				self.updater = None
		else:
			self.pipeline.set_state(gst.STATE_PLAYING)
			self.updater = glib.timeout_add(100, self.update)
	def update(self):
		self.pipeline.get_state()
		try:
			pos = self.pipeline.query_position(gst.Format(gst.FORMAT_TIME))[0] / gst.MSECOND * self.speed + self.offset
		except:
			self.updater = None
			if self.cb:
				cb = self.cb
				self.cb = None
				cb()
			return False
		self.set_pos(pos) # if self.speed <= 1 else pos / self.speed)
		return True
	def playing(self):
		return self.pipeline.get_state()[1] == gst.STATE_PLAYING
	def get_pos(self):
		self.pipeline.get_state()
		try:
			pos = self.pipeline.query_position(gst.Format(gst.FORMAT_TIME))[0] / gst.MSECOND * self.speed + self.offset
		except:
			return 0
		return pos #if self.speed <= 1 else pos / self.speed

Media.prober = gst.element_factory_make('playbin2')
Media.probe_pipeline = gst.Pipeline('probepipeline')
Media.probe_pipeline.add(Media.prober)
Media.probesink = gst.element_factory_make('gdkpixbufsink')
Media.prober.set_property('video-sink', Media.probesink)
