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
#include <Wt/WString>
#include <Wt/WGroupBox>
#include <Wt/WAnimation>
#include <Wt/WStringListModel>
#include <Wt/WComboBox>
#include <Wt/WPanel>
#include <Wt/WSlider>
#include <Wt/WTimer>
#include <Wt/WInPlaceEdit>
#include <Wt/Json/Array>
#include <Wt/Json/Parser>
#include <Wt/Json/Object>
#include <Wt/Json/Serializer>
#include <Wt/WSignalMapper>
#include <Wt/WTreeTable>
#include <Wt/WTree>
#include <Wt/WTreeNode>
#include <Wt/WTreeTableNode>
#include <zmq.hpp>
#include <string>
#include <iostream>
#include <boost/range/adaptor/reversed.hpp>
#include <Wt/WCompositeWidget>
#include <Wt/WTreeTableNode>

#define MAXSIZE 1048576 //Maximum size of ZMQ read buffer used here. 

class MyTreeTableNode : public Wt::WTreeTableNode
{
	public:
		MyTreeTableNode(const Wt::WString& labelText, Wt::WIconPair *labelIcon = 0,  Wt::WTreeTableNode *parentNode = 0) :  
		Wt::WTreeTableNode( labelText,  labelIcon , parentNode )
		{
			
		}
	using Wt::WTreeTableNode::labelArea;
	Wt::WString text;		

};

class TimeWidget : public Wt::WText //This explicitly does not use the Wt::WTime class, as we want to use milliseconds as our base unit, especially when communicating with the Python side of this project. The Wt::Wtime supports only already quite correctly formatted times, and we don't need that. Additionally, it's meant for clock times, not intervals
{
    public:
	TimeWidget(Wt::WContainerWidget *parent = 0);
//  TimeWidget(double time, WContainerWidget *parent = 0);
	long long time();
	bool setTime(long time);
    private:
	long _time;
};


TimeWidget::TimeWidget(Wt::WContainerWidget *parent )
 : Wt::WText(parent)
{  }  

long long TimeWidget::time()
{
	return this->_time;
}

bool TimeWidget::setTime(long time)
{
	if(time != -1)
	{
		bool negative = false;
		if (time < 0)
		{
			negative = true;
			time *= -1; 
		}
			
		this->_time = time;
		int minutes, seconds, milliseconds;
		seconds =  ((time / 1000) % 60) ;
		minutes =  ((time / (1000*60)) % 60);
		milliseconds = time - seconds*1000 - minutes*60*1000; 
		if (negative)
		{
			this->setText(Wt::WString("- {1}:{2}:{3}").arg(minutes).arg(seconds).arg(milliseconds)); //TODO: zero-pad this string
		}
		else
		{
			this->setText(Wt::WString("{1}:{2}:{3}").arg(minutes).arg(seconds).arg(milliseconds)); //TODO: zero-pad this string
		}
	}
	return true;
}


using namespace Wt;



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
  Wt::WSlider *beforeSlider;
static long start_track_time;
static long stop_track_time;
static long time_speed;
private:
 std::vector<MyTreeTableNode*> fragment_set;
  void clicked(WPushButton* source );
  void loadFragments(zmq::socket_t &socket);
  void loadFragments();
  void updateInputs();
  void loadGroup(MyTreeTableNode *current_root, Json::Array fragments);
  void mark_current_fragment(long long track_time);
  long current_track_time( zmq::socket_t *socket  =0  );
  WSlider *posSlider;
  MyTreeTableNode *addNode(MyTreeTableNode *parent, WString name, const long start, const long stop );
  Wt::WStringListModel *get_trackmodel( zmq::socket_t &socket );
  Wt::WStringListModel *get_trackmodel( );
  Wt::WStringListModel *filter_trackmodel( WStringListModel *trackmodel, WContainerWidget *filterwidget );
