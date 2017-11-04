
#ifndef earzmq_included
#define earzmq_included

 
#include <zmq.hpp>
#include <Wt/Json/Array>
#include <Wt/Json/Parser>
#include <Wt/Json/Object>
#include <Wt/Json/Serializer>
#include <iostream>
#define MAXSIZE 1048576 //Maximum size of ZMQ read buffer used here. 

/*

ZeroMQ handling below.
An interact function can take a socket or not. If provided with a socket, it will be used. If not, one will be opened and closed for this interaction. 
Send and recieve always need a socket.
When sending more than one interaction, preferably create a socket and use that.
WStrings, strings are both handled
*/

namespace zmq_conn
{
  zmq::socket_t* connect();
  void disconnect(  zmq::socket_t *socket = 0);
  Wt::Json::Object interact(const char value[], zmq::socket_t *socket = 0    );
  Wt::Json::Object interact(std::string value, zmq::socket_t *socket = 0    );
  Wt::Json::Object interact(Wt::Json::Object value, zmq::socket_t *socket = 0);
  Wt::Json::Object interact(Wt::WString value, zmq::socket_t *socket = 0);
  Wt::Json::Object interact(std::string value, bool raw, zmq::socket_t *socket = 0);
  std::string recv(zmq::socket_t *socket);
  void send(std::string value, zmq::socket_t *socket = 0, bool raw = false );

};

#endif //earzmq_included
