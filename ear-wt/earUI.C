/*
 * Example used: Copyright (C) 2008 Emweb bvba, Heverlee, Belgium.
 *
 * Code copyright Kasper Loopstra, 2017  
 */
#include "earUI.h"
EarUI::EarUI(const Wt::WEnvironment& env) : Wt::WApplication(env)
{
  ui_track_idx = -1;
  max_tags = 0;
    setTitle("Ear interface"); 
//setCssTheme("polished"); //This fixes the columns of the treetable, soimehow
    setTheme(new Wt::WBootstrapTheme());
   
    Wt::WPanel *selectPanel = new Wt::WPanel(root());
    selectPanel->setTitle("Select new track");
    selectPanel->resize(width,Wt::WLength::Auto);
    selectPanel->setCollapsible(true);
    selectPanel->setCollapsed(true); 

//    selectPanel->setObjectName("selectionPanel");
//    Wt::WAnimation animation(Wt::WAnimation::SlideInFromTop,
//			 Wt::WAnimation::EaseOut,
//			 100); //Animation breaks with a VBoxLayout. Maybe report?
 //   selectPanel->setAnimation(animation);

TrackSearchContainer *trackSearchContainer = new TrackSearchContainer(this);

selectPanel->setCentralWidget(trackSearchContainer);





    Wt::WPanel *sliderPanel = new Wt::WPanel(root());
    sliderPanel->setTitle("Change settings");
    sliderPanel->resize(width,Wt::WLength::Auto);
    sliderPanel->setCollapsible(true);
    sliderPanel->setCollapsed(true); 




    Wt::WContainerWidget *inputContainer = new Wt::WContainerWidget(root());
   inputContainer->resize(width,500);


    inputContainer->resize(width,Wt::WLength::Auto); //Auto does not work when in a collapsible box?
    Wt::WVBoxLayout *inputbox = new Wt::WVBoxLayout();
    inputContainer->setLayout(inputbox);
 


    Wt::Json::Object inputs;
    inputs = zmq_conn::interact("inputs?");
    Wt::WContainerWidget *thisInputContainer;
    Wt::WSlider *inputSlider;
    Wt::Json::Array inputSettings;
    Wt::WText *valueText; 
    Wt::WText *titleText;
    Wt::WContainerWidget *inputButtonContainer;
this->log("notice")<<"Making input boxes";
    for(auto input_name:inputs.names())   
    {
this->log("notice")<<"Making input box for "<<input_name;
	thisInputContainer = new Wt::WContainerWidget();
        inputbox->addWidget(thisInputContainer);
	Wt::WVBoxLayout *thisInputBox = new Wt::WVBoxLayout();
	thisInputContainer->setLayout(thisInputBox);
	Wt::WHBoxLayout *textbox = new Wt::WHBoxLayout(); //Todo, make sure we pad the buttons and not expand them
	Wt::WContainerWidget *thisSliderContainer = new Wt::WContainerWidget();
       	thisInputBox->addLayout(textbox);	
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
	
	inputButtonContainer = new Wt::WContainerWidget();
	thisInputBox->addWidget(inputButtonContainer);
	Wt::WHBoxLayout *buttonbox = new Wt::WHBoxLayout();
	buttonbox->addStretch();
        inputButtonContainer->setLayout(buttonbox);
this->log("info")<<"Handling special sliders";
	if (input_name == "before")
	{
		std::vector<int> beforeButtons = {0,3,6,10};
	        for(auto before:beforeButtons)
		{
			Wt::WPushButton *bbutton = new Wt::WPushButton(Wt::WString(std::to_string(before)));
			buttonbox->addWidget(bbutton,0);
			buttonbox->addStretch();
			bbutton->clicked().connect(std::bind([=] ()
			{
				zmq_conn::interact("input:before:"+std::to_string(before));
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
			Wt::WPushButton *sbutton = new Wt::WPushButton(Wt::WString(title));
			buttonbox->addWidget(sbutton,0);
			buttonbox->addStretch();
			sbutton->clicked().connect(std::bind([=] ()
			{
				zmq_conn::interact("input:speed:"+std::to_string(speed));
				updateInputs();
			} ));
			
    	        }
	  
	}
	inputSlider->valueChanged().connect(std::bind([=] ()
        {
        	zmq_conn::interact("input:"+input_name+":"+inputSlider->valueText());
		valueText->setText(inputSlider->valueText());
		updateInputs();
        }));


    }
    Wt::WContainerWidget *posContainer = new Wt::WContainerWidget();
    Wt::WVBoxLayout *posInputBox = new Wt::WVBoxLayout();
    posContainer->setLayout(posInputBox);
    inputbox->addWidget(posContainer);



#ifdef PLOT //ifdef'd out as it crashes on all Debian versions of Wt //TODO add to the boxes as well
    Wt::WContainerWidget *chartContainer = new Wt::WContainerWidget(inputContainer);
	chartText = new  Wt::WText( "chart", chartContainer);
this->log("info")<<"Charting";
 waveformChart = new Wt::Chart::WCartesianChart(chartContainer);
 waveformModel = new Wt::WStandardItemModel(0,3);
 loadWaveform();
 waveformChart->setModel(waveformModel);        // set the model
 waveformChart->setXSeriesColumn(0);    // set the column that holds the categorie
 waveformChart->setType(Wt::Chart::ScatterPlot);
 waveformChart->setAutoLayoutEnabled(true);
 	Wt::Chart::WDataSeries *l = new Wt::Chart::WDataSeries(1, Wt::Chart::LineSeries);
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
#endif //Plot 
this->log("info")<<"Making pos slider";
	Wt::WHBoxLayout *posTextBox = new Wt::WHBoxLayout();
	posInputBox->addLayout(posTextBox);
	Wt::WText *posLabel = new Wt::WText("Position");
	posTextBox->addWidget(posLabel,0);
	TimeWidget *posText = new TimeWidget();
	posTextBox->addWidget(posText,0);
	posText->setTime(0);
    posSlider = new Wt::WSlider();
	posSlider->valueChanged().connect(std::bind([=] ()
    {
this->log("info")<<"Updating pos because posSlider valueChanged";
	zmq_conn::interact("pos:"+posSlider->valueText());
        posText->setTime(posSlider->value());
    }));


    posSlider->resize(width,50); 
    posInputBox->addWidget(posSlider,1);


    Wt::WContainerWidget *posButtonContainer = new Wt::WContainerWidget();
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
	Wt::WPushButton *sbutton = new Wt::WPushButton(Wt::WString(title));
        seekButtonBox->addWidget(sbutton,0);
	seekButtonBox->addStretch();
	sbutton->clicked().connect(std::bind([=] ()
        {
		zmq_conn::interact("pos:"+std::to_string(current_track_time()+seek*1000));
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

    


//    socket->disconnect(zmq_port);


sliderPanel->setCentralWidget(inputContainer);


    Wt::WContainerWidget *markerContainer = new Wt::WContainerWidget(root());
    markerContainer->resize(width,Wt::WLength::Auto);
    Wt::WContainerWidget *fragmentButtonsContainer = new Wt::WContainerWidget(markerContainer);
    
    playPauseButton = new Wt::WPushButton("Play from start",fragmentButtonsContainer);
    playPauseButton->clicked().connect(std::bind([=] ()
    {
	zmq_conn::interact(std::string("event:pause"));
    }));
    Wt::WPushButton *stopButton = new Wt::WPushButton("Stop",fragmentButtonsContainer); 
    stopButton->clicked().connect(std::bind([=] ()
    {
	zmq_conn::interact(std::string("event:stop"));
    }));
    stopButton->setMargin(5, Wt::Left);




    markerTree = new Wt::WTreeTable();
    markerContainer->addWidget(markerTree);
//    markerTree->setObjectName("markertree"); //TODO: Still something wierd with the columns, they are sometimes offset
    markerTree->tree()->setSelectionMode(Wt::ExtendedSelection);
    markerTree->addColumn("",100);
    markerTree->addColumn("",100); 
    markerTree->addColumn("",100); //start
    markerTree->addColumn("",100);  //end
    markerTree->addColumn("",50); //StartButton

 
    Wt::WPushButton *playSelectionButton = new Wt::WPushButton("Play selection" ,fragmentButtonsContainer);     
    playSelectionButton->setMargin(5, Wt::Left);
    playSelectionButton->clicked().connect(std::bind([=] ()
    {	 playSelection(markerTree);
    }));


    Wt::WPushButton *tabButton = new Wt::WPushButton("Group selection >>>>" ,fragmentButtonsContainer);     //This randomises the order
    tabButton->setMargin(5, Wt::Left);
    tabButton->clicked().connect(std::bind([=] ()
    { 
	groupMarkers(markerTree);
    }));	
    
    Wt::WPushButton *untabButton = new Wt::WPushButton("Ungroup selection <<<<<" ,fragmentButtonsContainer);     
    untabButton->setMargin(5, Wt::Left);
    untabButton->clicked().connect(std::bind([=] ()
    {
     //def ungroup(markerTree):
	std::vector< Wt::WTreeNode*> siblings;
	Wt::WTreeNode *parent;
	Wt::WTreeNode *grandparent;
	std::vector< Wt::WTreeNode*> uncles; //My parents siblings
	int index = 0;

	std::set<Wt::WTreeNode*> unSortedselectedNodes = markerTree->tree()->selectedNodes();
	std::vector<Wt::WTreeNode*> selectedNodes ( unSortedselectedNodes.begin(), unSortedselectedNodes.end());
	std::sort(selectedNodes.begin(),selectedNodes.end(), fragmentAbeforeB);
 
	Wt::WTreeNode *firstNode = *selectedNodes.begin();
	Wt::WTreeNode *lastNode = *selectedNodes.rbegin(); //Note, the reverse of the beginning is not the end
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
		index++;
		grandparent->insertChildNode(index,node);
	}
    }));	





    Wt::WPushButton *splitButton = new Wt::WPushButton("Split fragment here", fragmentButtonsContainer);
    splitButton->setMargin(5, Wt::Left);
    splitButton->clicked().connect(std::bind([=] ()
    {	
	long pos = current_track_time();	
//def split(pos, markerTree); 
	for (auto fragmentTTN:children_as_vector(markerTree->tree()->treeRoot()) ) 
	{
		TimeWidget *startW = dynamic_cast<TimeWidget*>(fragmentTTN->columnWidget(1));
		TimeWidget *stopW = dynamic_cast<TimeWidget*>(fragmentTTN->columnWidget(2));
		long start = startW->time();
		long stop = stopW->time();
		if(pos > start and pos < stop)
		{
			//Fragment to split
			stopW->setTime(pos);
			Wt::WTreeNode *my_parent = fragmentTTN->parentNode();
			const std::vector< Wt::WTreeNode * > siblings = my_parent->childNodes();
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
				this->log("warn")<<"Can't find myself when splitting a fragment";
				return;
			}
			MyTreeTableNode *newFragmentTTN = MyTreeTableNode::addNode(0,"New node", pos, stop);
			my_parent->insertChildNode(index+1, newFragmentTTN);
		}
		
	}	
    }	));
    Wt::WPushButton *joinButton = new Wt::WPushButton("Join selected fragments", fragmentButtonsContainer); 
    joinButton->setMargin(5, Wt::Left);
    joinButton->clicked().connect(std::bind([=] ()
    { //def join(markerTree)
//Get selected nodes
	long prevStop = -2;
	std::string newname ="";
	std::set<Wt::WTreeNode*> unSortedselectedNodes =markerTree->tree()->selectedNodes();
	std::vector<Wt::WTreeNode*> selectedNodes ( unSortedselectedNodes.begin(), unSortedselectedNodes.end());
	std::sort(selectedNodes.begin(),selectedNodes.end(), fragmentAbeforeB);
 
	for (auto node:selectedNodes)	{
	//tree returns a tree with tree nodes. We need treetable nodes!
		MyTreeTableNode *fragmentTTN =  dynamic_cast<MyTreeTableNode*>(node);
		TimeWidget *startW = dynamic_cast<TimeWidget*>(fragmentTTN->columnWidget(1));
		TimeWidget *stopW = dynamic_cast<TimeWidget*>(fragmentTTN->columnWidget(2)); //TODO: Make this more flexible and not so dependant on orders. Maybe witch to a node object?
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
        firstNode->editWidget->setText(newname);
	dynamic_cast<TimeWidget*>(firstNode->columnWidget(2))->setTime(prevStop);
	bool first=true;
//Dwlete all others
	for (auto node:selectedNodes)	
	{
		if (not first)
		{
			node->parentNode()->removeChildNode(node);
		}
		first = false;
	}
     }));
 
    Wt::WPushButton *delgrpButton = new Wt::WPushButton("Delete empty group", fragmentButtonsContainer);
    delgrpButton->setMargin(5, Wt::Left);
    delgrpButton->clicked().connect(std::bind([=] ()
    {//def del_empty(markerTree)	
	std::set<Wt::WTreeNode*> selectedNodes = markerTree->tree()->selectedNodes();
	for (auto node:selectedNodes)	
	{
		MyTreeTableNode *fragmentTTN = dynamic_cast<MyTreeTableNode*>(node);
		TimeWidget *startW = dynamic_cast<TimeWidget*>(fragmentTTN->columnWidget(1));
		TimeWidget *stopW = dynamic_cast<TimeWidget*>(fragmentTTN->columnWidget(2));
		long start = startW->time();
		long stop = stopW->time(); 
		if (start != -1  or stop !=-1)
		{
this->log("info")<<"Trying to delete a none-group";
			return;
		}
		if(node->childNodes().size()>0)
		{
this->log("info")<<"Trying to delete a non-empty group";
			return;
		}
		node->parentNode()->removeChildNode(node);
	}
		
	}));

    Wt::WPushButton *savebutton = new Wt::WPushButton("Save fragments", fragmentButtonsContainer);
    savebutton->setMargin(5, Wt::Left);
    savebutton->clicked().connect(std::bind([=] ()
    {	//def save(markerTree)
	Wt::Json::Value fragmentsval = saveFragments(dynamic_cast<MyTreeTableNode*>(markerTree->tree()->treeRoot() ));
	Wt::Json::Array& fragments = fragmentsval; 
	std::string fragstring = Wt::Json::serialize(fragments);
	fragstring = "{ \"fragments\" : "+fragstring + "}";
	zmq_conn::interact(fragstring,true);
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
	zmq::socket_t *socket = zmq_conn::connect();
	bool playing;
	Wt::Json::Object playingj;
	long long track_time = 	current_track_time(socket);
	playingj = zmq_conn::interact(std::string("playing?"),socket);

	zmq_conn::disconnect(socket);
	posSlider->setValue(track_time);
	posText->setTime(track_time);
	
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
			playPauseButton->setText("Play from start");
		}
	}
        mark_current_fragment(track_time); 
   }));


    timer->start();
    updateInputs();


}
void EarUI::mark_current_fragment(long long pos)
{
	for (auto fragmentTTN:children_as_vector(markerTree->tree()->treeRoot()) ) 
	{
		TimeWidget *startW = dynamic_cast<TimeWidget*>(fragmentTTN->columnWidget(1));
		TimeWidget *stopW = dynamic_cast<TimeWidget*>(fragmentTTN->columnWidget(2));
		long start = startW->time();
		long stop = stopW->time();
		if(pos > start and pos < stop)
		{ 
			fragmentTTN->decorationStyle().setBackgroundColor(Wt::WColor(255,0,0)); //TODO: ?Make a proper style, and enlarge the font or something
//TODO: Maybe get the parents too, if the current widget is not shown			

		}
		else
		{

		fragmentTTN->decorationStyle().setBackgroundColor(Wt::WColor(255,255,255)); //TODO: Properly remove the previously added style

		}
	
	}
}
long EarUI::current_track_time( zmq::socket_t *socket )
{

	Wt::Json::Object posj ;
	posj = zmq_conn::interact(std::string("pos?"),socket);
	Wt::Json::Value posjv = posj.get("pos");	
	const long long pos = posjv;
	return pos;
	
}

Wt::Json::Value EarUI::saveFragments(MyTreeTableNode *root)
{ //Does not seem to need an object at all

	Wt::Json::Value retVal = Wt::Json::Value(Wt::Json::ArrayType);
	Wt::Json::Array& ret = retVal; 
	Wt::WString name;
	name = root->text;	
	if(root->childNodes().size()>0)
	{
		ret.push_back(Wt::Json::Value(Wt::WString("group")));
		ret.push_back(Wt::Json::Value(name));
		Wt::Json::Value out_children_value = Wt::Json::Value(Wt::Json::ArrayType);	
		Wt::Json::Array& out_children = out_children_value; //TODO FIXME Ordering?
		for(auto mynode:root->childNodes()) //I wonder, what order do we get these in?
		{
			out_children.push_back(saveFragments(dynamic_cast<MyTreeTableNode*>(mynode)));
		}

		ret.push_back(out_children_value);

	}
	else //TODO add room for annotations
	{
		ret.push_back(Wt::Json::Value(Wt::WString("fragment")));
		ret.push_back(Wt::Json::Value(name));
		TimeWidget *startW = dynamic_cast<TimeWidget*>(root->columnWidget(1));
		TimeWidget *stopW = dynamic_cast<TimeWidget*>(root->columnWidget(2));
		long long start = startW->time();
		long long stop = stopW->time();
		this->log("info")<< "Saving fragment "<<std::to_string(start) << "  "<<std::to_string(stop);
		ret.push_back(Wt::Json::Value(start));
		ret.push_back(Wt::Json::Value(stop));
	}
return retVal;
}
void EarUI::loadGroup(MyTreeTableNode *current_root, Wt::Json::Array fragments)
{ //Recursively add the fragments to the treetable //does not need object at all
this->log("info") <<"Loading fragments";
	for(auto fragmentValue:fragments)
	{
		const Wt::Json::Array& fragment = fragmentValue;
		std::string type = fragment[0];
		Wt::WString name = fragment[1];
this->log("debug") <<"Loading fragment "<<name<<" of type "<<type;
		if (type == "group")
		{
			
			loadGroup( MyTreeTableNode::addNode(current_root,name,-1,-1) ,fragment[2]);	
		}
		else if (type == "fragment")
		{
			long long start_time = fragment[2]; 
			long long stop_time = fragment[3];
			MyTreeTableNode::addNode(current_root,name,start_time,stop_time);
		}
		else
		{
this->log("warn")<<"Node type not understood";
		}
	}

		
}
#ifdef PLOT
void EarUI::loadWaveform()
{
	Wt::Json::Object ret = zmq_conn::interact(std::string("waveform?"));
	Wt::Json::Array waveform = ret.get("waveform");
chartText->setText(WString(std::to_string(waveform.size())));
	waveformModel->removeRows(0,waveformModel->rowCount());
	waveformModel->insertRows(0,waveform.size());
	if(waveform.size() < 10)
	{
	waveformModel->insertRows(0,10);
		waveformTimer->start();
		for(int i=0;i<10+1;i++)
		{
			waveformModel->setData(i,0,i);
			waveformModel->setData(i,1,i);
			waveformModel->setData(i,2,i);
		}
		return;
	}
	int i=0;
	for(auto item: waveform)
	{
		Json::Array foo = item;
		double timestamp = foo[0];
		double l = foo[1];
		double r = foo[2];
		waveformModel->setData(i,0,timestamp);
		waveformModel->setData(i,1,l);
		waveformModel->setData(i,2,r);
		i++;
	}
	
  //waveformChart->setModel(waveformModel);        // set the model
 waveformChart->resize(500,50); //This is needed to render the modlel, but causes a crash on my laptop. This is because of some font issues with libharu. Retry on more recent OS's first to see if that fixes things.
//https://sourceforge.net/p/witty/mailman/message/30272114/ //Won't work on Debian for now
//http://witty-interest.narkive.com/1FcaBlfE/wt-interest-wpdfimage-error-102b
}

#endif


void EarUI::loadFragments(zmq::socket_t *socket)
{
	//needs markerTree
	#ifdef PLOT
	waveformTimer->start();
	#endif
	bool disconnect = false;
	if (socket == 0)
	{
		socket = zmq_conn::connect();
		disconnect = true;
	}
	Wt::WTreeTable *treeTable; 
	treeTable = this->markerTree;
	
	Wt::Json::Object response;

	response = zmq_conn::interact("title?",socket);

	Wt::WString trackname = response.get("title"); 
	MyTreeTableNode *root = new MyTreeTableNode(trackname);
	treeTable->setTreeRoot(root,trackname); 
	MyTreeTableNode *current_root = root;
//	this->fragment_set.clear();
	response = zmq_conn::interact("fragments?",socket);
	Wt::Json::Array fragments;
	fragments = response.get("fragments");

	loadGroup(current_root,fragments);
	
	root->expand();

        TimeWidget *firstW = dynamic_cast<TimeWidget*>((*children_as_vector(markerTree->tree()->treeRoot()) .begin())->columnWidget(1));
        //TimeWidget *lastW = dynamic_cast<TimeWidget*>((*fragment_set.rbegin())->columnWidget(2));
        TimeWidget *lastW = dynamic_cast<TimeWidget*>((*children_as_vector(markerTree->tree()->treeRoot()).rbegin())->columnWidget(2));
        posSlider->setMinimum(firstW->time());
        posSlider->setMaximum(lastW->time());
	if (disconnect)
	{
		zmq_conn::disconnect(socket);
	}	
}



void EarUI::updateInputs()
{	
//this->log("debug")<<"Updating inputs";
    zmq::socket_t *socket = zmq_conn::connect();
	Wt::Json::Object responses;
	responses=zmq_conn::interact(std::string("inputs?"),socket);
	Wt::Json::Array inputSettings;
	Wt::WSlider *sliderWidget;
	Wt::WText *textWidget;
	for(auto name:responses.names())
	{
		inputSettings = responses.get(name);
		sliderWidget = inputSliders[name];
		sliderWidget->setValue(inputSettings[2]);
		textWidget = inputTexts[name];
		textWidget->setText(sliderWidget->valueText());

	}
	Wt::Json::Object response;
	response=zmq_conn::interact("track?",socket);
	
	int server_track_idx = response.get("current");
	if(ui_track_idx != server_track_idx)
	{
		loadFragments(socket);
ui_track_idx = server_track_idx;
	}	


zmq_conn::disconnect(socket);	
		
}






Wt::WApplication *createApplication(const Wt::WEnvironment& env)
{
  return new EarUI(env);
}





int main(int argc, char **argv)
{
    Wt::WString::setDefaultEncoding(Wt::UTF8);
    return Wt::WRun(argc, argv, &createApplication);
}

