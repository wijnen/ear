/*
 * Copyright (C) 2008 Emweb bvba, Heverlee, Belgium.
 *
 * See the LICENSE file for terms of use.
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

//#define MAXSIZE 1048576
#define MAXSIZE 1048576
// c++0x only, for std::bind
// #include <functional>



using namespace Wt;

  Wt::WTreeTableNode *addNode(Wt::WTreeTableNode *parent, const char *name, const double start, const double end,          const double duration);
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
  void loadMarkers(zmq::socket_t &socket,WString trackname);
  void updateInputs();
  

};

/*
 * The env argument contains information about the new session, and
 * the initial request. It must be passed to the WApplication
 * constructor so it is typically also an argument for your custom
 * application constructor.
*/
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
        button->setMargin(5, Left);                            // add 5 pixels margin
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
    Json::Array json_tracknames;  //From HERE //@Bas
    json_tracknames = tracks.get("tracks");
    std::vector<Wt::WString> tracknames;
    WString foo;
    for (auto trackname:json_tracknames)
	{	
		foo = trackname;
		tracknames.push_back(foo);
	} //To HERE is basically a cast between an Array of Json::Values (being strings, castable to WString) to a vector of WStrings. Can we do this in one line?
    trackmodel->setStringList(tracknames);
    trackcombo->setNoSelectionEnabled(true);
    trackcombo->setModel(trackmodel);
    trackcombo->setObjectName("trackComboBox"); 
    trackcombo->changed().connect(std::bind([=] () 
    {
    	int row = trackcombo->currentIndex();
	interact_zmq("track:"+std::to_string(row));
	updateInputs();
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
        inputSlider->resize(500,50); //UI values
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
 
    socket.disconnect("tcp://localhost:5555");

    Wt::WContainerWidget *markerContainer = new Wt::WContainerWidget;
    root()->addWidget(markerContainer);
   // markerContainer->setHidden(true);

    Wt::WTreeTable *markerTree = new Wt::WTreeTable();
    markerContainer->addWidget(markerTree);
    markerTree->resize(600,600); 
    markerTree->setObjectName("markertree");
    markerTree->tree()->setSelectionMode(Wt::ExtendedSelection);
    markerTree->addColumn("Start",200);
    markerTree->addColumn("End",200);
    markerTree->addColumn("Duration",200);
    markerTree->addColumn("Foo",200);
   






    updateInputs();


}

void EarUI::loadMarkers(zmq::socket_t &socket,WString trackname)
{
	Wt::WTreeTable *treeTable; //This should become a WAbstractItemModel to make an MVC TreeTable. I'll need some help with that.
	treeTable = dynamic_cast<WTreeTable*> (findWidget("markertree"));
	//Wt::WTreeTableNode *root = new Wt::WTreeTableNode(trackname);
	Wt::WTreeTableNode *root = new Wt::WTreeTableNode("stuff");
	treeTable->setTreeRoot(root, "Markers for this track");
	Wt::WTreeTableNode *current_root = root;
	double current_end=0;
	Json::Array markers;
	Json::Object response;
	response=interact_zmq(socket,"markers?");
	markers=response.get("markers");
	Json::Object marker;
	double start,end,duration;
	WString name;
	for(auto vmarker:markers)
	{	
		marker=vmarker;
		start=marker.get("start");
		end=marker.get("end");
		name=marker.get("name");
		duration = end-start;
		std::string sname= name.narrow();
std::cout<<"Marker: "<<name<<" start:"<<start<<" end:"<<end<<std::endl;
		const char * cname = sname.c_str();

		if(end<current_end) //We're currently within this marker
		{
			addNode(current_root,cname,start,end,duration);
		}
		else
		{
std::cout<<"Rooting the marker"<<std::endl;
			current_end = end;
			current_root = addNode(current_root,cname,start,end,duration);
		}
			
	}	
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
		textWidget->setText(name + ":" + sliderWidget->valueText()); //The slider does the casting!

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
		loadMarkers(socket,comboBox->currentText() );
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





Wt::WTreeTableNode *addNode(Wt::WTreeTableNode *parent, const char *name,
				const double start, const double end,
				const double duration) {

	Wt::WTreeTableNode *node = new Wt::WTreeTableNode(name, 0, parent);
	Wt::WPushButton *startButton = new Wt::WPushButton("&darr;");	
	Wt::WPushButton *endButton = new Wt::WPushButton("&uarr;");
	node->setColumnWidget(1,startButton);
	node->setColumnWidget(2,endButton);
	//node->doubleClicked().connect(std::bind([=]() { //There is no doubleClicked signal, unfortunately.
/*	node->selected().connect(std::bind([=]() {
std::cout<<"Handlign a startbutton click from the markertree"<<std::endl;
	//Json::Object send; This should be a dict with a name at some point, so we can send the actual start time as a double. However, the earUI always expects a string at this point
//	EarUI::interact_zmq("play:"+std::to_string(start));
	WString command="play:"+std::to_string(start);
	EarUI::interact_zmq(command);
		

	}));*/
	startButton->clicked().connect(std::bind([=]() { //@Bas: If we uncomment this, it'll compile, but segfault on page load (not ever outputting the line belwo). This function should not even be called before a button is clicked. How?
std::cout<<"Handlign a startbutton click from the markertree"<<std::endl;

		Wt::WTreeTableNode *otherNode;
		for (auto nodeObj:parent->tree()->treeRoot()->childNodes())
		{
			otherNode = dynamic_cast<Wt::WTreeTableNode*> (nodeObj);
			Wt::WText *otherStartWidget = dynamic_cast<Wt::WText*>( otherNode->columnWidget(3)); //See *
//			Wt::WText *otherEndWidget = dynamic_cast<Wt::WText*>(otherNode->columnWidget(4)); //See *
			std::string foo = otherStartWidget->text().narrow();
			if (std::atof( foo.c_str()) < start) //Start time of this widget
			{
				otherNode->tree()->select(otherNode);
			}
			else
			{
				otherNode->tree()->select(otherNode,false);
			}
		}
	}));	
	Wt::WText *startWidget = new Wt::WText(std::to_string(start)); //Let's store our private variables in hidden widgets instead of subclassing this mess. Actually, we probably want to show it at some point anyway. Subclassing might be needed/better
	
	Wt::WText *endWidget = new Wt::WText(std::to_string(end));
	startWidget->setHidden(true);
	endWidget->setHidden(true);
	node->setColumnWidget(3, startWidget); //See *
	node->setColumnWidget(4, endWidget);
	
//i//	node->setColumnWidget(4, new Wt::WText(std::to_string(duration)));*/
	return node;
    }







void EarUI::clicked(WPushButton* source )
{
std::cout <<"Clicked: " << source->text() <<std::endl;
   interact_zmq("event:"+source->text().narrow());
updateInputs();
}

Json::Object EarUI::interact_zmq(Wt::WString value)
{
	return interact_zmq(value.narrow());
}


Json::Object EarUI::interact_zmq(Json::Object value) //Implementation is exact same as below, because of same names. Can this be any better in C? //@Bas
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
  /*
   * You could read information from the environment to decide whether
   * the user has permission to start a new application
   */
  return new EarUI(env);
}







int main(int argc, char **argv)
{
  /*
   * Your main method may set up some shared resources, but should then
   * start the server application (FastCGI or httpd) that starts listening
   * for requests, and handles all of the application life cycles.
   *
   * The last argument to WRun specifies the function that will instantiate
   * new application objects. That function is executed when a new user surfs
   * to the Wt application, and after the library has negotiated browser
   * support. The function should return a newly instantiated application
   * object.
   */
  return WRun(argc, argv, &createApplication);
}