Json::Value saveFragments(MyTreeTableNode *root);
  int max_tags = 0;
  WPushButton *playPauseButton;
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

    
    Wt::WPanel *panel = new Wt::WPanel(root());
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
    panel->setHidden(false);

    for (auto name : names) 
    {
        WPushButton *button = new WPushButton(name ,root());     
        button->setMargin(5, Left);                          
 	button->clicked().connect(std::bind([=] ()
	{
   		interact_zmq("event:"+name);
		updateInputs();
	}));
	currentTrackContainer->addWidget(button);


    }
   
    playPauseButton = new WPushButton("Play",currentTrackContainer);
   playPauseButton->clicked().connect(std::bind([=] ()
	{
		interact_zmq(std::string("event:pause"));
	} 
   	));
    Wt::WContainerWidget *trackListContainer = new Wt::WContainerWidget();
    root()->addWidget(trackListContainer);



    Wt::WComboBox *trackcombo = new Wt::WComboBox(trackListContainer); //TODO: Change from track list container to something searchable
    trackcombo->setMargin(10, Wt::Right);

    
    //trackcombo->setNoSelectionEnabled(true); //Depends on Wt version, disabled for now

    WStringListModel *trackmodel = get_trackmodel(socket);
    trackcombo->setModel(trackmodel);



    trackcombo->setObjectName("trackComboBox"); 
    trackcombo->changed().connect(std::bind([=] () 
    {
	WStringListModel *this_trackmodel = dynamic_cast<WStringListModel*>( trackcombo->model());
    	int row = trackcombo->currentIndex();
	int tracknumber = boost::any_cast<int>
                       (this_trackmodel->data(this_trackmodel->index(row,0), Wt::UserRole)); 
	interact_zmq("track:"+std::to_string(tracknumber));
	updateInputs();
	loadFragments();

                TimeWidget *firstW = dynamic_cast<TimeWidget*>((*fragment_set.begin())->columnWidget(1));
                TimeWidget *lastW = dynamic_cast<TimeWidget*>((*fragment_set.rbegin())->columnWidget(2));
                posSlider->setMinimum(firstW->time());
                posSlider->setMaximum(lastW->time());
	
    }));
    Wt::WContainerWidget *filterContainer = new WContainerWidget(trackListContainer);
    Wt::WContainerWidget *searchContainer = new WContainerWidget(filterContainer);
    Wt::WContainerWidget *filtersContainer = new WContainerWidget(filterContainer);
    Wt::WLineEdit *filterbox = new Wt::WLineEdit(searchContainer);
    filterbox->setPlaceholderText("Filter"); //TODO fuzzy search
    Wt::WPushButton *addFilter = new Wt::WPushButton("Add",searchContainer);
    addFilter->clicked().connect(std::bind([=] ()
    {
	Wt::WContainerWidget *thisFilter = new Wt::WContainerWidget(filtersContainer); 
	new WText(filterbox->text(),thisFilter);
	Wt::WPushButton *removeButton = new WPushButton("X",thisFilter);
		removeButton->clicked().connect(std::bind([=] ()
		{
			filtersContainer->removeWidget(thisFilter);
			thisFilter->clear();
			if(filtersContainer->count()>0)
			{
			 	trackcombo->setModel(filter_trackmodel(get_trackmodel(),filtersContainer));
			}
			else
			{
			 	trackcombo->setModel(get_trackmodel());
			}
		}));
	trackcombo->setModel(filter_trackmodel(trackmodel,filtersContainer));
	filterbox->setText("");
	filterbox->setPlaceholderText("Filter");
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
	inputSlider->setObjectName("inputSlider:"+input_name); //TODO: Change this, and updateInputs to using class members and not HTML names.
        inputSlider->resize(500,50);
	int min, max;
	min = inputSettings[0];
	max = inputSettings[1]; 
        inputSlider->setMinimum(min); 
        inputSlider->setMaximum(max); 
	inputSlider->setTickInterval( (max-min)/6);
        inputSlider->setValue(inputSettings[2]); 
	inputSlider->setTickPosition(Wt::WSlider::TicksAbove);
	if (input_name == "before")
	{
		std::vector<int> beforeButtons = {0,3,6,10};
		WContainerWidget *beforeButtonContainer = new WContainerWidget(thisInputContainer);
	       for(auto before:beforeButtons)
		{
			WPushButton *bbutton = new WPushButton(WString(std::to_string(before)),beforeButtonContainer );
			bbutton->clicked().connect(std::bind([=] ()
			{
				interact_zmq("input:before:"+std::to_string(before));
				updateInputs();
			}));
		}

	 	beforeSlider = inputSlider; //We need to reference this further on
	}
	if (input_name =="speed")
	{
		std::vector<int> speedButtons = {50,75,80,90,100,110,120,125,150,200};
		WContainerWidget *speedButtonContainer = new WContainerWidget(thisInputContainer);
	       for(auto speed:speedButtons)
		{
			std::string title = std::to_string(speed);
			title += "%";
			WPushButton *sbutton = new WPushButton(WString(title),speedButtonContainer );
			sbutton->clicked().connect(std::bind([=] ()
			{
				interact_zmq("input:speed:"+std::to_string(speed));
				updateInputs();
			} ));
			
    	        }
	  
	}
        inputText = new Wt::WText(thisInputContainer);
	inputText->setObjectName("inputText:"+input_name);
	inputContainer->addWidget(inputSlider);
//	inputContainer->addWidget(inputText);
	inputText->setText(input_name+": "+inputSlider->valueText());
	
	inputSlider->valueChanged().connect(std::bind([=] ()
        {
        	interact_zmq("input:"+input_name+":"+inputSlider->valueText());
		inputText->setText(input_name+": "+inputSlider->valueText());
		updateInputs();
        }));
    }
    WContainerWidget *posContainer = new WContainerWidget(inputContainer);
    WContainerWidget *posButtonContainer = new WContainerWidget(posContainer);

    std::vector<int> seekButtons = {-10,-5,-1,1,5,10};
    for(auto seek:seekButtons)
    {
	std::string title = std::to_string(seek);
	if (seek>0)
	{
		title = "+" + title;
	}
	title += "s";
	WPushButton *sbutton = new WPushButton(WString(title),posButtonContainer);
	sbutton->clicked().connect(std::bind([=] ()
        {
		interact_zmq("pos:"+std::to_string(current_track_time()+seek*1000));
        } ));
	
    }
    posSlider = new WSlider(posContainer);
    posSlider->resize(500,50);
    posSlider->setTickInterval(60000); //One minute in ms
    posSlider->setTickPosition(Wt::WSlider::TicksAbove);
    TimeWidget *posText = new TimeWidget(posContainer);
    posText->setTime(0);
    posSlider->valueChanged().connect(std::bind([=] ()
    {
std::cout<<"interacting pos"<<std::endl;
	interact_zmq("pos:"+posSlider->valueText());
std::cout<<"interacted pos"<<std::endl;
        posText->setTime(posSlider->value());
    }));
/*    posSlider->sliderMoved().connect(std::bind([=] (int v) //TODO:Implement this. I'm not sure why it does not currently work, as it looks like Wt casts the value to a static int. 
//Also TODO, use this to stop the slider from being moved by the udpating timer if someone is dragging it
    { 
	posText->setTime(v); //So we can move and see where we'll end up before releasing
    }));*/

    


  
    socket.disconnect("tcp://localhost:5555");

    Wt::WContainerWidget *markerContainer = new Wt::WContainerWidget;
    root()->addWidget(markerContainer);

    WTreeTable *markerTree = new WTreeTable();
    markerContainer->addWidget(markerTree);
    markerTree->setObjectName("markertree"); //TODO: Still something wierd with the columns, they are sometimes offset
    markerTree->tree()->setSelectionMode(Wt::ExtendedSelection);
    markerTree->addColumn("Start time",100);
    markerTree->addColumn("End time",100); //StartButton
    markerTree->addColumn("Play from here",20); //StartButton

    WPushButton *button = new WPushButton("Play selection" ,root());     
    button->setMargin(5, Left);
    button->clicked().connect(std::bind([=] ()
    {	
	std::string ret = "{\"play\": [";	
	bool first = true;
	for (auto fragmentTN:markerTree->tree()->selectedNodes())
	{
	//tree returns a tree with tree nodes. We need treetable nodes!
		MyTreeTableNode *fragmentTTN =  dynamic_cast<MyTreeTableNode*>(fragmentTN);
		TimeWidget *startW = dynamic_cast<TimeWidget*>(fragmentTTN->columnWidget(1));
		TimeWidget *stopW = dynamic_cast<TimeWidget*>(fragmentTTN->columnWidget(2));
		long start = startW->time();
		long stop = stopW->time();
		if (start ==-1 or stop ==-1) //Check for group markers //TODO FIXME// Somehow still broken, dunno why //This needs to take the child?
		{
			continue;
		}
		if (not first)
		{	
			ret += " , ";
		}
		else
		{
			start -= beforeSlider->value()*1000;
			//start_track_time = start;
		}
		first = false;
		ret += "["+std::to_string(start)+" , "+	std::to_string(stop)+"]\n";
		//Manual socket as we need to send our own little "JSON" string plaintext.
	}
	ret += "]}";
		   zmq::context_t context (1);
		   zmq::socket_t socket (context, ZMQ_REQ);
		   socket.connect ("tcp://localhost:5555");
		   socket.send(ret.c_str(),ret.size());
		   recv_zmq(socket); //Just dummy anyway
		   socket.disconnect("tcp://localhost:5555");
		

	
    }));
    currentTrackContainer->addWidget(button);


//TODO: Split out the tab and untab functions so we can call them with the keyboard
    WPushButton *tabButton = new WPushButton(">>>>" ,root());     
    tabButton->setMargin(5, Left);
   tabButton->clicked().connect(std::bind([=] ()
    {
    WTreeNode *parent;
    WTreeNode *newNode;
	std::set<WTreeNode*> selectedNodes =markerTree->tree()->selectedNodes();
			std::vector< WTreeNode*> siblings;
 	
	WTreeNode *firstNode = *selectedNodes.begin();
	if(firstNode == markerTree->tree()->treeRoot())
	{
		return;
	}
	parent = firstNode->parentNode();
	newNode = addNode(0 ,"Group" ,-1,-1);
	siblings = parent->childNodes();
	int index = std::find(siblings.begin(), siblings.end(), firstNode) - siblings.begin();
	parent->insertChildNode(index, newNode);
		
	for (auto node:selectedNodes)
	{	
		parent->removeChildNode(node);
		newNode->addChildNode(node);
	}
    }));	
    currentTrackContainer->addWidget(tabButton);
    WPushButton *untabButton = new WPushButton("<<<<<" ,root());     
    untabButton->setMargin(5, Left);
    untabButton->clicked().connect(std::bind([=] ()
    {
	std::vector< WTreeNode*> siblings;
	WTreeNode *parent;
	WTreeNode *grandparent;
	std::vector< WTreeNode*> uncles; //My parents siblings
	int index = 0;

	std::set<WTreeNode*> selectedNodes =markerTree->tree()->selectedNodes();
	WTreeNode *firstNode = *selectedNodes.begin();
	WTreeNode *lastNode = *selectedNodes.rbegin(); //Note, the reverse of the beginning is not the end
	parent = firstNode->parentNode();
	if(parent == markerTree->tree()->treeRoot())
	{
		return;
	}
	siblings = parent->childNodes();

	if(firstNode != siblings.front() and lastNode != siblings.back())
	{ 
		return;
	}
	grandparent = parent->parentNode();
	uncles = grandparent->childNodes();
	index = std::find(uncles.begin(), uncles.end(), parent) - uncles.begin();
	if (firstNode == siblings.front() and lastNode != siblings.back())
	{
		index--;
	}
	for (auto node:selectedNodes)
	{
		parent->removeChildNode(node);
		index ++;
		grandparent->insertChildNode(index,node);
	}
    }));	
    currentTrackContainer->addWidget(untabButton);





    WPushButton *button1 = new WPushButton("Split fragment here", root());
    button1->setMargin(5, Left);
    button1->clicked().connect(std::bind([=] ()
    {	
	long pos = current_track_time();	
	for (auto fragmentTTN:this->fragment_set)
	{
		TimeWidget *startW = dynamic_cast<TimeWidget*>(fragmentTTN->columnWidget(1));
		TimeWidget *stopW = dynamic_cast<TimeWidget*>(fragmentTTN->columnWidget(2));
		long start = startW->time();
		long stop = stopW->time();
		if(pos > start and pos < stop)
		{
			//Fragment to split
			stopW->setTime(pos);
			WTreeNode *my_parent = fragmentTTN->parentNode();
			const std::vector< WTreeNode * > siblings = my_parent->childNodes();
			int index = -1;
			for(unsigned int i=0;i<siblings.size()+1;i++) //There is insert, and insertBefore, but no insertAfter. So first, determine the index of the widget we're splitting, and then insert at that index.
			{
				if(fragmentTTN == siblings[i])
				{
					index = i;
				}
			}
			if (index ==-1)
			{
				std::cout<<"Can't find myself when splitting a fragment"<<std::endl;
				return;
			}
			MyTreeTableNode *newFragmentTTN = addNode(0,"New node", pos, stop);
			my_parent->insertChildNode(index+1, newFragmentTTN);
		}
		
	}	
    }	));
    currentTrackContainer->addWidget(button1);


    WPushButton *savebutton = new WPushButton("Save fragments", root());
    currentTrackContainer->addWidget(savebutton);
    savebutton->setMargin(5, Left);
    savebutton->clicked().connect(std::bind([=] ()
    {	
	Json::Value fragmentsval = saveFragments(dynamic_cast<MyTreeTableNode*>(markerTree->tree()->treeRoot() ));
	Json::Array& fragments = fragmentsval; 
	std::string fragstring = Json::serialize(fragments);
	fragstring = "{ \"fragments\" : "+fragstring + "}";
			   zmq::context_t context (1);
		   zmq::socket_t socket (context, ZMQ_REQ);
		   socket.connect ("tcp://localhost:5555");
		   socket.send(fragstring.c_str(),fragstring.size());
		   recv_zmq(socket); //Just dummy anyway
		   socket.disconnect("tcp://localhost:5555");
    }));	

   TimeWidget *currentTime = new TimeWidget();
   currentTime->setTime(0);


   Wt::WTimer *timer = new Wt::WTimer();  
   timer->setInterval(50);
   timer->timeout().connect(std::bind([=] ()
   {
    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_REQ);
    socket.connect ("tcp://localhost:5555");

long long track_time = 	EarUI::current_track_time(&socket);
  	currentTime->setTime(track_time);
	posSlider->setValue(track_time);
	posText->setTime(track_time);
	//TODO: Add a query for the playing state in here as well. This allows us to make play/pause a single button
//Also, change the interval. When not playing, we can be slower updating and catch up within a second or something. This'll mainly help making the logs more readable, but also might help some performance

Json::Object playingj;
bool playing;
	playingj = interact_zmq(socket,std::string("playing?"));

	playing = playingj.get("playing");
	if (playing)
	{
		playPauseButton->setText("Pause");
	}
	else
	{
		playPauseButton->setText("Play");
	}
    socket.disconnect("tcp://localhost:5555");
mark_current_fragment(track_time); 
   }));

	timer->start();
	root()->addWidget(currentTime);
    updateInputs();


}
void EarUI::mark_current_fragment(long long pos)
{
	for (auto fragmentTTN:this->fragment_set)
	{
		TimeWidget *startW = dynamic_cast<TimeWidget*>(fragmentTTN->columnWidget(1));
		TimeWidget *stopW = dynamic_cast<TimeWidget*>(fragmentTTN->columnWidget(2));
		long start = startW->time();
		long stop = stopW->time();
		if(pos > start and pos < stop)
		{ 
			fragmentTTN->decorationStyle().setBackgroundColor(WColor(255,0,0)); //TODO: ?Make a proper style, and enlarge the font or something
//TODO: Maybe get the parents too, if the current widget is not shown			

		}
		else
		{

		fragmentTTN->decorationStyle().setBackgroundColor(WColor(255,255,255)); //TODO: Properly remove the previously added style

		}
	
	}
}
long EarUI::current_track_time( zmq::socket_t *socket )
{

	Json::Object posj ;
	if (socket == 0 )
	{
		posj = interact_zmq(std::string("pos?"));
	}
	else
	{
		posj = interact_zmq(*socket,std::string("pos?"));
	}
	Json::Value posjv = posj.get("pos");	
	const long long pos = posjv;
	return pos;
	
}

