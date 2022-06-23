// Copyright 2022 Bas Wijnen <wijnen@debian.org>

#include "earzmq.h"

namespace zmq_conn {
	char const *zmq_port = 0;
	zmq::context_t context (1); //Important that this stays alive.
	signed int connections = 0;
}

zmq::socket_t* zmq_conn::connect() {
	//show_backtrace("connect");
	if (zmq_port == 0) {
		zmq_port = getenv("ZMQ_PORT");
	} 
	zmq::socket_t *socket = new zmq::socket_t (context, ZMQ_REQ);

	socket->connect (zmq_conn::zmq_port);
	connections++;
	//std::cerr << "Connecting for the " << connections << "th time" <<std::endl;
	return socket;
}


void zmq_conn::disconnect(zmq::socket_t *socket) {
	//std::cerr << "Disconnecting " << zmq_conn::zmq_port << std::endl;
	//std::cerr << "Num connections: "<<connections<<std::endl;
	assert (connections >0);

	connections--;
	socket->disconnect(zmq_port);
	delete	socket;
}


Wt::Json::Object zmq_conn::interact(const char value[], zmq::socket_t *socket) {
	return  zmq_conn::interact(std::string(value), false, socket);
}


Wt::Json::Object zmq_conn::interact(Wt::WString value, zmq::socket_t *socket) {
	return  zmq_conn::interact(value.narrow(),false, socket);
}


Wt::Json::Object zmq_conn::interact(Wt::Json::Object value, zmq::socket_t *socket) {
	return  zmq_conn::interact(Wt::Json::serialize(value), true, socket);
}


Wt::Json::Object zmq_conn::interact(std::string value, zmq::socket_t *socket) {
	return  zmq_conn::interact(value, false, socket);
}


Wt::Json::Object zmq_conn::interact(std::string value, bool raw, zmq::socket_t *socket) {
	bool disconn = false;

	if(socket == 0) {
		socket = connect();
		disconn = true;
	}
	send(value,socket, raw);
	Wt::Json::Object retval;
	Wt::Json::parse(recv(socket), retval);
	if (disconn) {
		disconnect(socket);
	}
	return retval;
}

void zmq_conn::send(std::string value, zmq::socket_t *socket, bool raw) {
	bool disconn = false;
	if(socket == 0) { 
		socket = connect();
		disconn = true;
	}
	std::string data;
	if (!raw) {
		Wt::Json::Value msg_str;
		msg_str = Wt::WString( value);
		Wt::Json::Array msg_arr;
		msg_arr.push_back(msg_str);
		data = Wt::Json::serialize(msg_arr);
	}
	else {
		data = value;
	}
	//std::cerr << "Sending data " << data << std::endl;
	socket->send(zmq::buffer(data.c_str(), data.size()), zmq::send_flags::none);
	if(disconn) {
		disconnect(socket);
	}
	return;
}

std::string zmq_conn::recv(zmq::socket_t *socket) {
	char data[MAXSIZE];
	zmq::mutable_buffer buffer(data, sizeof(data));
	auto nbytes = socket->recv(buffer, zmq::recv_flags::none);
	if (!nbytes) {
		std::cerr << "zmq recv failed!" << std::endl;
		std::abort();
	}
	//std::cerr << "Received " << std::string(data, nbytes->size) << std::endl;
	return std::string(data, nbytes->size);
}
