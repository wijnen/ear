LIBRARIES = -l wt -l zmq -l wthttp 

SHARED_OBJ = earzmq.o TimeWidget.o fragmentTree.o
UI_OBJECTS = $(SHARED_OBJ) filteredStringModel.o trackSearchContainer.o earUI.o
MOBILE_OBJECTS = $(SHARED_OBJ) earMobileUI.o

TARGETS = earUI earMobileUI resources

all: $(TARGETS)

resources:
	ln -s /usr/share/Wt/resources .

earMobileUI: $(MOBILE_OBJECTS)
	g++ -std=c++14 -Wall -Wextra -ggdb3 -O3 $^ $(LIBRARIES) -o $@ 

%.o: %.C
	g++ -std=c++14 -c -Wall -Wextra -ggdb3 -O3 $(LIBRARIES) $< -o $@

earUI: $(UI_OBJECTS)
	g++ -std=c++14 -Wall -Wextra -ggdb3 -O3 $^ $(LIBRARIES) -o $@ 

clean:
	rm -f $(TARGETS)
	rm -f $(UI_OBJECTS) $(MOBILE_OBJECTS)