Wt::WStringListModel *EarUI::filter_trackmodel( WStringListModel *trackmodel, WContainerWidget *filterWidget )
{
std::cout<<"Handling filter"<<std::endl;
	std::set<WString> filters;
	std::set<WString> tags;
	Wt::WStringListModel *newtrackmodel = new Wt::WStringListModel();
	int track_idx;
	for(auto thisFilterContainerW:filterWidget->children())
	{
		WContainerWidget *thisFilterContainer = dynamic_cast<WContainerWidget*>(thisFilterContainerW);
		WWidget *foo = thisFilterContainer->widget(0);
		WText *filtertext = dynamic_cast<WText*>(foo);
		filters.insert(filtertext->text());		
	}
std::cout<<"Made filter set"<<std::endl;
	int i = 0; //Needed to look up the data as well as the set
	int x = 0; //Needed to set new data
	for (auto track:trackmodel->stringList())
	{
		tags = boost::any_cast<std::set<WString>> (trackmodel->data(trackmodel->index(i,1), Wt::UserRole+1)); 
		track_idx = boost::any_cast<int> (trackmodel->data(trackmodel->index(i,0), Wt::UserRole)); 
		std::set<WString> res_set;
	 	set_intersection(filters.begin(), filters.end(), tags.begin(), tags.end(), std::inserter(res_set, res_set.end())); 
		if( res_set.size() > 0) // if tag in filters  
//Currently we are ORring our filters, we probably want to AND our filters. This should also be made optional I guess
		{
				newtrackmodel->addString(track);
				newtrackmodel->setData(x,0,track_idx,Wt::UserRole);
				newtrackmodel->setData(x,1,tags,Wt::UserRole+1);
				x++; //RowIndex in new track model
		}
		i++;
	 }
    trackmodel->setFlags(x-1,0);
    trackmodel->sort(0);
	 
 return newtrackmodel;
}

