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
#include <Wt/WVBoxLayout>
#include <Wt/WHBoxLayout>
#include <Wt/WAnimation>
#include <Wt/WStringListModel>
#include <Wt/WStandardItemModel>
#include <Wt/WComboBox>
#include <Wt/WSelectionBox>
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
#include <Wt/Chart/WCartesianChart>
#include <zmq.hpp>
#include <string>
#include <iostream>
#include <boost/range/adaptor/reversed.hpp>
#include <Wt/WCompositeWidget>
#include <Wt/WTreeTableNode>

#define MAXSIZE 1048576 //Maximum size of ZMQ read buffer used here. 
char const *zmq_port;

class FilteredStringModel : public Wt::WStringListModel
{
 /*
Class to make a stringlistmodel that filters itsself based on filters set previously. Uses zeroMQ to talk to the Python Ear implementation and let Python (fuzzywuzzy) do the actual heavy lifting
*/
    public:
	FilteredStringModel(std::string zmqString = std::string("filteredTracks?"), WObject *parent=0) : Wt::WStringListModel(parent)
{
searchString = "";
this->zmqString = zmqString;
};
	std::vector<Wt::WString> musts;
	std::vector<Wt::WString> mays;
	Wt::WString searchString;
	void update();
	Wt::WStringListModel(parent);
    private:
	Wt::WString zmqString;

};
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



    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_REQ);
 
    socket.connect (zmq_port);
    Wt::Json::Object response;
	socket.send(retval.c_str(),retval.size());	
	char buffer[MAXSIZE];
    int nbytes = socket.recv(buffer, MAXSIZE);
      Wt::Json::parse(std::string(buffer, nbytes),response);
	Wt::Json::Array options = response.get("options");
    socket.disconnect(zmq_port);
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




