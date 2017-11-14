#include "fragmentTree.h"


MyTreeTableNode::MyTreeTableNode(const Wt::WString& labelText, Wt::WIconPair *labelIcon,  Wt::WTreeTableNode *parentNode) :  Wt::WTreeTableNode( labelText,  labelIcon , parentNode )
{

}


MyTreeTableNode *MyTreeTableNode::addNode(MyTreeTableNode *parent, Wt::WString name, const long start, const long stop, bool mini ) {
	MyTreeTableNode *node = new MyTreeTableNode(name, 0, parent);
	Wt::WContainerWidget *labelArea = node->labelArea();
	Wt::WWidget *labelWidget = labelArea->widget(0); //See source of WTreeNode.
	labelArea->removeWidget(labelWidget);
	if (mini)
	{
		node->startButton = new Wt::WPushButton(name);
		labelArea->addWidget(node->startButton);
	}
	else
	{
		node->editWidget = new Wt::WInPlaceEdit(name);
		node->editWidget->valueChanged().connect(std::bind([=]() {
			node->text = node->editWidget->text();
		}));
	
		node->text = node->editWidget->text();
		labelArea->addWidget(node->editWidget );
	
		TimeWidget *startWidget = new TimeWidget();
		startWidget->setTime(start);
		node->setColumnWidget(1, startWidget); 
		node->startButton = new Wt::WPushButton("|>");
	}
//todo: add doubleclick trick to allow modal edit
	node->startButton->clicked().connect(std::bind([=]() { 
		if(start == -1)
		{	//We've clicked the startbutton on a group, so we need to find the first non-group widget. 
			if (node->childNodes().size() > 0)
			{
 				//So, there are children.. In this case take the first child and pretend we've clicked that startButton. It'll be recursive. 
				MyTreeTableNode *firstChild = dynamic_cast<MyTreeTableNode*> (*(node->childNodes()).begin());
				return firstChild->startButton->clicked().emit(Wt::WMouseEvent());
			}
			else
			{ //Where in a childless group, so we cannot play anything!
				return;
			}

		}
		std::cout <<"It's not a group: "<<start<<std::endl;
		Wt::Json::Object jStartBefore = zmq_conn::interact("inputs?"); 
		Wt::Json::Array aStartBefore = jStartBefore.get("before");
		signed long long startBefore = aStartBefore[2];
		zmq_conn::interact(Wt::WString("event:stop")); //Probably needed to help stop the track from stopping the middle of a play

		Wt::WString command="play:"+std::to_string(start - startBefore * 1000); 
		zmq_conn::interact(command);
	}));
	if(mini)
	{
	}
	else
	{
		node->setColumnWidget(3, node->startButton);
		TimeWidget *stopWidget = new TimeWidget();
		stopWidget->setTime(stop);
		node->setColumnWidget(2, stopWidget);
	}
	return node;
    }





bool fragmentAbeforeB(Wt::WTreeNode* A, Wt::WTreeNode* B) //Needs renaming)
{
	
		TimeWidget *startAt = dynamic_cast<TimeWidget*>( dynamic_cast<MyTreeTableNode*>(A)->columnWidget(1));
		TimeWidget *stopAt = dynamic_cast<TimeWidget*>( dynamic_cast<MyTreeTableNode*>(A)->columnWidget(2));
		long startA = startAt->time();
		long stopA = stopAt->time();

		TimeWidget *startBt = dynamic_cast<TimeWidget*>( dynamic_cast<MyTreeTableNode*>(B)->columnWidget(1));
		TimeWidget *stopBt = dynamic_cast<TimeWidget*>(  dynamic_cast<MyTreeTableNode*>(B)->columnWidget(2));
		long startB = startBt->time();
		long stopB = stopBt->time();
		//Strictly, this does not mean that A<B means that B>A, because of stuff. See what breaks? --KRL 11072017 FIXME
	if(startA < startB)
	{
		if(stopA<stopB)
		{
			return true;
		}
	}
	return false;
	


}



