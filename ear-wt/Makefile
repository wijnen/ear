LIBRARIES = -l wt -l zmq -l wthttp 
HEADERS = earUI_base.h earUI.h earzmq.h filteredStringModel.h fragmentTree.h TimeWidget.h trackSearchContainer.h

SHARED_OBJ = earzmq.o TimeWidget.o fragmentTree.o
UI_OBJECTS = $(SHARED_OBJ) filteredStringModel.o trackSearchContainer.o earUI.o
MOBILE_OBJECTS = $(SHARED_OBJ) earMobileUI.o
PLAYLIST_OBJECTS = $(SHARED_OBJ) earPlaylistUI.o trackSearchContainer.o filteredStringModel.o

CFLAGS = -std=c++14 -Wall -Wextra -ggdb3 -rdynamic -O3

TARGETS = earUI earMobileUI earPlaylistUI resources

all: $(TARGETS)

resources:
	test -L resources || ln -s /usr/share/Wt/resources .

%.o: %.C Makefile ${HEADERS}
	g++ -c ${CFLAGS} $< -o $@

earUI: $(UI_OBJECTS)
	g++ ${CFLAGS} $^ $(LIBRARIES) -o $@

earMobileUI: $(MOBILE_OBJECTS)
	g++ ${CFLAGS} $^ $(LIBRARIES) -o $@

earPlaylistUI: $(PLAYLIST_OBJECTS)
	g++ ${CFLAGS} $^ $(LIBRARIES) -o $@

clean:
	rm -f resources
	rm -f $(TARGETS)
	rm -f $(UI_OBJECTS) $(MOBILE_OBJECTS) $(PLAYLIST_OBJECTS)
