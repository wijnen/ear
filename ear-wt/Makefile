all: 
	g++ earUI.C -std=c++11 -o earUI -l wt -l wthttp  -l zmq -Wall 
%:%.C
	g++ $< -std=c++11 -o $@ -l wt -l wthttp  -l zmq -Wall -Wextra -g 

clean:
	rm -f earUI