Wt::WStringListModel *EarUI::get_trackmodel( )
{
    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_REQ);
    socket.connect ("tcp://localhost:5555");
	
	WStringListModel *retval = get_trackmodel(socket);

   socket.disconnect("tcp://localhost:5555");
std::cout<<"Disonnected from ZMQ"<<std::endl;
    return retval;
}


Wt::WStringListModel *EarUI::get_trackmodel(zmq::socket_t &socket )
{
    Wt::WStringListModel *trackmodel = new Wt::WStringListModel();
    Json::Object json_tracks;
    Json::Object response;
    response = interact_zmq(socket,"tracks?");
    json_tracks = response.get("tracks");
    Json::Object track;
    Json::Array json_tags;
    Json::Value json_tag;
    WString trackname;
    WString tag;
    int x = 0;
    for(auto trackname:json_tracks.names())
    {
	trackmodel->addString(trackname);
	json_tags = json_tracks.get(trackname);
	std::set<WString> tags;
	bool first =true;
	for (auto json_tag:json_tags)
	{
		if (first)
		{
			int track_idx = json_tag;
			trackmodel->setData(x,0,track_idx, Wt::UserRole); //First one is special, it's the track index
			first = false;
		}
		else
		{	
			tag = json_tag;
			tags.insert(tag);
		}
	}
	trackmodel->setData(x,1,tags, Wt::UserRole+1); //Note, these need to be fetched in the same way. I have no idea what the second argument here actually does, UserRole+1 is the actual index
 	x++;
    }
    max_tags = x;
    trackmodel->setFlags(x-1,0);
    trackmodel->sort(0);
	return trackmodel;

}

