#include "earzmq.h"

namespace zmq_conn{
   char const *zmq_port = 0;
   zmq::context_t context (1); //Important that this stays alive.
}
zmq::socket_t* zmq_conn::connect()
{

   if (zmq_port == 0)
   {
	zmq_port = getenv("ZMQ_PORT");
   } 
   zmq::socket_t *socket = new zmq::socket_t (context, ZMQ_REQ);
  
   socket->connect (zmq_conn::zmq_port);
   return socket;
}


void zmq_conn::disconnect(zmq::socket_t *socket)
{
	std::cout << "Not disconnecting " << zmq_conn::zmq_port << std::endl; //Currently we get a segfault on the second disconnect. If somehow keeping the connection does not have any downsides, it might be good to leave things this way. Check once we have two interface instances.
	//socket->disconnect(zmq_port);
	 //delete	socket;
}


Wt::Json::Object zmq_conn::interact(const char value[], zmq::socket_t *socket)
{
	return  zmq_conn::interact(std::string(value), false, socket);
}


Wt::Json::Object zmq_conn::interact(Wt::WString value, zmq::socket_t *socket)
{
	return  zmq_conn::interact(value.narrow(),false, socket);
}


Wt::Json::Object zmq_conn::interact(Wt::Json::Object value, zmq::socket_t *socket)
{
	return  zmq_conn::interact(Wt::Json::serialize(value), true, socket);
}


Wt::Json::Object zmq_conn::interact(std::string value, zmq::socket_t *socket)
{
	return  zmq_conn::interact(value, false, socket);
}


Wt::Json::Object zmq_conn::interact(std::string value, bool raw, zmq::socket_t *socket)
{
    bool disconn = false;
    if(socket == 0)
    {
	socket = connect();
    	disconn = true;
    }
    send(value,socket, raw);
    Wt::Json::Object retval;
    Wt::Json::parse(recv(socket),retval);
    if(disconn) 
    {
	disconnect(socket);
    }
    return retval;

}
void zmq_conn::send(std::string value, zmq::socket_t *socket, bool raw) 
{
    bool disconn = false;
    if(socket == 0)
    { 
	socket = connect();
	    disconn = true;
    }
std::string data;
    if (!raw)
    {
	 Wt::Json::Value msg_str;
	 msg_str = Wt::WString( value);
	 Wt::Json::Array msg_arr;
	 msg_arr.push_back(msg_str);
	 data = Wt::Json::serialize(msg_arr);
    }
    else
    {
	data = value;
    }
    socket->send(data.c_str(),data.size());
    if(disconn) 
    {
	disconnect(socket);
    }
    return;
}



std::string zmq_conn::recv(zmq::socket_t *socket) 
{
    char buffer[MAXSIZE];
    int nbytes = socket->recv(buffer, MAXSIZE);
    return std::string(buffer, nbytes);
}