class MyTreeTableNode : public Wt::WTreeTableNode
{
	public:
		MyTreeTableNode(const Wt::WString& labelText, Wt::WIconPair *labelIcon = 0,  Wt::WTreeTableNode *parentNode = 0) :  
		Wt::WTreeTableNode( labelText,  labelIcon , parentNode )
		{
			
		}
	using Wt::WTreeTableNode::labelArea;
	Wt::WString text;		
	Wt::WInPlaceEdit*  editWidget;

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
		this->_time = time; //Just moved this outside the if. NEEDS TESTING! --30-3-2017
	if(time != -1)
	{
		bool negative = false;
		if (time < 0)
		{
			negative = true;
			time *= -1; 
		}
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


std::vector<MyTreeTableNode*> children_as_vector(MyTreeTableNode *root)
{
	std::vector<MyTreeTableNode*> retval;
	std::vector<WTreeNode*> children = root->childNodes();
	for(auto node:children)
	{
		retval.push_back(dynamic_cast<MyTreeTableNode*>(node));
		if(node->childNodes().size()>0)

		{	std::vector<MyTreeTableNode*> grandchildren = children_as_vector(dynamic_cast<MyTreeTableNode*>(node));
			retval.insert(retval.end(),grandchildren.begin(),grandchildren.end()); 
		}
			
	}
	return retval;

}

std::vector<MyTreeTableNode*> ancestors_as_vector(MyTreeTableNode *child)
{
	std::vector<MyTreeTableNode*> retval;
	MyTreeTableNode* currentGen = child;
	while(currentGen != child->tree()->treeRoot())
	{
		retval.push_back(currentGen);
		currentGen = dynamic_cast<MyTreeTableNode*>(currentGen->parentNode());
	}
	return retval;
}

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
  Wt::WLength width = Wt::WLength::Auto; 
  std::vector<MyTreeTableNode*> fragment_set;
  void clicked(WPushButton* source );
  void loadFragments(zmq::socket_t &socket);
  void loadFragments();	
  void loadWaveform();
  WTimer *waveformTimer;
  void updateInputs();
  void loadGroup(MyTreeTableNode *current_root, Json::Array fragments);
  void mark_current_fragment(long long track_time);
  long current_track_time( zmq::socket_t *socket  =0  );
  std::map<std::string, Wt::WText*> inputTexts;
  std::map<std::string, Wt::WSlider*> inputSliders;

  int ui_track_idx = -1;
  WStandardItemModel *waveformModel;
  Chart::WCartesianChart *waveformChart;
  WText *chartText;
  WSlider *posSlider;
  MyTreeTableNode *addNode(MyTreeTableNode *parent, WString name, const long start, const long stop );
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
    socket.connect (zmq_port); //TODO: make host and port configurable
/*
ZMQ should connect and disconnect after every set of actions to make room for another interface. Saves working on lots of listeners and the Python interface doesn't have to know who's talking
*/
   


    Wt::WPanel *selectPanel = new Wt::WPanel(root());
selectPanel->setTitle("Select new track");
	selectPanel->resize(width,Wt::WLength::Auto);
    selectPanel->setObjectName("selectionPanel");
    selectPanel->setCollapsible(true);
//    Wt::WAnimation animation(Wt::WAnimation::SlideInFromTop,
//			 Wt::WAnimation::EaseOut,
//			 100); //Animation breaks with a VBoxLayout. Maybe report?
 //   selectPanel->setAnimation(animation);
    selectPanel->setCollapsed(true); 



Wt::WContainerWidget *trackSearchContainer = new Wt::WContainerWidget(root()); 
trackSearchContainer->resize(width,500);
//Make this a GridLayout and add a second column showing the track settings TODO

Wt::WVBoxLayout *vbox = new Wt::WVBoxLayout();
trackSearchContainer->setLayout(vbox);


Wt::WLineEdit *searchBox = new Wt::WLineEdit(); 
vbox->addWidget(searchBox);
searchBox->setPlaceholderText("Type to search");
Wt::WSelectionBox *trackSelectionBox = new Wt::WSelectionBox();
vbox->addWidget(trackSelectionBox,1);
FilteredStringModel *trackModel = new FilteredStringModel();
searchBox->textInput().connect(std::bind([=] ()
{
	trackModel->searchString = searchBox->text();
	std::cout<<"Updating model"<<std::endl;
	trackModel->update();
	trackSelectionBox->setModel(trackModel);
std::cout<<"Updated model"<<std::endl;
}));
trackModel->update();
trackSelectionBox->setModel(trackModel);
trackSelectionBox->setSelectionMode(Wt::SingleSelection);
Wt::WPushButton *selectButton = new Wt::WPushButton("Select track");
vbox->addWidget(selectButton);
selectButton->clicked().connect(std::bind([=] ()
{
	int row = trackSelectionBox->currentIndex();
	int tracknumber = boost::any_cast<int>(trackModel->data(trackModel->index(row,0),Wt::UserRole));
	interact_zmq(WString("track:"+std::to_string(tracknumber)));
	updateInputs();
}));

    Wt::WContainerWidget *filterContainer = new WContainerWidget(trackSearchContainer);
vbox->addWidget(filterContainer);
    Wt::WContainerWidget *searchContainer = new WContainerWidget(filterContainer);
    Wt::WContainerWidget *filtersContainer = new WContainerWidget(filterContainer);
    Wt::WLineEdit *filterbox = new Wt::WLineEdit(searchContainer);
    filterbox->setPlaceholderText("Filter"); 
    Wt::WPushButton *addFilter = new Wt::WPushButton("Add",searchContainer);
    addFilter->clicked().connect(std::bind([=] ()
    {
	Wt::WContainerWidget *thisFilter = new Wt::WContainerWidget(filtersContainer); 
	WString filterName = filterbox->text();
	new WText(filterName,thisFilter);
	Wt::WPushButton *removeButton = new WPushButton("X",thisFilter);
		removeButton->clicked().connect(std::bind([=] ()
		{
			filtersContainer->removeWidget(thisFilter);
			thisFilter->clear();
			std::vector<WString>::iterator pos = std::find(trackModel->musts.begin(), trackModel->musts.end(), filterName);
			if(pos!=trackModel->musts.end())
			{
				trackModel->musts.erase(pos);
				trackModel->update();
			}
		}));
        trackModel->musts.push_back(filterName);  
				trackModel->update();
	filterbox->setText("");
	filterbox->setPlaceholderText("Filter"); //TODO: Autofill/autocomplete
     }));
    
selectPanel->setCentralWidget(trackSearchContainer);




    Wt::WContainerWidget *inputContainer = new Wt::WContainerWidget(root());
    inputContainer->resize(width,Wt::WLength::Auto);
    Wt::WVBoxLayout *inputbox = new Wt::WVBoxLayout();
    inputContainer->setLayout(inputbox);
 
    Json::Object inputs;
    inputs = interact_zmq(socket,"inputs?");
    Wt::WContainerWidget *thisInputContainer;
    Wt::WSlider *inputSlider;
    Json::Array inputSettings;
    Wt::WText *valueText; 
    Wt::WText *titleText;
    WContainerWidget *inputButtonContainer;
std::cout<<"Making input boxes for "<<std::endl;
    for(auto input_name:inputs.names())   
    {
std::cout<<"Making input box for "<<input_name<<std::endl;
	thisInputContainer = new Wt::WContainerWidget();
        inputbox->addWidget(thisInputContainer);
	Wt::WVBoxLayout *thisInputBox = new Wt::WVBoxLayout();
	thisInputContainer->setLayout(thisInputBox);
	Wt::WHBoxLayout *textbox = new Wt::WHBoxLayout(); //Todo, make sure we pad the buttons and not expand them
	WContainerWidget *thisSliderContainer = new WContainerWidget();
       	thisInputBox->addLayout(textbox);	
std::cout<<"It's in the box"<<std::endl;	
	titleText = new Wt::WText(input_name);
	textbox->addWidget(titleText,0);
	valueText = new Wt::WText("");
	textbox->addWidget(valueText,0);	
	textbox->addStretch();
        inputTexts[input_name] = valueText;
//	valueText->setObjectName("inputText:"+input_name); //Soo, setting this makes the widget disappear, or at least act up. Report?

	inputSettings = inputs.get(input_name);
 	inputSlider = new Wt::WSlider(); 
	thisSliderContainer->addWidget(inputSlider);
	inputSlider->resize(width,50); 
//	inputSlider->resize( Wt::WLength(100,Wt::WLength::Unit::Percentage) ,50); //This kind of works. We get a huge slider (which si good) but the ticks are all in 100px or something on one side, and so is the min/max
 //inputSlider->setNativeControl(true); //If we use this, and move the slider, the program freezes
        inputSliders[input_name] =inputSlider;
	int min, max;
	min = inputSettings[0];
	max = inputSettings[1]; 
        inputSlider->setMinimum(min); 
	
        inputSlider->setMaximum(max); 
	inputSlider->setTickInterval( (max-min)/6);
        inputSlider->setValue(inputSettings[2]); 
	inputSlider->setTickPosition(Wt::WSlider::TicksAbove);
	thisInputBox->addWidget(inputSlider,1); //So, if we put it in iwthout a container it won't appear if it has an objectName. If we put it in a container, it appears, but resizing is wierd. If we put a 100% the slider is there but the ticks and the min/max are all in one corner, if we put it 500px wide everything works except it's a hard limit. I'm debugging on a 4k screen and it should work on a phone. Now what? //Turns out, if you don't set the objectName, it all works as advertised. TODO: Make a minimal example and submit the bug
	
std::cout<<"Slider made"<<std::endl;	
std::cout<<"Line"<<std::endl;
	inputButtonContainer = new WContainerWidget();
	thisInputBox->addWidget(inputButtonContainer);
	WHBoxLayout *buttonbox = new Wt::WHBoxLayout();
	buttonbox->addStretch();
        inputButtonContainer->setLayout(buttonbox);
std::cout<<"Who's special?"<<std::endl;
	if (input_name == "before")
	{
std::cout<<" Before is special"<<std::endl;
		std::vector<int> beforeButtons = {0,3,6,10};
	       for(auto before:beforeButtons)
		{
			WPushButton *bbutton = new WPushButton(WString(std::to_string(before)));
			buttonbox->addWidget(bbutton,0);
	buttonbox->addStretch();
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
		std::vector<int> speedButtons = {50,75,90,100,110,120,150,175,200};
	       for(auto speed:speedButtons)
		{
			std::string title = std::to_string(speed);
			title += "%";
			WPushButton *sbutton = new WPushButton(WString(title));
			buttonbox->addWidget(sbutton,0);
	buttonbox->addStretch();
			sbutton->clicked().connect(std::bind([=] ()
			{
				interact_zmq("input:speed:"+std::to_string(speed));
				updateInputs();
			} ));
			
    	        }
	  
	}
std::cout<<"Connecting the valueChanged"<<std::endl;
	inputSlider->valueChanged().connect(std::bind([=] ()
        {
        	interact_zmq("input:"+input_name+":"+inputSlider->valueText());
		valueText->setText(inputSlider->valueText());
		updateInputs();
        }));


    }
 std::cout<<"Done making new sliders"<<std::endl;
    WContainerWidget *posContainer = new WContainerWidget();
    Wt::WVBoxLayout *posInputBox = new Wt::WVBoxLayout();
    posContainer->setLayout(posInputBox);
    inputbox->addWidget(posContainer);

#ifdef PLOT //ifdef'd out as it crashes on all Debian versions of Wt //TODO add to the boxes as well
    WContainerWidget *chartContainer = new WContainerWidget(inputContainer);
	chartText = new  WText( "chart", chartContainer);
std::cout<<"Charting"<<std::endl;
 waveformChart = new Chart::WCartesianChart(chartContainer);
 waveformModel = new WStandardItemModel(0,3);
 loadWaveform();
 waveformChart->setModel(waveformModel);        // set the model
std::cout<< "Got a model"<<std::endl;
 waveformChart->setXSeriesColumn(0);    // set the column that holds the categorie
 waveformChart->setType(Wt::Chart::ScatterPlot);
 waveformChart->setAutoLayoutEnabled(true);
 	Chart::WDataSeries *l = new Chart::WDataSeries(1, Wt::Chart::LineSeries);
    l->setShadow(WShadow(3, 3, WColor(0, 0, 0, 127), 3));
 waveformChart->addSeries(*l);
/*Chart::WDataSeries *r = new Chart::WDataSeries(2, Wt::Chart::LineSeries);
    r->setShadow(WShadow(3, 3, WColor(0, 0, 0, 127), 3));
   waveformChart->addSeries(*r);*/
   waveformChart->resize(width,100);
	waveformTimer = new WTimer();
	waveformTimer->setSingleShot(true);
	waveformTimer->setInterval(50);
	waveformTimer->timeout().connect( std::bind([=] ()
	{
		loadWaveform();
	}));
	waveformTimer->start();
std::cout<<"Done charting "<<std::endl;
#endif //Plot 
std::cout<<"Making pos slider"<<std::endl;
	Wt::WHBoxLayout *posTextBox = new Wt::WHBoxLayout();
	posInputBox->addLayout(posTextBox);
	Wt::WText *posLabel = new Wt::WText("Position");
	posTextBox->addWidget(posLabel,0);
	TimeWidget *posText = new TimeWidget();
	posTextBox->addWidget(posText,0);
	posText->setTime(0);
    posSlider = new WSlider();
	posSlider->valueChanged().connect(std::bind([=] ()
    {
std::cout<<"interacting pos"<<std::endl;
	interact_zmq("pos:"+posSlider->valueText());
std::cout<<"interacted pos"<<std::endl;
        posText->setTime(posSlider->value());
    }));


    posSlider->resize(width,50); 
    posInputBox->addWidget(posSlider,1);


std::cout<<"Making pos buttons"<<std::endl; 
    WContainerWidget *posButtonContainer = new WContainerWidget();
    posInputBox->addWidget(posButtonContainer);
    Wt::WHBoxLayout *seekButtonBox = new Wt::WHBoxLayout();
    posButtonContainer->setLayout(seekButtonBox);
	seekButtonBox->addStretch();
    std::vector<int> seekButtons = {-10,-5,-1,1,5,10};
    for(auto seek:seekButtons)
    {
	std::string title = std::to_string(seek);
	if (seek>0)
	{
		title = "+" + title;
	}
	title += "s";
	WPushButton *sbutton = new WPushButton(WString(title));
        seekButtonBox->addWidget(sbutton,0);
	seekButtonBox->addStretch();
	sbutton->clicked().connect(std::bind([=] ()
        {
		interact_zmq("pos:"+std::to_string(current_track_time()+seek*1000));
        } ));
	
    }
    posSlider->resize(width,50);
    posSlider->setTickInterval(60000); //One minute in ms
    posSlider->setTickPosition(Wt::WSlider::TicksAbove);/*
*    posSlider->sliderMoved().connect(std::bind([=] (int v) //TODO:Implement this. I'm not sure why it does not currently work, as it looks like Wt casts the value to a static int. 
//Also TODO, use this to stop the slider from being moved by the udpating timer if someone is dragging it
    { 
	posText->setTime(v); //So we can move and see where we'll end up before releasing
    }));*/

    

std::cout<<"Done making pos slider"<<std::endl;

  
    socket.disconnect(zmq_port);

    Wt::WContainerWidget *markerContainer = new Wt::WContainerWidget(root());
    markerContainer->resize(width,Wt::WLength::Auto);
    Wt::WContainerWidget *fragmentButtonsContainer = new Wt::WContainerWidget(markerContainer);
    
    WTreeTable *markerTree = new WTreeTable();
    markerContainer->addWidget(markerTree);
    markerTree->setObjectName("markertree"); //TODO: Still something wierd with the columns, they are sometimes offset /TODO: Remove the setObjectName here as well
    markerTree->tree()->setSelectionMode(Wt::ExtendedSelection);
    markerTree->addColumn("Start time",100);
    markerTree->addColumn("End time",100); 
    markerTree->addColumn("Play from here",20); //StartButton
    playPauseButton = new WPushButton("Play",fragmentButtonsContainer);
    playPauseButton->clicked().connect(std::bind([=] ()
    {
	interact_zmq(std::string("event:pause"));
    }));
    WPushButton *stopButton = new WPushButton("Stop",fragmentButtonsContainer); 
    stopButton->clicked().connect(std::bind([=] ()
    {
	interact_zmq(std::string("event:stop"));
    }));
    stopButton->setMargin(5, Left);
 
    WPushButton *playSelectionButton = new WPushButton("Play selection" ,fragmentButtonsContainer);     
    playSelectionButton->setMargin(5, Left);
    playSelectionButton->clicked().connect(std::bind([=] ()
    {	
	std::string ret = "{\"play\": [";	
	bool first = true;
	for(auto node:children_as_vector(dynamic_cast<MyTreeTableNode*>(markerTree->tree()->treeRoot())))
	{
		TimeWidget *startW = dynamic_cast<TimeWidget*>(node->columnWidget(1));
		TimeWidget *stopW = dynamic_cast<TimeWidget*>(node->columnWidget(2));
		long start = startW->time();
		long stop = stopW->time();
		bool selected = markerTree->tree()->isSelected(node);
		if (start == -1 or stop ==-1)
		{
			continue;
		}
		for(auto parent:ancestors_as_vector(node))
		{
			if(markerTree->tree()->isSelected(parent) and not parent->isExpanded())
			{
				selected = true;
				break;
			}
		}
		if(selected)
		{
	
			if (first)
			{
				start -= beforeSlider->value()*1000;
				first = false;
			}
			else
			{
				ret+=" , ";
			}
			ret += "["+std::to_string(start)+" , "+	std::to_string(stop)+"]\n";
			
		}
	}
	ret+="]}";
std::cout<<ret<<std::endl;
		   zmq::context_t context (1);
		   zmq::socket_t socket (context, ZMQ_REQ);
		   socket.connect (zmq_port);
		   socket.send(ret.c_str(),ret.size());
		   recv_zmq(socket); //Just dummy anyway
		   socket.disconnect(zmq_port);
		

	
    }));


//TODO: Split out the tab and untab functions so we can call them with the keyboard
    WPushButton *tabButton = new WPushButton("Group selection >>>>" ,fragmentButtonsContainer);     
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
    
    WPushButton *untabButton = new WPushButton("Ungroup selection <<<<<" ,fragmentButtonsContainer);     
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





    WPushButton *splitButton = new WPushButton("Split fragment here", fragmentButtonsContainer);
    splitButton->setMargin(5, Left);
    splitButton->clicked().connect(std::bind([=] ()
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
    WPushButton *joinButton = new WPushButton("Join selected fragments", fragmentButtonsContainer);
    joinButton->setMargin(5, Left);
    joinButton->clicked().connect(std::bind([=] ()
    {
//Get selected nodes
	long prevStop = -2;
	std::string newname ="";
	std::set<WTreeNode*> selectedNodes =markerTree->tree()->selectedNodes();
	for (auto node:selectedNodes)	{
	//tree returns a tree with tree nodes. We need treetable nodes!
		MyTreeTableNode *fragmentTTN =  dynamic_cast<MyTreeTableNode*>(node);
		TimeWidget *startW = dynamic_cast<TimeWidget*>(fragmentTTN->columnWidget(1));
		TimeWidget *stopW = dynamic_cast<TimeWidget*>(fragmentTTN->columnWidget(2));
		long start = startW->time();
		long stop = stopW->time();
		if (prevStop !=-2)
		{
			if (prevStop != start)
			{
				return; //Not contiguous
			}
		}
		prevStop = stop;
		newname +=" "+ fragmentTTN->text.narrow();
	}
//Change the first node
	MyTreeTableNode *firstNode = dynamic_cast<MyTreeTableNode*>(*selectedNodes.begin());
        firstNode->editWidget->setText(newname) ;//Will this work? //It doesn't.. The one below does though
	dynamic_cast<TimeWidget*>(firstNode->columnWidget(2))->setTime(prevStop);
	bool first=true;
//Dlete all others
	for (auto node:selectedNodes)	
	{
		if (not first)
		{
			node->parentNode()->removeChildNode(node);
		}
		first = false;
	}
     }));
 
    WPushButton *delgrpButton = new WPushButton("Delete empty group", fragmentButtonsContainer);
    delgrpButton->setMargin(5, Left);
    delgrpButton->clicked().connect(std::bind([=] ()
    {	
	std::set<WTreeNode*> selectedNodes =markerTree->tree()->selectedNodes();
	for (auto node:selectedNodes)	
	{
		MyTreeTableNode *fragmentTTN =  dynamic_cast<MyTreeTableNode*>(node);
		TimeWidget *startW = dynamic_cast<TimeWidget*>(fragmentTTN->columnWidget(1));
		TimeWidget *stopW = dynamic_cast<TimeWidget*>(fragmentTTN->columnWidget(2));
		long start = startW->time();
		long stop = stopW->time(); 
		if (start != -1  or stop !=-1)
		{
std::cout<<"TRying to delete a none-group"<<std::endl;
			return;
		}
		if(node->childNodes().size()>0)
		{
std::cout<<"Trying to delete a non-empty group"<<std::endl;
			return;
		}
		node->parentNode()->removeChildNode(node);
	}
		
	}));

    WPushButton *savebutton = new WPushButton("Save fragments", fragmentButtonsContainer);
    savebutton->setMargin(5, Left);
    savebutton->clicked().connect(std::bind([=] ()
    {	
	Json::Value fragmentsval = saveFragments(dynamic_cast<MyTreeTableNode*>(markerTree->tree()->treeRoot() ));
	Json::Array& fragments = fragmentsval; 
	std::string fragstring = Json::serialize(fragments);
	fragstring = "{ \"fragments\" : "+fragstring + "}";
			   zmq::context_t context (1);
		   zmq::socket_t socket (context, ZMQ_REQ);
		   socket.connect (zmq_port);
		   socket.send(fragstring.c_str(),fragstring.size());
		   recv_zmq(socket); //Just dummy anyway
		   socket.disconnect(zmq_port);
    }));	


   Wt::WTimer *inputtimer = new Wt::WTimer();  
   inputtimer->setInterval(2500);
   inputtimer->timeout().connect(std::bind([=] ()
   {
	updateInputs();
   }));
   inputtimer->start();

   Wt::WTimer *timer = new Wt::WTimer();  
   timer->setInterval(100);
   timer->timeout().connect(std::bind([=] ()
   {
	zmq::context_t context (1);
	zmq::socket_t socket (context, ZMQ_REQ);
	socket.connect (zmq_port);

	long long track_time = 	EarUI::current_track_time(&socket);
	posSlider->setValue(track_time);
	posText->setTime(track_time);
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
		if(track_time > 0)
		{
			playPauseButton->setText("Continue");
		}
		else
		{
			playPauseButton->setText("Play");
		}
	}
        socket.disconnect(zmq_port);
        mark_current_fragment(track_time); 
   }));

    timer->start();
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

Json::Value EarUI::saveFragments(MyTreeTableNode *root)
{

	Json::Value retVal = Json::Value(Json::ArrayType);
	Json::Array& ret = retVal; 
	WString name;
	name = root->text;	
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
#ifdef PLOT
void EarUI::loadWaveform()
{
std::cout<<"Loading waveform"<<std::endl;
	Json::Object ret = interact_zmq(std::string("waveform?"));
std::cout<<"Extravting waveform"<<std::endl;
	Json::Array waveform = ret.get("waveform");
chartText->setText(WString(std::to_string(waveform.size())));
std::cout<<"Got a waveform"<<std::endl;
	waveformModel->removeRows(0,waveformModel->rowCount());
	waveformModel->insertRows(0,waveform.size());
	if(waveform.size() < 10)
	{
std::cout<<"WAves too short"<<std::endl;
	waveformModel->insertRows(0,10);
		waveformTimer->start();
		for(int i=0;i<10+1;i++)
		{
			waveformModel->setData(i,0,i);
			waveformModel->setData(i,1,i);
			waveformModel->setData(i,2,i);
		}
std::cout<<"Added dummy data"<<std::endl;
		return;
	}
	int i=0;
std::cout<<"Filling model"<<std::endl;
	for(auto item: waveform)
	{
		Json::Array foo = item;
		double timestamp = foo[0];
		double l = foo[1];
		double r = foo[2];
std::cout<<std::to_string(i) <<" "<< std::to_string(timestamp) <<" "<< std::to_string(l) <<std::endl;
		waveformModel->setData(i,0,timestamp);
		waveformModel->setData(i,1,l);
		waveformModel->setData(i,2,r);
		i++;
	}
std::cout<<"Done filling model"<<std::endl;
	
  //waveformChart->setModel(waveformModel);        // set the model
 waveformChart->resize(500,50); //This is needed to render the modlel, but causes a crash on my laptop. This is because of some font issues with libharu. Retry on more recent OS's first to see if that fixes things.
//https://sourceforge.net/p/witty/mailman/message/30272114/ //Won't work on Debian for now
//http://witty-interest.narkive.com/1FcaBlfE/wt-interest-wpdfimage-error-102b
std::cout<<"Set a model"<<std::endl;
}

#endif



void EarUI::loadFragments()
{
    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_REQ);
    socket.connect (zmq_port);
	loadFragments(socket);
    socket.disconnect(zmq_port);
#ifdef PLOT
waveformTimer->start();
#endif

}

void EarUI::loadFragments(zmq::socket_t &socket)
{

	Wt::WTreeTable *treeTable; 
	treeTable = dynamic_cast<WTreeTable*> (findWidget("markertree"));
	
	Json::Object response;

	response = interact_zmq(socket,"title?");

	WString trackname = response.get("title"); 
	MyTreeTableNode *root = new MyTreeTableNode(trackname);
	treeTable->setTreeRoot(root,trackname); 
	MyTreeTableNode *current_root = root;
	this->fragment_set.clear();
	response = interact_zmq(socket,"fragments?");
	Json::Array fragments;
	fragments = response.get("fragments");

	loadGroup(current_root,fragments);
	
	root->expand();

                TimeWidget *firstW = dynamic_cast<TimeWidget*>((*fragment_set.begin())->columnWidget(1));
                TimeWidget *lastW = dynamic_cast<TimeWidget*>((*fragment_set.rbegin())->columnWidget(2));
                posSlider->setMinimum(firstW->time());
                posSlider->setMaximum(lastW->time());
	
}



void EarUI::updateInputs()
{	
 std::cout<<"Updating inpouts"<<std::endl;
    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_REQ);
    socket.connect (zmq_port);
	Json::Object responses;
	responses=interact_zmq(socket,std::string("inputs?"));
	Json::Array inputSettings;
	WSlider *sliderWidget;
	WText *textWidget;
	for(auto name:responses.names())
	{
		inputSettings = responses.get(name);
		sliderWidget = inputSliders[name];
		sliderWidget->setValue(inputSettings[2]);
		textWidget = inputTexts[name];
		textWidget->setText(sliderWidget->valueText());
 std::cout<<"Updating texts"<<std::endl;

	}
	Json::Object response;
	response=interact_zmq(socket,"track?");
	
	int server_track_idx = response.get("current");
	if(ui_track_idx != server_track_idx)
	{
std::cout<<"UI track "<<ui_track_idx<<" server track "<<server_track_idx<<std::endl;
		loadFragments(socket);
ui_track_idx = server_track_idx;
	}	

 std::cout<<"Updated"<<std::endl;

	
    socket.disconnect(zmq_port);
		
}





MyTreeTableNode *EarUI::addNode(MyTreeTableNode *parent, WString name, const long start, const long stop ) {
	MyTreeTableNode *node = new MyTreeTableNode(name, 0, parent);
	WContainerWidget *labelArea = node->labelArea();
	WWidget *labelWidget = labelArea->widget(0); //See source of WTreeNode.
	labelArea->removeWidget(labelWidget);
	node->editWidget = new WInPlaceEdit(name);
	
	node->editWidget->valueChanged().connect(std::bind([=]() {
		node->text = node->editWidget->text();
	}));
	node->text = node->editWidget->text();
	labelArea->addWidget(node->editWidget );
	TimeWidget *startWidget = new TimeWidget();
	startWidget->setTime(start);
	node->setColumnWidget(1, startWidget); 
//todo: add doubleclick trick to allow modal edit
	Wt::WPushButton *startButton = new WPushButton("|>");
	startButton->clicked().connect(std::bind([=]() {
std::cout<<"Handlign a startbutton click from the markertree"<<std::endl;
		long mystart = start;	
		if(start == -1)
		{ 
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
		interact_zmq(WString("event:stop")); //Probably needed to help stop the track from stopping the middle of a play

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
    socket.connect (zmq_port);
    Json::Object retval;
    retval = interact_zmq(socket,value);	
    socket.disconnect(zmq_port);
    return retval;
}


Json::Object EarUI::interact_zmq(std::string value)
{   
    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_REQ);
    socket.connect (zmq_port);
    Json::Object retval;
    retval = interact_zmq(socket,value);	
    socket.disconnect(zmq_port);
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
    Json::parse(recv_zmq(socket),retval);
    return retval;
}
void EarUI::send_zmq(zmq::socket_t &socket, std::string value) 
{
    Json::Value msg_str;
    msg_str = WString( value);
    Json::Array msg_arr;
    msg_arr.push_back(msg_str);
    std::string data = Json::serialize(msg_arr);
    socket.send(data.c_str(),data.size());
}


std::string EarUI::recv_zmq(zmq::socket_t &socket) 
{
    char buffer[MAXSIZE];
    int nbytes = socket.recv(buffer, MAXSIZE);
    return std::string(buffer, nbytes);
}


WApplication *createApplication(const WEnvironment& env)
{
  return new EarUI(env);
}







int main(int argc, char **argv)
{
    zmq_port = getenv("ZMQ_PORT");
    WString::setDefaultEncoding(UTF8);
    return WRun(argc, argv, &createApplication);
}

