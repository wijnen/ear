import gtk
import media

class Fileselector(gtk.TreeView):
	def __init__(self, gui):
		self.data = gui.data
		self.store = gtk.ListStore(object, str, str)
		gtk.TreeView.__init__(self, self.store)
		self.namecol = self.mkcol('Name', 1)
		self.timecol = self.mkcol('Duration', 2)
		self.update_list()
		self.connect('row-activated', self.activate)
		self.load_event = gui.register_event('load')
		gui.register_attribute('rename_track', None, self.rename_track)
		gui.register_attribute('update', None, self.update_list)
	def mkcol(self, title, col):
		ret = gtk.TreeViewColumn(title)
		renderer = gtk.CellRendererText()
		self.append_column(ret)
		ret.pack_start(renderer)
		ret.add_attribute(renderer, 'text', col)
		return ret
	def rename_track(self, track):
		self.update_list()
		self.store.set_value(track.iter, 1, track.name)
	def update_list(self, dummy = None):
		current = self.store.get_iter_first()
		for track in self.data['db']:
			if self.data['tag'] != '' and self.data['tag'] not in track.tags:
				# Current should not be in the list.
				if track.iter is not None:
					# But it is: remove it.
					n = self.store.iter_next(current)
					self.store.remove(current)
					track.iter = None
					current = n
				continue
			if track.iter is not None and track == self.store.get_value(current, 0):
				# Current is already in the list.
				track.iter = current
				current = self.store.iter_next(current)
				continue
			assert track.iter is None
			# Add current to the list.
			track.iter = self.store.insert_before(current, (track, track.name, media.mkintervalstr(track.duration)))
	def activate(self, tv, path, col):
		self.data['track'] = self.store.get_value(self.store.get_iter(path), 0)
		self.load_event()
