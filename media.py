import datetime
import gi
gi.require_version('Gst', '1.0')
from gi.repository import Gst
from gi.repository import GObject
GObject.threads_init()
from gi.repository import GLib
import os

Gst.init(())

def parsemoment(m):
    return int(float(m) * 1000)

def unparsemoment(m):
    return '%.3f' % (m / 1000.)

def parseinterval(i):
    return int(float(m) * 1000)

def mktimestr(t):
    t = int(t // 100)
    frac = t % 10
    s = t // 10
    m = s // 60
    s -= m * 60
    h = m // 60
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
        self.pipeline = Gst.Pipeline()
        self.player = Gst.ElementFactory.make('playbin')
        self.pipeline.add(self.player)
        self.player.set_property('flags', 0x03)
        self.sink = Gst.ElementFactory.make('gdkpixbufsink')
        self.bus = self.pipeline.get_bus()
        self.bus.add_watch(0, self.new_pixbuf, None)
        self.player.set_property('video-sink', self.sink)
        self.audiobin = Gst.Bin()
        self.convert = Gst.ElementFactory.make('audioconvert')
        self.pitch = Gst.ElementFactory.make('pitch', 'pitch')
        self.convert2 = Gst.ElementFactory.make('audioconvert')
        self.audiosink = Gst.ElementFactory.make('autoaudiosink')
        self.audiobin.add(self.convert)
        self.audiobin.add(self.pitch)
        self.audiobin.add(self.convert2)
        self.audiobin.add(self.audiosink)
        self.convert.link(self.pitch)
        self.pitch.link(self.convert2)
        self.convert2.link(self.audiosink)
        pad = Gst.GhostPad.new('sink', self.convert.get_static_pad('sink'))
        pad.set_active(True)
        self.audiobin.add_pad(pad)
        self.player.set_property('audio-sink', self.audiobin)
    @classmethod
    def get_duration(cls, filename): #Class method so we can check a files duration without instantiating everything
        cls.prober.set_property('uri', 'file://' + os.path.realpath(filename))
        cls.probe_pipeline.set_state(Gst.State.PAUSED)
        cls.probe_pipeline.get_state(Gst.CLOCK_TIME_NONE)
        try:
            ret = cls.probe_pipeline.query_duration(Gst.Format(Gst.Format.TIME))[1] / Gst.MSECOND
        except:
            #import traceback
            #traceback.print_exc()
            print('Not using file "%s"' % os.path.realpath(filename))
            ret = None
        cls.probe_pipeline.set_state(Gst.State.NULL)
        return ret
    def new_pixbuf(self, bus, message, arg):
        if message.type != Gst.MessageType.ELEMENT:
            return True
        if self.send_pixbuf:
            self.send_pixbuf(self.sink.get_property('last-pixbuf'))
        return True
    def load(self, track, index):
        assert index is None or index == -1 or len(track['files']) > index
        # Discard old cb.
        self.cb = None
        track['media'] = index
        self.track = track
        if self.pipeline is not None:
            self.pause(True)
            self.pipeline.set_state(Gst.State.NULL)
        if index is None or index < 0:
            filename, self.offset, self.media_duration = None, None, None
            self.set_range(0, 1)
            return
        print(track['files'])
        filename, self.offset = track['files'][index]
        filename = os.path.join(track['root'],filename)
        self.media_duration = self.get_duration(filename)
        track['duration'] = self.media_duration
        self.offset *= 1000.
        filename = os.path.join(os.path.dirname(track['root']), filename)
        self.player.set_property('uri', 'file://' + os.path.realpath(filename))
        self.pipeline.set_state(Gst.State.PAUSED)
        self.set_range(-self.media_duration, track['duration'])
    def set_speed(self, speed):
        pos = self.get_pos()
        self.speed = speed
        self.play(pos, play = None)
    def play(self, start = None, end = None, cb = (), play = True): # For cb, None means no cb, so use something else for "no change".
        print("Lets play in media",self)
        self.pitch.set_property('tempo', self.speed)
        self.offset = self.track['files'][self.track['media']][1] * 1000.
        if cb != ():
            self.cb = cb
        if end is None:
            end = self.endtarget
        if end is not None and start is not None and end < start:
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
            wait = start - self.offset
            start = self.offset
        print("Playing from {} to {}".format(start,end))
        print("Offset {}, media_duration {}".format(self.offset, self.media_duration))
        if start is not None and start < self.offset + self.media_duration:
            start -= self.offset
            start /= self.speed
            if end is not None:
                end /= self.speed
            self.pause()
            self.pipeline.get_state(Gst.CLOCK_TIME_NONE)
            if end is not None and (start is None or end > start):
                self.pipeline.seek(1.0, Gst.Format(Gst.Format.TIME), Gst.SeekFlags.ACCURATE | Gst.SeekFlags.FLUSH, Gst.SeekType.SET, start * Gst.MSECOND, Gst.SeekType.SET, end * Gst.MSECOND)
            else:
                self.pipeline.seek(1.0, Gst.Format(Gst.Format.TIME), Gst.SeekFlags.ACCURATE | Gst.SeekFlags.FLUSH, Gst.SeekType.SET, start * Gst.MSECOND, Gst.SeekType.END, 0)
        else:
            print("Not playinf, because of reasons")
        if play:
            self.pause(False)
    def seek(self, delta):
        self.play(self.get_pos() + delta, play = None)
    def pause(self, pausing = True):
        if pausing:
            self.pipeline.set_state(Gst.State.PAUSED)
            if self.updater:
                GLib.source_remove(self.updater)
                self.updater = None
        else:
            self.pipeline.set_state(Gst.State.PLAYING)
            self.updater = GLib.timeout_add(100, self.update)
    def update(self):
        self.pipeline.get_state(Gst.CLOCK_TIME_NONE)
        try:
            pos = self.pipeline.query_position(Gst.Format(Gst.Format.TIME))[1] / Gst.MSECOND * self.speed + self.offset
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
        return self.pipeline.get_state(Gst.CLOCK_TIME_NONE)[1] == Gst.State.PLAYING
    def get_pos(self):
        self.pipeline.get_state(Gst.CLOCK_TIME_NONE)
        try:
            pos = self.pipeline.query_position(Gst.Format(Gst.Format.TIME))[1] / Gst.MSECOND * self.speed + self.offset
        except:
            return 0
        return pos #if self.speed <= 1 else pos / self.speed

Media.prober = Gst.ElementFactory.make('playbin')
Media.probe_pipeline = Gst.Pipeline()
Media.probe_pipeline.add(Media.prober)
Media.probesink = Gst.ElementFactory.make('gdkpixbufsink')
Media.prober.set_property('video-sink', Media.probesink)

# vim: set expandtab tabstop=4 shiftwidth=4 :
 
