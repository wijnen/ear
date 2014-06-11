import gtk
import media
import os

class Markers (gtk.TreeView):
	def __init__ (self, gui):
		self.data = gui.data
		self.target = None
		self.endtarget = None
		self.store = gtk.TreeStore (object, str, str, str, gtk.gdk.Pixbuf, str, gtk.gdk.Pixbuf)
		gtk.TreeView.__init__ (self, self.store)
		self.markers = [self.render_icon (gtk.STOCK_MEDIA_FORWARD, gtk.ICON_SIZE_MENU, None), self.render_icon (gtk.STOCK_MEDIA_PLAY, gtk.ICON_SIZE_MENU, None)]
		self.stoppers = [self.render_icon (gtk.STOCK_GOTO_TOP, gtk.ICON_SIZE_MENU, None), self.render_icon (gtk.STOCK_GO_DOWN, gtk.ICON_SIZE_MENU, None), self.render_icon (gtk.STOCK_GOTO_BOTTOM, gtk.ICON_SIZE_MENU, None)]
		gui.register_attribute ('load', None, self.load)
		gui.register_attribute ('update', None, self.update)
		gui.register_attribute ('current', self.get_current, self.show_marker)
		gui.register_attribute ('target', None, self.target_marker)
		gui.register_attribute ('endtarget', None, self.endtarget_marker)
		self.iconcol, renderers = self.mkcol ('', (4, 6), True)
		self.namecol, self.namerenderer = self.mkcol ('Name', 1)
		self.namerenderer.connect ('edited', self.rename)
		self.set_expander_column (self.namecol)
		self.mkcol ('Start', 2)
		self.mkcol ('End', 5)
		self.mkcol ('Duration', 3)
		self.connect ('row-activated', self.activate)
		self.connect ('row-collapsed', lambda tv, iter, path: self.recompute_marks ())
		self.connect ('row-expanded', lambda tv, iter, path: self.recompute_marks ())
	def mkcol (self, title, col, icons = False):
		tvcol = gtk.TreeViewColumn (title)
		self.append_column (tvcol)
		if icons:
			renderer = (gtk.CellRendererPixbuf (), gtk.CellRendererPixbuf ())
			tvcol.pack_start (renderer[0])
			tvcol.pack_start (renderer[1])
			tvcol.add_attribute (renderer[0], 'pixbuf', col[0])
			tvcol.add_attribute (renderer[1], 'pixbuf', col[1])
		else:
			renderer = gtk.CellRendererText ()
			tvcol.pack_start (renderer)
			tvcol.add_attribute (renderer, 'text', col)
		return tvcol, renderer
	def clear_targets (self, node):
		if self.store.get_value (node, 4):
			self.store.set_value (node, 4, None)
		if not self.row_expanded (self.store.get_path (node)):
			return
		c = self.store.iter_children (node)
		while c:
			self.clear_targets (c)
			c = self.store.iter_next (c)
	def recompute_target_impl (self, node):
		obj = self.store.get_value (node, 0)
		if not self.row_expanded (self.store.get_path (node)):
			if obj is None or obj.start <= self.target <= obj.end:
				if self.store.get_value (node, 4) != self.markers[1]:
					self.store.set_value (node, 4, self.markers[1])
			else:
				if self.store.get_value (node, 4) != self.markers[0]:
					self.store.set_value (node, 4, self.markers[0])
			return
		c = self.store.iter_children (node)
		if c is None or self.store.get_value (c, 0).start > self.target:
			if self.store.get_value (node, 4) != self.markers[1]:
				self.store.set_value (node, 4, self.markers[1])
			while c:
				self.clear_targets (c)
				c = self.store.iter_next (c)
			return
		if self.store.get_value (node, 4):
			self.store.set_value (node, 4, None)
		last = None
		while c:
			if self.store.get_value (c, 0).start > self.target:
				break
			if last:
				self.clear_targets (last)
			last = c
			c = self.store.iter_next (c)
		self.recompute_target_impl (last)
		while c:
			self.clear_targets (c)
			c = self.store.iter_next (c)
	def recompute_target (self):
		# Recompute the "you are here" target.
		self.recompute_target_impl (self.store.get_iter_first ())
	def clear_endtargets (self, node):
		if self.store.get_value (node, 6):
			self.store.set_value (node, 6, None)
		if not self.row_expanded (self.store.get_path (node)):
			return
		c = self.store.iter_children (node)
		while c:
			self.clear_endtargets (c)
			c = self.store.iter_next (c)
	def recompute_endtarget_impl (self, node):
		obj = self.store.get_value (node, 0)
		c = self.store.iter_children (node)
		if obj and obj.start == self.endtarget:
			# In case of "until here", the marker should be at the parent, so set it and clear all children.
			if self.store.get_value (node, 6) != self.stoppers[0]:
				self.store.set_value (node, 6, self.stoppers[0])
			while c:
				self.clear_endtargets (c)
				c = self.store.iter_next (c)
			return
		if not c or not self.row_expanded (self.store.get_path (node)):
			# If the row is not expanded, the marker must be at the parent.
			if self.endtarget == self.data['track'].duration or (obj and obj.end == self.endtarget):
				if self.store.get_value (node, 6) != self.stoppers[2]:
					self.store.set_value (node, 6, self.stoppers[2])
			else:
				if self.store.get_value (node, 6) != self.stoppers[1]:
					self.store.set_value (node, 6, self.stoppers[1])
			return
		# The marker is expanded, but the first child may be later than the endtarget.
		if self.endtarget and self.store.get_value (c, 0).start > self.endtarget:
			if self.store.get_value (node, 6) != self.stoppers[1]:
				self.store.set_value (node, 6, self.stoppers[1])
			while c:
				self.clear_endtargets (c)
				c = self.store.iter_next (c)
			return
		# The marker is expanded and the endtarget should be on one of the child nodes.
		if self.store.get_value (node, 6):
			self.store.set_value (node, 6, None)
		last = None
		while c:
			if self.endtarget and self.store.get_value (c, 0).start > self.endtarget:
				break
			if last:
				self.clear_endtargets (last)
			last = c
			c = self.store.iter_next (c)
		if last:
			self.recompute_endtarget_impl (last)
		while c:
			self.clear_endtargets (c)
			c = self.store.iter_next (c)
	def recompute_endtarget (self):
		# Recompute the "play until here" target.
		self.recompute_endtarget_impl (self.store.get_iter_first ())
	def recompute_marks_impl (self, node, m):
		obj = self.store.get_value (node, 0)
		if obj is not None:
			m.append ((obj.start, 'bottom', ''))
		if not self.row_expanded (self.store.get_path (node)):
			return
		c = self.store.iter_children (node)
		while c:
			self.recompute_marks_impl (c, m)
			c = self.store.iter_next (c)
	def recompute_marks (self):
		m = []
		self.recompute_marks_impl (self.store.get_iter_first (), m)
		self.data['scalemarks'] (m)
		self.recompute_target ()
		self.recompute_endtarget ()
	def target_marker (self, pos):
		self.target = pos
		self.recompute_target ()
	def endtarget_marker (self, pos):
		self.endtarget = pos
		self.recompute_endtarget ()
	def rename (self, renderer, path, text):
		iter = self.store.get_iter (path)
		self.store.set_value (iter, 1, text)
		obj = self.store.get_value (iter, 0)
		if obj is None:
			self.data['track'].name = text
			self.data['rename_track'] (self.data['track'])
		else:
			obj.name = text
	def load (self, value):
		self.store.clear ()
		self.store.append (None, (None, self.data['track'].name, media.mkmomentstr (0), media.mkintervalstr (self.data['track'].duration), None, media.mkmomentstr (self.data['track'].duration), None))
		self.update ()
		self.collapse_all ()
		self.expand_row (self.store.get_path (self.store.get_iter_first ()), False)
		#self._dump_tree ('tree')
		self.recompute_target ()
	def update (self, arg = None):
		self.namerenderer.set_property ('editable', self.data['editable'] ())
		# Current is the iterator in the tree which points to the last node which may still have more children.
		# Next is the iterator which points to the next node, should current be removed.
		current = self.store.get_iter_first ()
		self.store.set_value (current, 1, self.data['track'].name)
		idx = 0
		while True:
			if idx < len (self.data['track'].markers):
				listobj = self.data['track'].markers[idx]
			else:
				listobj = None
			# Find next.
			next = self.store.iter_children (current)
			if not next:
				next = self.store.iter_next (current)
				if not next:
					n = current
					while n:
						n = self.store.iter_parent (n)
						if not n:
							next = None
							break
						next = self.store.iter_next (n)
						if next:
							break
			# Check what to do with next.
			if listobj:
				new = (listobj, listobj.name, media.mkmomentstr (listobj.start), media.mkintervalstr (listobj.end - listobj.start), None, media.mkmomentstr (listobj.end), None)
			if next:
				nextobj = self.store.get_value (next, 0)
				if current:
					currentobj = self.store.get_value (current, 0)
				# Keep it only if it is the same as in the list, and at the same level as it should be.
				if nextobj is listobj and (currentobj is None or listobj.end <= currentobj.end):
					parent = self.store.iter_parent (next)
					if parent and self.store.get_value (parent, 0) == currentobj:
						nextobj.iter = next
						self.store.set (next, 1, new[1], 2, new[2], 3, new[3])
						idx += 1
						current = next
						continue
				if listobj is None or (nextobj.start, nextobj.end) < (listobj.start, listobj.end):
					# Remove next from tree.
					self.store.remove (next)
					continue
			if not listobj:
				break
			# New object must be inserted.
			while True:
				currentobj = self.store.get_value (current, 0)
				if currentobj is None or listobj.end <= currentobj.end:
					if next:
						parent = self.store.iter_parent (next)
					if next and parent and self.store.get_value (parent, 0) == currentobj:
						current = self.store.insert_before (current, next, new)
					else:
						current = self.store.append (current, new)
					self.expand_to_path (self.store.get_path (current))
					listobj.iter = current
					idx += 1
					break
				current = self.store.iter_parent (current)
		self.recompute_marks ()
	def get_current (self):
		selection = self.get_selection ()
		selected = selection.get_selected ()
		if selected:
			return self.store.get_value (selected[1], 0)
		return None
	def show_marker (self, (marker, edit)):
		path = self.store.get_path (marker.iter)
		self.expand_to_path (path)
		if edit:
			self.set_cursor (path, self.namecol, True)
	def activate (self, tv, path, col):
		obj = self.store.get_value (self.store.get_iter (path), 0)
		self.data['media'].play (obj.start - self.data['before'] () if obj else 0)
		self.data['playing'] (True)
	def _dump_tree (self, title = 'Tree dump', iter = None, indent = ''):
		if iter is not None:
			print (indent + self.store.get_value (iter, 1))
			indent += '  '
		else:
			print ('=== ' + title)
		c = self.store.iter_children (iter)
		while c:
			self._dump_tree (title, c, indent)
			c = self.store.iter_next (c)
