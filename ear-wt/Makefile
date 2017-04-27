all:  earUI

%:%.C
	g++ $< -std=c++11 -o $@ -l wt -l wthttp  -l zmq -Wall -Wextra -g 

clean:
	rm -f earUI