Json::Value EarUI::saveFragments(MyTreeTableNode *root)
{

	Json::Value retVal = Json::Value(Json::ArrayType);
std::cout<<__LINE__<<std::endl;
	Json::Array& ret = retVal; 
	WString name;
//	WText *nameWidget = root->label();
std::cout<<__LINE__<<std::endl;
	name = root->text;	
std::cout<<__LINE__<<std::endl;
	if(root->childNodes().size()>0)
	{
		ret.push_back(Json::Value(WString("group")));
		ret.push_back(Json::Value(name));
		Json::Value out_children_value = Json::Value(Json::ArrayType);	
		Json::Array& out_children = out_children_value;
		for(auto mynode:root->childNodes())
		{
			out_children.push_back(saveFragments(dynamic_cast<MyTreeTableNode*>(mynode)));
		}

		ret.push_back(out_children_value);

	}
	else //TODO add room for annotations
	{
		ret.push_back(Json::Value(WString("fragment")));
		ret.push_back(Json::Value(name));
		TimeWidget *startW = dynamic_cast<TimeWidget*>(root->columnWidget(1));
		TimeWidget *stopW = dynamic_cast<TimeWidget*>(root->columnWidget(2));
		long long start = startW->time();
		long long stop = stopW->time();
		ret.push_back(Json::Value(start));
		ret.push_back(Json::Value(stop));
	}
return retVal;
}
void EarUI::loadGroup(MyTreeTableNode *current_root, Json::Array fragments)
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
			
			loadGroup( addNode(current_root,name,-1,-1) ,fragment[2]);	
		}
		else if (type == "fragment")
		{
			long long start_time = fragment[2]; 
			long long stop_time = fragment[3];
			addNode(current_root,name,start_time,stop_time);
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
	MyTreeTableNode *root = new MyTreeTableNode("Fragments");
	treeTable->setTreeRoot(root, "Fragments for this track");
	MyTreeTableNode *current_root = root;
	this->fragment_set.clear();
	Json::Object response;
	response = interact_zmq(socket,"fragments?");
	Json::Array fragments;
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
		textWidget->setText(name + ": " + sliderWidget->valueText()); //The slider does the casting from double to string!

	}
	WComboBox *comboBox;
	comboBox = dynamic_cast<WComboBox*> (findWidget("trackComboBox"));
	Json::Object response;
	response=interact_zmq(socket,"track?");
	int track;
	track = response.get("current");
