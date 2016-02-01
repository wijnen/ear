all: ear.gui
	./ear

%.gui: %.gui.in
	xmlgen < $< > $@

clean:
	rm -f *.pyc *.pyo ear.gui
