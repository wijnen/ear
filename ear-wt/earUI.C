/*
 * Example used: Copyright (C) 2008 Emweb bvba, Heverlee, Belgium.
 *
 * Code copyright Kasper Loopstra, 2017  
 */

#include <Wt/WApplication>
#include <Wt/WBreak>
#include <Wt/WContainerWidget>
#include <Wt/WLineEdit>
#include <Wt/WPushButton>
#include <Wt/WText>
#include <Wt/WGroupBox>
#include <Wt/WAnimation>
#include <Wt/WStringListModel>
#include <Wt/WComboBox>
#include <Wt/WPanel>
#include <Wt/WSlider>
#include <Wt/Json/Array>
#include <Wt/Json/Parser>
#include <Wt/Json/Object>
#include <Wt/Json/Serializer>
#include <Wt/WSignalMapper>
#include <Wt/WTreeTable>
#include <Wt/WTree>
#include <Wt/WTreeTableNode>
#include <zmq.hpp>
#include <string>
#include <iostream>

#define MAXSIZE 1048576 //Maximum size of ZMQ read buffer used here. 



using namespace Wt;

  Wt::WTreeTableNode *addNode(Wt::WTreeTableNode *parent, WString name, const double start);
class EarUI : public WApplication
{
public:
  EarUI(const WEnvironment& env);

static  std::string recv_zmq(zmq::socket_t &socket);
static  void send_zmq(zmq::socket_t &socket, std::string value);
static  void send_zmq(std::string value);
static  Json::Object interact_zmq(std::string value);
static  Json::Object interact_zmq(Wt::WString value);
static  Json::Object interact_zmq(Json::Object);
static  Json::Object interact_zmq(zmq::socket_t &socket,std::string value);
static  Json::Object interact_zmq(zmq::socket_t &socket,Json::Object value);
//Public static so I can use them from the node callback, that's in a Wt object, not an EarUI one.

private:
  void clicked(WPushButton* source );
//  void loadMarkers(zmq::socket_t &socket,WString trackname);
  void loadFragments(zmq::socket_t &socket);
  void loadFragments();
  void updateInputs();
  void loadGroup(Wt::WTreeTableNode *current_root, Json::Array fragments);

};

EarUI::EarUI(const WEnvironment& env)
  : WApplication(env)
{
    setTitle("Ear test interface"); 

    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_REQ);
    socket.connect ("tcp://localhost:5555"); //TODO: make host and port configurable
/*
ZMQ should connect and disconnect after every set of actions to make room for another interface. Saves working on lots of listeners and the Python interface doesn't have to know who's talking
*/
    Json::Object command_names; //
    command_names = interact_zmq(socket,"events?");
    std::set<std::string> names;
    names = command_names.names();
std::cout<<"Parsed names"<<std::endl;

    
    Wt::WPanel *panel = new Wt::WPanel();

    panel->setTitle("Control current track");
    panel->setObjectName("trackpanel");
    panel->addStyleClass("centered-example");
    panel->setCollapsible(true);
    Wt::WAnimation animation(Wt::WAnimation::SlideInFromTop,
			 Wt::WAnimation::EaseOut,
			 100);
    panel->setAnimation(animation);
    Wt::WContainerWidget *currentTrackContainer = new Wt::WContainerWidget();
    panel->setCentralWidget(currentTrackContainer);
    root()->addWidget(panel);
    panel->setHidden(true);

    Wt::WSignalMapper<WPushButton *> *ButtonMap = new Wt::WSignalMapper<Wt::WPushButton *>(this);
    ButtonMap->mapped().connect(this,&EarUI::clicked);
    for (auto name : names) 
    {
        WPushButton *button = new WPushButton(name ,root());     
        button->setMargin(5, Left);                          
	ButtonMap->mapConnect(button->clicked(),button);
	currentTrackContainer->addWidget(button);
    }
    
    Wt::WContainerWidget *trackListContainer = new Wt::WContainerWidget();
    root()->addWidget(trackListContainer);

    Json::Object tracks;

    tracks = interact_zmq(socket,"tracks?");

    Wt::WComboBox *trackcombo = new Wt::WComboBox(trackListContainer);
    trackcombo->setMargin(10, Wt::Right);
    Wt::WStringListModel *trackmodel = new Wt::WStringListModel(trackcombo);
    Json::Array json_tracknames;  
    json_tracknames = tracks.get("tracks");
    std::vector<Wt::WString> tracknames;
    WString foo;
    for (auto trackname:json_tracknames)
	{	
		foo = trackname;
		tracknames.push_back(foo);
	} 
    trackmodel->setStringList(tracknames);
    trackcombo->setNoSelectionEnabled(true);
    trackcombo->setModel(trackmodel);
    trackcombo->setObjectName("trackComboBox"); 
    trackcombo->changed().connect(std::bind([=] () 
    {
    	int row = trackcombo->currentIndex();
	interact_zmq("track:"+std::to_string(row));
	updateInputs();
	loadFragments();
    }));

    Wt::WContainerWidget *inputContainer = new Wt::WContainerWidget();
    root()->addWidget(inputContainer);
    Json::Object inputs;
    inputs = interact_zmq(socket,"inputs?");
    Wt::WContainerWidget *thisInputContainer;
    Wt::WSlider *inputSlider;
    Json::Array inputSettings;
    Wt::WText *inputText; 
    for(auto input_name:inputs.names())   
    {
	thisInputContainer = new Wt::WContainerWidget();
        inputContainer->addWidget(thisInputContainer);
	inputSettings = inputs.get(input_name);
 	inputSlider = new Wt::WSlider(thisInputContainer);
	inputSlider->setObjectName("inputSlider:"+input_name);
        inputSlider->resize(500,50);
        inputSlider->setMinimum(inputSettings[0]); 
        inputSlider->setMaximum(inputSettings[1]); 
        inputSlider->setValue(inputSettings[2]); 
	inputSlider->setTickPosition(Wt::WSlider::TicksAbove);
        inputText = new Wt::WText(thisInputContainer);
	inputText->setObjectName("inputText:"+input_name);
	inputContainer->addWidget(inputSlider);
	inputContainer->addWidget(inputText);
	inputText->setText(input_name+":"+inputSlider->valueText());
	
	inputSlider->valueChanged().connect(std::bind([=] ()
        {
        	interact_zmq("input:"+input_name+":"+inputSlider->valueText());
		inputText->setText(input_name+":"+inputSlider->valueText());
		updateInputs();
        }));
    }
    