std::cout<<"Current track in ui is "<<comboBox->currentIndex()<<std::endl;
std::cout<<"Current track in ear is "<<track<<std::endl;
int row = comboBox->currentIndex();
//if (row!=-1) //Wait till everything is inited.. //TODO
// //Segfault round here{
std::cout<<"Precast"<<std::endl;
WStringListModel *trackmodel = dynamic_cast<WStringListModel*>(comboBox->model());
std::cout<<"Pre int cast"<<std::endl;
int tracknumber =-2;
if (row!=-1)
{
 tracknumber = boost::any_cast<int>(trackmodel->data(trackmodel->index(row,0), Wt::UserRole)); 
}
std::cout<<"postcast"<<std::endl;
	if (track !=tracknumber) 
	{
std::cout<<"Loading markers"<<std::endl;
		loadFragments(socket);
	}	
	else
	{
std::cout<<"Not loading markers"<<std::endl;
	}
	int newtrack = 0;
	int atrack = 0;
	int i = 0;
	for (auto dummy:trackmodel->stringList()) //Loop through the model to find the track. Somehow, we can't get the indeces to be used as index, or something
	{
		atrack =  boost::any_cast<int>(trackmodel->data(trackmodel->index(i,0), Wt::UserRole)); 
		if(atrack == track)
		{
			newtrack = i;
			break;
		}
    		i++;
	}
