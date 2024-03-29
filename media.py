import datetime
import logging
import gi
import traceback
gi.require_version('Gst', '1.0')
from gi.repository import Gst
from gi.repository import GObject
GObject.threads_init()
from gi.repository import GLib
import os
import time

import urllib.request as url #Otherwise all files with # in the name have 0 duration
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
        self.gstvolume = Gst.ElementFactory.make("volume")
        self.gstvolume.set_property('volume',1.0) 
        self.audiobin.add(self.convert)
        self.audiobin.add(self.pitch)
        self.audiobin.add(self.convert2)
        self.audiobin.add(self.gstvolume)
        self.audiobin.add(self.audiosink)
        self.convert.link(self.pitch)
        self.pitch.link(self.convert2)
        self.convert2.link(self.gstvolume)
        self.gstvolume.link(self.audiosink)
        
        pad = Gst.GhostPad.new('sink', self.convert.get_static_pad('sink'))
        pad.set_active(True)
        self.audiobin.add_pad(pad)
        self.player.set_property('audio-sink', self.audiobin)
        self.timeout = None
        self.countdown_end = None
        self.waveform = [(None,None,None)]
    def volume(self,volume = None):
        logging.debug("Setting volume {}".format(volume))
        if volume != None:
            self.gstvolume.set_property("volume",volume)
        return self.gstvolume.get_property("volume")
    def make_waveform(self,filename): #TODO make optional for performance
        pipeline = Gst.parse_launch("   filesrc location=\""+filename+"\" ! decodebin     ! audioconvert ! audioresample ! level ! fakesink ")
        if pipeline == None:
            return
        bus = pipeline.get_bus()
        bus.add_signal_watch()
        output = []
        done = False
        def callback(bus,msg):
            if msg.type == Gst.MessageType.ELEMENT:
                if  "GstLevel" not in str(type(msg.src)):
                    return #TODO: Optimize code below to not append and be faster
                rms = msg.get_structure().get_value('rms')
                output.append( (msg.get_structure().get_value('timestamp')/Gst.MSECOND/100 , abs(rms[0]),abs(rms[1])))
            elif msg.type == Gst.MessageType.EOS:
                self.waveform = output #TODO set some signal that this can be sent to Wt
                pipeline.set_state(Gst.State.NULL)
        bus.connect('message', callback)
        pipeline.set_state(Gst.State.PLAYING)
    @classmethod
    def get_duration(cls, filename): #Class method so we can check a files duration without instantiating everything
        cls.prober.set_property('uri', 'file://' + url.pathname2url(os.path.realpath(filename)))
        cls.probe_pipeline.set_state(Gst.State.PAUSED)
        cls.probe_pipeline.get_state(Gst.CLOCK_TIME_NONE)
        if any([bugged in filename for bugged in ["mp3","wav","m4a"]]):
            time.sleep (0.125) #https://www.ruby-forum.com/topic/1232772
        try:
            ret = cls.probe_pipeline.query_duration(Gst.Format(Gst.Format.TIME))[1] / Gst.MSECOND
        except:
            #import traceback
            #traceback.print_exc()
            logging.warning('Not using file "%s"' % os.path.realpath(filename))
            ret = None
        cls.probe_pipeline.set_state(Gst.State.NULL)
        return ret
    def new_pixbuf(self, bus, message, arg): #TODO: Rename to signal_handler
        #print(message.type)
        if message.type == Gst.MessageType.EOS:#This will be called at the end of a stream, but also at the end of a played fragment
            self.done()
            return True
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
        self.waveform = [(None,None,None)]
        self.track = track
        if self.pipeline is not None:
            self.pause(True)
            self.pipeline.set_state(Gst.State.NULL)
        if index is None or index < 0:
            filename, self.offset, self.media_duration = None, None, None
            self.set_range(0, 1)
            return
        filename, self.offset = track['files'][index]
        filename = os.path.join(track['root'],filename)
        self.media_duration = self.get_duration(filename)
        track['duration'] = self.media_duration
        self.offset *= 1000.
        filename = os.path.join(os.path.dirname(track['root']), filename)
        self.player.set_property('uri', 'file://' + url.pathname2url(os.path.realpath(filename)))
        self.pipeline.set_state(Gst.State.PAUSED)
        self.set_range(-self.media_duration, track['duration'])
        self.make_waveform(filename)
    def set_speed(self, speed):
        pos = self.get_pos()
        self.speed = speed
        self.play(pos, play = None)
    def play(self, start = None, end = None, cb = (), play = True, timed = False): # For cb, None means no cb, so use something else for "no change".
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
            wait = -1 *(start - self.offset)
            loop = GLib.MainLoop()
            start = self.offset
            if self.timeout != None: 
                GLib.source_remove(self.timeout) #Only do one timeout at a time to prevent weird re-starting issues
                self.timeout = None 
            if self.playing(): 
                self.pause(True) 
            self.timeout = GLib.timeout_add(wait, lambda _: self.play(start = start, end=end, cb=cb,play=play, timed = True), None)
            self.countdown_end = time.time() + wait/1000.
            return True
        logging.debug("start {}, offset {}, duration {}, play {}".format(start, self.offset, self.media_duration, play))
        if start is not None and start < self.offset + self.media_duration:
            logging.debug("Actually playing because start is not none")
            start -= self.offset
            start /= self.speed
            if end is not None:
                end /= self.speed
            self.pause()
            self.pipeline.get_state(Gst.CLOCK_TIME_NONE)
            if end is not None and (start is None or end > start):
                ret = self.pipeline.seek(1.0, Gst.Format(Gst.Format.TIME), Gst.SeekFlags.ACCURATE | Gst.SeekFlags.FLUSH, Gst.SeekType.SET, start * Gst.MSECOND, Gst.SeekType.SET, end * Gst.MSECOND)
                if not ret:
                    ret = self.pipeline.seek_simple(Gst.Format.TIME, Gst.SeekFlags.KEY_UNIT | Gst.SeekFlags.FLUSH, start * Gst.MSECOND)
                    logging.warning("Simple seeking")
                    #self.playbin.seek_simple(Gst.Format.TIME,  Gst.SeekFlags.FLUSH | Gst.SeekFlags.KEY_UNIT, seek_time_secs * Gst.SECOND) 
            else:
                ret = self.pipeline.seek(1.0, Gst.Format(Gst.Format.TIME), Gst.SeekFlags.ACCURATE | Gst.SeekFlags.FLUSH, Gst.SeekType.SET, start * Gst.MSECOND, Gst.SeekType.END, 0)
                if not ret:
                    ret = self.pipeline.seek_simple(Gst.Format.TIME, Gst.SeekFlags.KEY_UNIT|Gst.SeekFlags.ACCURATE | Gst.SeekFlags.FLUSH, start * Gst.MSECOND)
                    logging.warning("Simple seeking")
            if not ret:
                logging.warning("Seeking not allowed by gstreamer")
        if play:
            self.pause(False)
        if timed:
            self.timeout = None
            self.countdown_end = None
            return False #Bit weird, but makes this function possible to be used as a oneshot timer.
    def done(self):
        self.updater = None
        if self.cb:
                cb = self.cb
                self.cb = None
                cb()
        else:
            logging.debug("Ending without a callback") #That means our current playing is probably done
            if (self.get_pos()+1000 < self.media_duration):
                logging.debug("But wait, there's more track than that we've played!")
                self.play(start=self.get_pos(),end=self.media_duration,play=False) #So pause it, because if we let it run we play more than we wanted.
            else:
                logging.debug("Stopping because end of track reached, and resetting position to start of track.")
                self.play(start=0,play=False)
            
            #Do not change the position but make sure the playing state is off, and allow us to continue where we left off
        return False

    def seek(self, delta):
        self.play(self.get_pos() + delta, play = None)
    def pause(self, pausing = True):
        logging.debug("Pause in media, {}".format(pausing))
        #logging.debug("\n".join(traceback.format_stack()))
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
        do_callback = False
        try:
            pos = self.pipeline.query_position(Gst.Format(Gst.Format.TIME))[1] / Gst.MSECOND * self.speed + self.offset
        except:
            self.end()
        self.set_pos(pos) # if self.speed <= 1 else pos / self.speed)
        return True
    def playing(self):
        return self.pipeline.get_state(Gst.CLOCK_TIME_NONE)[1] == Gst.State.PLAYING
    def get_pos(self):
        if self.timeout != None:
            if not self.playing():
                return -1000*(self.countdown_end - time.time())
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
 
