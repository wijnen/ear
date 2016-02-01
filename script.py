import glib
import lua
import re

current_track = None
play = None
set_status = None
data = None

class Marker (object):
	def __init__ (self, m):
		self.__dict__['_m'] = m
	def __getattr__ (self, attr):
		if attr == 'name':
			return self._m.name
		if attr == 'start':
			return self._m.start
		if attr == 'end':
			return self._m.end
		raise AttributeError ('attribute %s not found' % attr)
	def __setattr__ (self, attr, value):
		raise AttributeError ('setting attributes on markers is not allowed')

class Media (object):
	def __init__ (self, m):
		self.__dict__['_m'] = m
	def __getattr__ (self, attr):
		if attr == 'filename':
			return Opaque (self._m[0])
		if attr == 'offset':
			return self._m[1]
		if attr == 'duration':
			return self._m[2]
		raise AttributeError ('attribute %s not found' % attr)
	def __setattr__ (self, attr, value):
		raise AttributeError ('setting attributes on media is not allowed')

class Opaque (object):
	def __init__ (self, text):
		self._text = text
	def __add__ (self, text):
		if isinstance (text, Opaque):
			return Opaque (self._text + text._text)
		return Opaque (self._text + text)
	def __radd__ (self, text):
		return Opaque (text + self._text)
	def __iadd__ (self, text):
		if isinstance (text, Opaque):
			self._text += text._text
		else:
			self._text += text
		return self
	def __repr__ (self):
		return '[opaque string]'
	def __setattr__ (self, attr, value):
		raise AttributeError ('setting attributes on opaque strings is not allowed')

class Track (object):
	def __init__ (self, data):
		self.__dict__['_track'] = data['track']
	def __getattr__ (self, attr):
		if attr == 'name':
			return self._track.name
		elif attr == 'duration':
			return self._track.duration
		elif attr == 'media':
			return [Media (m) for m in self._track.files]
		else:
			raise AttributeError ('attribute %s not found' % attr)
	def __setattr__ (self, attr, value):
		raise AttributeError ('setting attributes on tracks is not allowed')

def marker (regex):
	result = markers (regex)
	if len (result) != 1:
		print('not returning %s' % repr (result))
		return None
	return result[0]

def markers (regex):
	r = re.compile (regex)
	ret = []
	for m in current_track.markers:
		if r.match (m.name):
			ret.append (Marker (m))
	return ret

def schedule (ms, cb):
	glib.timeout_add (ms, self._cb, cb, current_track)

def _cb (cb, track):
	# Don't allow callbacks if the track has changed.
	if current_track is data['track']:
		cb ()

def do_set_status (text):
	if isinstance (text, Opaque):
		set_status (text._text)
	else:
		set_status (text)

def run (script, new_data, new_play, new_set_status, new_get_speed, new_set_speed):
	global data, current_track, play, set_status
	l = lua.lua ()
	current_track = new_data['track']
	data = new_data
	play = new_play
	set_status = new_set_status
	get_speed = new_get_speed
	set_speed = new_set_speed
	l.run (var = 'track', value = Track (data), name = 'setup track')
	# The lambda function for play is required because of lua weirdness.
	l.run (var = 'play', value = lambda *a, **ka: play (*a, **ka), name = 'setup play')
	l.run (var = 'marker', value = marker, name = 'setup marker')
	l.run (var = 'markers', value = markers, name = 'setup markers')
	l.run (var = 'schedule', value = schedule, name = 'setup schedule')
	l.run (var = 'set_status', value = do_set_status, name = 'setup set_status')
	l.run (var = 'get_speed', value = get_speed, name = 'setup get_speed')
	l.run (var = 'set_speed', value = set_speed, name = 'setup set_speed')
	l.run (script, name = 'user script')