std::cout<<" newtrack " << std::to_string(newtrack) << " tracknumber  " << std::to_string(tracknumber) << "track  " << std::to_string(track) << std::endl;
	comboBox->setCurrentIndex(newtrack) ;
//}
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





MyTreeTableNode *EarUI::addNode(MyTreeTableNode *parent, WString name, const long start, const long stop ) {
	MyTreeTableNode *node = new MyTreeTableNode(name, 0, parent);
	WContainerWidget *labelArea = node->labelArea();
	WWidget *labelWidget = labelArea->widget(0); //See source of WTreeNode.
	labelArea->removeWidget(labelWidget);
	WInPlaceEdit  *editWidget = new WInPlaceEdit(name);
	
	editWidget->valueChanged().connect(std::bind([=]() {
		node->text = editWidget->text();
	}));
	node->text = editWidget->text();
	labelArea->addWidget(editWidget );
	TimeWidget *startWidget = new TimeWidget();
	startWidget->setTime(start);
	node->setColumnWidget(1, startWidget); 
	Wt::WPushButton *startButton = new WPushButton("|>");
	startButton->clicked().connect(std::bind([=]() {
std::cout<<"Handlign a startbutton click from the markertree"<<std::endl;
		long mystart = start;	
		if(start == -1)
		{ //MARK 
			if (node->childNodes().size() > 0)
			{
				MyTreeTableNode *myttn = dynamic_cast<MyTreeTableNode*> (*(node->childNodes()).begin());
				mystart = dynamic_cast<TimeWidget*>(myttn->columnWidget(1))->time();
			}
			else
			{
				return;
			}
		}
		long startBefore = mystart - beforeSlider->value()*1000;
		interact_zmq(WString("event:stop")); //Will this stop the wierd issues?

		WString command="play:"+std::to_string(startBefore); 
		interact_zmq(command);
	}));
	node->setColumnWidget(3, startButton);

	TimeWidget *stopWidget = new TimeWidget();
	stopWidget->setTime(stop);
	node->setColumnWidget(2, stopWidget);
	this->fragment_set.push_back(node);
	return node;
    }







