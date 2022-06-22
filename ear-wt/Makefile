LIBRARIES = -l wt -l zmq -l wthttp 

SHARED_OBJ = earzmq.o TimeWidget.o fragmentTree.o
UI_OBJECTS = $(SHARED_OBJ) filteredStringModel.o trackSearchContainer.o earUI.o
MOBILE_OBJECTS = $(SHARED_OBJ) earMobileUI.o
PLAYLIST_OBJECTS = $(SHARED_OBJ) earPlaylistUI.o

CFLAGS = -std=c++14 -Wall -Wextra -ggdb3 -rdynamic -O3

TARGETS = earUI earMobileUI earPlayListUI resources

all: $(TARGETS)

earPlaylistUI: $(PLAYLIST_OBJECTS)
	g++ -std=c++14 -Wall -Wextra -ggdb3 -O3 $^ $(LIBRARIES) -o $@


resources:
	test -L resources || ln -s /usr/local/share/Wt/resources .

earMobileUI: $(MOBILE_OBJECTS)
	g++ ${CFLAGS} $^ $(LIBRARIES) -o $@

%.o: %.C Makefile earUI.h earzmq.h filteredStringModel.h fragmentTree.h TimeWidget.h trackSearchContainer.h
	g++ -c ${CFLAGS} $(LIBRARIES) $< -o $@

earUI: $(UI_OBJECTS)
	g++ ${CFLAGS} $^ $(LIBRARIES) -o $@

clean:
	rm -f $(TARGETS)
	rm -f $(UI_OBJECTS) $(MOBILE_OBJECTS)