void playSelection(Wt::WTreeTable *markerTree)
{

	std::string ret = "{\"play\": [";	
	bool first = true;
	for(auto node:children_as_vector(dynamic_cast<MyTreeTableNode*>(markerTree->tree()->treeRoot())))
	{
		TimeWidget *startW = dynamic_cast<TimeWidget*>(node->columnWidget(1));
		TimeWidget *stopW = dynamic_cast<TimeWidget*>(node->columnWidget(2)); //Todo: Find a way to make this more flexible to re-ordering
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
				Wt::Json::Object jStartBefore = zmq_conn::interact("inputs?"); 
				Wt::Json::Array aStartBefore = jStartBefore.get("before");
				signed long long startBefore = aStartBefore[2];
				start -= startBefore * 1000;
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
		
	zmq_conn::interact(ret,true);


}
void groupMarkers(Wt::WTreeTable *markerTree)
{
 	Wt::WTreeNode *parent;
	Wt::WTreeNode *newNode;
	std::set<Wt::WTreeNode*> unSortedselectedNodes =markerTree->tree()->selectedNodes();
	std::vector<Wt::WTreeNode*> selectedNodes ( unSortedselectedNodes.begin(), unSortedselectedNodes.end());
	std::sort(selectedNodes.begin(),selectedNodes.end(), fragmentAbeforeB);
 
	std::vector< Wt::WTreeNode*> siblings;
 	
	Wt::WTreeNode *firstNode = *selectedNodes.begin();
	if(firstNode == markerTree->tree()->treeRoot())
	{
		return;
	}
	parent = firstNode->parentNode();
	newNode = MyTreeTableNode::addNode(0 ,"Group" ,-1,-1);
	siblings = parent->childNodes();
	int index = std::find(siblings.begin(), siblings.end(), firstNode) - siblings.begin();
	parent->insertChildNode(index, newNode);
	for(auto node:selectedNodes)
	{
		parent->removeChildNode(node);
		newNode->addChildNode(node);
	}

}

void ungroupMarkers(Wt::WTreeTable *markerTree)
{
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

}



void splitFragment(Wt::WTreeTable *markerTree, long pos)
{

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
			assert (index !=-1);
		/*	{
				this->log("warn")<<"Can't find myself when splitting a fragment";
				return;
			}*/
			MyTreeTableNode *newFragmentTTN = MyTreeTableNode::addNode(0,"New node", pos, stop);
			my_parent->insertChildNode(index+1, newFragmentTTN);
		}
		
	}	

}

void joinSelectedFragments(Wt::WTreeTable *markerTree)
{
	long prevStop = -2; //To check for contiguous fragments. Fragment times are never negative, but -1 is used to signify groups
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

}


void deleteEmptyGroups(Wt::WTreeTable *markerTree)
{
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
//this->log("info")<<"Trying to delete a none-group";
			return;
		}
		if(node->childNodes().size()>0)
		{
//this->log("info")<<"Trying to delete a non-empty group";
			return;
		}
		node->parentNode()->removeChildNode(node);
	}
	
}









void loadGroup(MyTreeTableNode *current_root, Wt::Json::Array fragments, bool mini)
{ //Recursively add the fragments to the treetable //does not need object at all
//this->log("info") <<"Loading fragments";
	for(auto fragmentValue:fragments)
	{
		const Wt::Json::Array& fragment = fragmentValue;
		std::string type = fragment[0];
		Wt::WString name = fragment[1];
//this->log("debug") <<"Loading fragment "<<name<<" of type "<<type;
		if (type == "group")
		{
			
			loadGroup( MyTreeTableNode::addNode(current_root,name,-1,-1, mini) ,fragment[2], mini);	
		}
		else if (type == "fragment")
		{
			long long start_time = fragment[2]; 
			long long stop_time = fragment[3];
			MyTreeTableNode::addNode(current_root,name,start_time,stop_time, mini);
		}
		else
		{
//this->log("warn")<<"Node type not understood";
		}
	}

		
}

void loadFragments(Wt::WTreeTable *markerTree, bool mini, zmq::socket_t *socket)
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
		
	Wt::Json::Object response;

	response = zmq_conn::interact("title?",socket);

	Wt::WString trackname = response.get("title"); 
	MyTreeTableNode *root = new MyTreeTableNode(trackname);
	markerTree->setTreeRoot(root,trackname); 
	MyTreeTableNode *current_root = root;
	response = zmq_conn::interact("fragments?",socket);
	Wt::Json::Array fragments;
	fragments = response.get("fragments");

	loadGroup(current_root,fragments, mini);
	
	root->expand();
	if (disconnect)
	{
		zmq_conn::disconnect(socket);
	}	
}













void saveFragmentsTree(Wt::WTreeTable *markerTree) 
{
	Wt::Json::Value fragmentsval = saveFragments(dynamic_cast<MyTreeTableNode*>(markerTree->tree()->treeRoot() ));
	Wt::Json::Array& fragments = fragmentsval; 
	std::string fragstring = Wt::Json::serialize(fragments);
	fragstring = "{ \"fragments\" : "+fragstring + "}";
	zmq_conn::interact(fragstring,true);

}

Wt::Json::Value saveFragments(MyTreeTableNode *root)
{ 

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
		//this->log("info")<< "Saving fragment "<<std::to_string(start) << "  "<<std::to_string(stop);
		ret.push_back(Wt::Json::Value(start));
		ret.push_back(Wt::Json::Value(stop));
	}
return retVal;
}



std::vector<MyTreeTableNode*> children_as_vector(Wt::WTreeNode *root)
{
	return children_as_vector(dynamic_cast<MyTreeTableNode*>(root));
}


std::vector<MyTreeTableNode*> children_as_vector(MyTreeTableNode *root)
{
	std::vector<MyTreeTableNode*> retval;
	std::vector<Wt::WTreeNode*> children = root->childNodes();
	for(auto node:children)
	{
		retval.push_back(dynamic_cast<MyTreeTableNode*>(node)); //FIXME? How does this do the ordering of things. Is push_back the right function?
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