//    beforeSlider =  new Wt::WSlider(thisInputContainer);
 
    socket.disconnect("tcp://localhost:5555");

    Wt::WContainerWidget *markerContainer = new Wt::WContainerWidget;
    root()->addWidget(markerContainer);
   // markerContainer->setHidden(true);

    Wt::WTreeTable *markerTree = new Wt::WTreeTable();
    markerContainer->addWidget(markerTree);
    markerTree->resize(1000,1000); 
    markerTree->setObjectName("markertree");
    markerTree->tree()->setSelectionMode(Wt::ExtendedSelection);
    markerTree->addColumn("Start",200);
    markerTree->addColumn("",50); //StartButton
    markerTree->addColumn("",200);
    markerTree->addColumn("",200);
   






    updateInputs();


}

void EarUI::loadGroup(Wt::WTreeTableNode *current_root, Json::Array fragments)
{ //Recursively add the fragments to the treetable
std::cout<<"\n\nLoading fragments"<<std::endl;
	for(auto fragmentValue:fragments)
	{
		const Json::Array& fragment = fragmentValue;
		std::string type = fragment[0];
		WString name = fragment[1];
std::cout<<"Loading fragment "<<name<<" of type "<<type<<std::endl;
		if (type == "group")
		{
std::cout<<"Making a new group"<<std::endl;
			
			loadGroup( addNode(current_root,name,0) ,fragment[2]);	
		}
		else if (type == "fragment")
		{
			int start_time = fragment[2];
			addNode(current_root,name,start_time);
		}
		else
		{
std::cout<<"Type not understood"<<std::endl;
		}
	}
		
}

void EarUI::loadFragments()
{
    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_REQ);
    socket.connect ("tcp://localhost:5555");
	loadFragments(socket);
    socket.disconnect("tcp://localhost:5555");
}
void EarUI::loadFragments(zmq::socket_t &socket)
{

	Wt::WTreeTable *treeTable; 
	treeTable = dynamic_cast<WTreeTable*> (findWidget("markertree"));
	Wt::WTreeTableNode *root = new Wt::WTreeTableNode("Fragments");
	treeTable->setTreeRoot(root, "Fragments for this track");
	Wt::WTreeTableNode *current_root = root;
	Json::Array fragments;
	Json::Object response;
	response = interact_zmq(socket,"fragments?");
	fragments = response.get("fragments");

	loadGroup(current_root,fragments);
	root->expand();
}


void EarUI::updateInputs()
{	
    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_REQ);
    socket.connect ("tcp://localhost:5555");
	Json::Object responses;
	responses=interact_zmq(socket,std::string("inputs?"));
	Json::Array inputSettings;
	WSlider *sliderWidget;
	WText *textWidget;
	for(auto name:responses.names())
	{
		inputSettings = responses.get(name);
		sliderWidget = dynamic_cast<WSlider*> (findWidget("inputSlider:"+name));
		sliderWidget->setValue(inputSettings[2]);
		textWidget = dynamic_cast<WText*> (findWidget("inputText:"+name));
		textWidget->setText(name + ":" + sliderWidget->valueText()); //The slider does the casting from double to string!

	}
	WComboBox *comboBox;
	comboBox = dynamic_cast<WComboBox*> (findWidget("trackComboBox"));
	Json::Object response;
	response=interact_zmq(socket,"track?");
	int track;
	track = response.get("current");