/*

ZeroMQ handling below.
An interact function can take a socket or not. If provided with a socket, it will be used. If not, one will be opened and closed for this interaction. 
Send and recieve always need a socket.
When sending more than one interaction, preferably create a socket and use that.
WStrings, strings are both handled
*/
//TODO: Replace all the duplication below with some default values for socket pointers. See get_current_track_time as an example;

Json::Object EarUI::interact_zmq(Wt::WString value)
{
	return interact_zmq(value.narrow());
}


Json::Object EarUI::interact_zmq(Json::Object value)//Should use a function template 
{
    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_REQ);
    socket.connect ("tcp://localhost:5555");
//std::cout<<"Connected to ZMQ"<<std::endl;
    Json::Object retval;
    retval = interact_zmq(socket,value);	
    socket.disconnect("tcp://localhost:5555");
//std::cout<<"Disonnected from ZMQ"<<std::endl;
    return retval;
}


Json::Object EarUI::interact_zmq(std::string value)
{   
    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_REQ);
    socket.connect ("tcp://localhost:5555");
//std::cout<<"Connected to ZMQ"<<std::endl;
    Json::Object retval;
    retval = interact_zmq(socket,value);	
    socket.disconnect("tcp://localhost:5555");
//std::cout<<"Disonnected from ZMQ"<<std::endl;
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
//std::cout<<"Recieving and parsing"<<std::endl;	
    Json::parse(recv_zmq(socket),retval);
//std::cout<<"Parsed"<<std::endl;
    return retval;
}
void EarUI::send_zmq(zmq::socket_t &socket, std::string value) 
{
//std::cout<<"Sending stuff: "<<value<<std::endl;
    Json::Value msg_str;
    msg_str = WString( value);
    Json::Array msg_arr;
    msg_arr.push_back(msg_str);
    std::string data = Json::serialize(msg_arr);
    socket.send(data.c_str(),data.size());
}


std::string EarUI::recv_zmq(zmq::socket_t &socket) 
{
//std::cout<<"Recieving stuff"<<std::endl;
    char buffer[MAXSIZE];
    int nbytes = socket.recv(buffer, MAXSIZE);
//std::cout<<"Recieved stuff"<<buffer<<std::endl;
    return std::string(buffer, nbytes);
}


WApplication *createApplication(const WEnvironment& env)
{
  return new EarUI(env);
}







int main(int argc, char **argv)
{

    WString::setDefaultEncoding(UTF8);
  return WRun(argc, argv, &createApplication);
}

