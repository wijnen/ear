LIBRARIES = -l wt -l zmq -l wthttp 
OBJECTS=earzmq.o filteredStringModel.o TimeWidget.o trackSearchContainer.o fragmentTree.o 
all:  	earUI earMobileUI
earMobileUI:  earMobileUI.o fragmentTree.o earzmq.o TimeWidget.o
	g++ -std=c++14 -Wall -Wextra -ggdb3 -O3 $^ $(LIBRARIES) -o $@ 
%.o: %.C
	g++ -std=c++14 -c -Wall -Wextra -ggdb3 -O3 $(LIBRARIES) $< -o $@
earUI: $(OBJECTS) earUI.o
	g++ -std=c++14 -Wall -Wextra -ggdb3 -O3 $^ $(LIBRARIES) -o $@ 
clean:
	rm -f earUI
	rm -f *.o