std::cout<<"Current track in ui is "<<comboBox->currentIndex()<<std::endl;
std::cout<<"Current track in ear is "<<track<<std::endl;
	if (track !=comboBox->currentIndex())
	{
std::cout<<"Loading markers"<<std::endl;
		loadFragments(socket);
	}	
	else
	{
std::cout<<"Not loading markers"<<std::endl;
	}
	comboBox->setCurrentIndex(track) ;
	WPanel *panel;
	panel = dynamic_cast<WPanel*>(findWidget("trackpanel"));
std::cout<<"Set track to "<<comboBox->currentIndex()<<std::endl;
	if ( track != -1)
	{
		panel->setHidden(false);
	}
	else
	{
		panel->setHidden(true);
	}


	
    socket.disconnect("tcp://localhost:5555");
		
}





Wt::WTreeTableNode *addNode(Wt::WTreeTableNode *parent, WString name, const double start ) {
	Wt::WTreeTableNode *node = new Wt::WTreeTableNode(name, 0, parent);
Wt::WText *startWidget = new Wt::WText(std::to_string(start));  
/*	node->selected.connect(std::bind([=]() {
std::cout<<"Selected a node, please select the children"<<std::endl;
	}));*/
//	startWidget->setHidden(true);
	node->setColumnWidget(2, startWidget); 


	Wt::WPushButton *startButton = new WPushButton("|>");
	startButton->clicked().connect(std::bind([=]() {
std::cout<<"Handlign a startbutton click from the markertree"<<std::endl;
		WString command="play:"+std::to_string(start); //In the future, this should send a list of start times to start from, in case multiple markers are selected. The last marker also automatically indicates an end time (the end time of that fragment). For now, send a single string
		EarUI::interact_zmq(command);
	}));
	
	
	node->setColumnWidget(3, startButton);
	return node;
    }







void EarUI::clicked(WPushButton* source )
{
std::cout <<"Clicked: " << source->text() <<std::endl;
   interact_zmq("event:"+source->text().narrow());
updateInputs();
}


/*

ZeroMQ handling below.
An interact function can take a socket or not. If provided with a socket, it will be used. If not, one will be opened and closed for this interaction. 
Send and recieve always need a socket.
When sending more than one interaction, preferably create a socket and use that.
WStrings, strings are both handled
*/


Json::Object EarUI::interact_zmq(Wt::WString value)
{
	return interact_zmq(value.narrow());
}


Json::Object EarUI::interact_zmq(Json::Object value)//Should use a function template 
{
    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_REQ);
    socket.connect ("tcp://localhost:5555");
std::cout<<"Connected to ZMQ"<<std::endl;
    Json::Object retval;
    retval = interact_zmq(socket,value);	
    socket.disconnect("tcp://localhost:5555");
std::cout<<"Disonnected from ZMQ"<<std::endl;
    return retval;
}


Json::Object EarUI::interact_zmq(std::string value)
{   
    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_REQ);
    socket.connect ("tcp://localhost:5555");
std::cout<<"Connected to ZMQ"<<std::endl;
    Json::Object retval;
    retval = interact_zmq(socket,value);	
    socket.disconnect("tcp://localhost:5555");
std::cout<<"Disonnected from ZMQ"<<std::endl;
    return retval;
}

Json::Object EarUI::interact_zmq(zmq::socket_t &socket,Json::Object value)
{
	return interact_zmq(socket, Json::serialize(value));
}


Json::Object EarUI::interact_zmq(zmq::socket_t &socket,std::string value)
{
    send_zmq(socket,value);
    Json::Object retval;
std::cout<<"Recieving and parsing"<<std::endl;	
    Json::parse(recv_zmq(socket),retval);
std::cout<<"Parsed"<<std::endl;
    return retval;
}
void EarUI::send_zmq(zmq::socket_t &socket, std::string value) 
{
std::cout<<"Sending stuff: "<<value<<std::endl;
    Json::Value msg_str;
    msg_str = WString( value);
    Json::Array msg_arr;
    msg_arr.push_back(msg_str);
    std::string data = Json::serialize(msg_arr);
    socket.send(data.c_str(),data.size());
}


std::string EarUI::recv_zmq(zmq::socket_t &socket) 
{
std::cout<<"Recieving stuff"<<std::endl;
    char buffer[MAXSIZE];
    int nbytes = socket.recv(buffer, MAXSIZE);
std::cout<<"Recieved stuff"<<buffer<<std::endl;
    return std::string(buffer, nbytes);
}


WApplication *createApplication(const WEnvironment& env)
{
  return new EarUI(env);
}







int main(int argc, char **argv)
{
  return WRun(argc, argv, &createApplication);
}

