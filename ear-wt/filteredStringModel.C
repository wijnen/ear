#include "filteredStringModel.h"

FilteredStringModel::FilteredStringModel(std::string zmqString, WObject *parent) : Wt::WStringListModel(parent)
{
	searchString = "";
	this->zmqString = zmqString;
}


void FilteredStringModel::update()
{


	std::string retval = "{";
	retval += "\""+zmqString.narrow()+"\" : {";
 	retval += "\"searchString\" : \""+searchString.narrow()+"\", ";
	retval += "\"musts\" : [";
	for (auto filter:musts)
	{
		retval += "\""+filter.narrow()+ "\" , ";
	}
	if(musts.size()>0)
	{
		retval = retval.substr(0,retval.length() -3); //Remove comma
	}
	retval += "], ";
	retval += "\"mays\" : [";
	for (auto filter:mays)
	{
		retval += "\""+filter.narrow()+ "\" , ";
	}
	if(mays.size()>0)
	{
		retval = retval.substr(0,retval.length() -3); //Remove comma
	}
	retval += "] ";
	retval +="}}";


/*
    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_REQ);
 
    socket.connect (zmq_port);
    Wt::Json::Object response;
	socket.send(retval.c_str(),retval.size());	
	char buffer[MAXSIZE];
    int nbytes = socket.recv(buffer, MAXSIZE);
      Wt::Json::parse(std::string(buffer, nbytes),response);
	Wt::Json::Array options = response.get("options");
    socket.disconnect(zmq_port); KRLZMQ*/
        Wt::Json::Object ooptions = zmq_conn::interact(retval, true);
        Wt::Json::Array options = ooptions.get("options");
	int x=0;
	removeRows(0,rowCount());
	for(auto voption:options)
	{
	Wt::Json::Array option = voption;
	std::string name = option[0];
	int idx = option[1];
	        addString(name);
		setData(x,0,idx,Wt::UserRole);
		x++;
	}

}

