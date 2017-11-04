LIBRARIES = -l wt -l zmq -l wthttp 
OBJECTS=earUI.o earzmq.o
all:  	earUI earMobileUI 
%.o: %.C
	g++ -std=c++14 -c -Wall -Wextra -ggdb3 -O3 $(LIBRARIES) $< -o $@
earUI: $(OBJECTS)
	g++ -std=c++14 -Wall -Wextra -ggdb3 -O3 $^ $(LIBRARIES) -o $@ 
clean:
	rm -f earUI
	rm -f *.o
