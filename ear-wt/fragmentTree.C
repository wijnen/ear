// Copyright 2022 Bas Wijnen <wijnen@debian.org>

#include "fragmentTree.h"


MyTreeTableNode::MyTreeTableNode(const Wt::WString& labelText, const long start, const long stop, bool mini)
		:  Wt::WTreeTableNode( labelText,  nullptr ) {
	Wt::WContainerWidget *myLabelArea(labelArea());
	Wt::WWidget *labelWidget(myLabelArea->widget(0)); //See source of WTreeNode.
	myLabelArea->removeWidget(labelWidget);
	if (mini) {
		// We make these, even if we're doing the mini-tree. We use
		// these widgets to store the actual data, so that if we need
		// the start or stop time, we can get them out of the widget
		// and get the right numbers, even if we haven't saved the
		// fragment set yet.
		startWidget = new TimeWidget();
		stopWidget = new TimeWidget();
	}
	else {
		setColumnWidget(2, std::make_unique <TimeWidget> ());
		startWidget = dynamic_cast <TimeWidget *> (columnWidget(2));
		setColumnWidget(3, std::make_unique <TimeWidget> ());
		stopWidget = dynamic_cast <TimeWidget *> (columnWidget(3));
	}
	startWidget->setTime(start);
	stopWidget->setTime(stop);

	Wt::WString startLabel;

	if (mini)
		startLabel = labelText;
	else
	{
		setColumnWidget(1, std::make_unique <Wt::WInPlaceEdit> ());
		editWidget = dynamic_cast <Wt::WInPlaceEdit *> (columnWidget(1));
		std::string str = labelText.toUTF8();
		str.erase(std::remove(str.begin(), str.end(), ' '), str.end()); //This removes whitespace
		if(str.length()<1) {
			editWidget->setText(Wt::WString::Empty);
			editWidget->setPlaceholderText("New Node");

		}
		else
			editWidget->setText(labelText);
		editWidget->valueChanged().connect(std::bind([&]() {

			std::string str = editWidget->text().toUTF8();
			str.erase(std::remove(str.begin(), str.end(), ' '), str.end()); //This removes whitespace
			if(str.length()<1)
			{
				editWidget->setText(Wt::WString::Empty);
			}
			text = editWidget->text();
		}));

		text = editWidget->text();

		startLabel = "▶️";
	}
//todo: add doubleclick trick to allow modal edit
	startButton = myLabelArea->addWidget(std::make_unique <Wt::WPushButton> (startLabel));
	startButton->clicked().connect(std::bind([=]() {
		if (start == -1) {	//We've clicked the startbutton on a group, so we need to find the first non-group widget.
			if (childNodes().size() > 0) {
 				// So, there are children.. In this case take the first child and pretend we've clicked that startButton. It'll be recursive.
				MyTreeTableNode *firstChild = dynamic_cast<MyTreeTableNode*> (*(childNodes()).begin());
				return firstChild->startButton->clicked().emit(Wt::WMouseEvent());
			}
			else {
				// We're in a childless group, so we cannot play anything!
				return;
			}
		}
		Wt::Json::Object jStartBefore = zmq_conn::interact("inputs?");
		Wt::Json::Array aStartBefore = jStartBefore.get("before");
		signed long long startBefore = aStartBefore[2];
		zmq_conn::interact(Wt::WString("event:stop")); //Probably needed to help stop the track from stopping the middle of a play

		Wt::WString command="play:"+std::to_string(start - startBefore * 1000);
		zmq_conn::interact(command);
	}));
    }





bool fragmentAbeforeB(Wt::WTreeNode* A, Wt::WTreeNode* B) //Needs renaming)
{
	MyTreeTableNode *myA =  dynamic_cast<MyTreeTableNode*>(A);
	while(myA->startWidget->time() ==-1) //If it's a group....   //TODO: make get_first_fragment_child
	{
		if(myA->childNodes().size() ==0)
			return false; //If it's an empty group
		myA = dynamic_cast<MyTreeTableNode*>(*(myA->childNodes().begin())); //Otherwise, get the first child. It's a while loop, so it'll get the first non-group child in a direct line of first-born children.
	}
	long startA = myA->startWidget->time();
	long stopA = myA->stopWidget->time();

	MyTreeTableNode *myB =  dynamic_cast<MyTreeTableNode*>(B);
	while(myB->startWidget->time() ==-1)
	{
		if(myB->childNodes().size() ==0)
			return false;
		myB = dynamic_cast<MyTreeTableNode*>(*(myB->childNodes().begin()));
	}
	long startB = myB->startWidget->time();
	long stopB = myB->stopWidget->time();

	//Strictly, this does not mean that A<B means that B>A, because of stuff. See what breaks? --KRL 11072017 FIXME
	if(startA < startB)
	{
		if(stopA<stopB)
			return true;
	}
	return false;
}

void playSelection(Wt::WTreeTable *markerTree) {
	std::string ret = "{\"play\": [";
	bool first = true;
	for (auto node:children_as_vector(dynamic_cast <MyTreeTableNode *> (markerTree->tree()->treeRoot()))) {
		long start = node->startWidget->time();
		long stop = node->stopWidget->time();
		bool selected = markerTree->tree()->isSelected(node);
		if (start == -1 or stop ==-1)
			continue;
		for(auto parent:ancestors_as_vector(node)) {
			if(markerTree->tree()->isSelected(parent) and not parent->isExpanded()) {
				selected = true;
				break;
			}
		}
		if(selected) {
			if (first) {
				Wt::Json::Object jStartBefore = zmq_conn::interact("inputs?");
				Wt::Json::Array aStartBefore = jStartBefore.get("before");
				signed long long startBefore = aStartBefore[2];
				start -= startBefore * 1000;
				first = false;
			}
			else
				ret+=" , ";
			ret += "["+std::to_string(start)+" , "+	std::to_string(stop)+"]\n";
		}
	}
	ret+="]}";

	zmq_conn::interact(ret,true);
}

void groupMarkers(Wt::WTreeTable *markerTree) {
 	Wt::WTreeNode *parent;
	std::set<Wt::WTreeNode*> unSortedselectedNodes =markerTree->tree()->selectedNodes();
	std::vector<Wt::WTreeNode*> selectedNodes ( unSortedselectedNodes.begin(), unSortedselectedNodes.end());
	std::sort(selectedNodes.begin(),selectedNodes.end(), fragmentAbeforeB);

	std::vector< Wt::WTreeNode*> siblings;

	Wt::WTreeNode *firstNode = *selectedNodes.begin();
	if(firstNode == markerTree->tree()->treeRoot())
		return;
	parent = firstNode->parentNode();
	siblings = parent->childNodes();
	int index = std::find(siblings.begin(), siblings.end(), firstNode) - siblings.begin();
	parent->insertChildNode(index, std::make_unique <MyTreeTableNode> ("Group", -1, -1, false));
	auto newNode = parent->childNodes()[index];
	for(auto node:selectedNodes)
		newNode->addChildNode(parent->removeChildNode(node));
}

void ungroupMarkers(Wt::WTreeTable *markerTree) {
	std::vector< Wt::WTreeNode*> siblings;
	Wt::WTreeNode *parent;
	Wt::WTreeNode *grandparent;
	std::vector< Wt::WTreeNode*> uncles; //My parent's siblings
	int index = 0;

	std::set <Wt::WTreeNode*> unSortedselectedNodes = markerTree->tree()->selectedNodes();
	std::vector <Wt::WTreeNode*> selectedNodes (unSortedselectedNodes.begin(), unSortedselectedNodes.end());
	std::sort(selectedNodes.begin(),selectedNodes.end(), fragmentAbeforeB);

	Wt::WTreeNode *firstNode = *selectedNodes.begin();
	Wt::WTreeNode *lastNode = *selectedNodes.rbegin(); //Note, the reverse of the beginning is not the end
	parent = firstNode->parentNode();
	if(parent == markerTree->tree()->treeRoot()) {
		return;
	}
	siblings = parent->childNodes();

	if(firstNode != siblings.front() and lastNode != siblings.back()) {
		// Invalid selection: can only ungroup nodes from an edge.
		return;
	}
	grandparent = parent->parentNode();
	uncles = grandparent->childNodes();
	index = std::find(uncles.begin(), uncles.end(), parent) - uncles.begin();
	if (firstNode == siblings.front() and lastNode != siblings.back()) {
		index--;
	}
	for (auto node: selectedNodes) {
		index++;
		grandparent->insertChildNode(index, parent->removeChildNode(node));
	}
}



void splitFragment(Wt::WTreeTable *markerTree, long pos) {
	for (auto fragmentTTN:children_as_vector(markerTree->tree()->treeRoot()) ) {
		long start = fragmentTTN->startWidget->time();
		long stop = fragmentTTN->stopWidget->time();
		if (pos > start && pos < stop) {
			//Fragment to split
			fragmentTTN->stopWidget->setTime(pos);
			Wt::WTreeNode *my_parent = fragmentTTN->parentNode();
			const std::vector< Wt::WTreeNode * > siblings = my_parent->childNodes();
			int index = -1;
			for (unsigned int i = 0; i < siblings.size(); i++) {
				//There is insert, and insertBefore, but no insertAfter. So first, determine the index of the widget we're splitting, and then insert at that index.
				if (fragmentTTN == siblings[i])
					index = i;
			}
			assert (index !=-1);
			my_parent->insertChildNode(index+1, std::make_unique <MyTreeTableNode> ("", pos, stop, false));
		}

	}

}

void joinSelectedFragments(Wt::WTreeTable *markerTree) {
	long prevStop = -2; //To check for contiguous fragments. Fragment times are never negative, but -1 is used to signify groups
	std::string newname ="";
	std::set<Wt::WTreeNode*> unSortedselectedNodes =markerTree->tree()->selectedNodes();
	std::vector<Wt::WTreeNode*> selectedNodes ( unSortedselectedNodes.begin(), unSortedselectedNodes.end());
	std::sort(selectedNodes.begin(),selectedNodes.end(), fragmentAbeforeB);

	for (auto node:selectedNodes) {
	//tree returns a tree with tree nodes. We need treetable nodes!
		MyTreeTableNode *fragmentTTN =  dynamic_cast<MyTreeTableNode*>(node);
		long start = fragmentTTN->startWidget->time();
		long stop = fragmentTTN->stopWidget->time();
		if (prevStop !=-2) {
			if (prevStop != start)
				return; //Not contiguous
		}
		prevStop = stop;
		newname +=" "+ fragmentTTN->text.narrow();
	}
//Change the first node
	MyTreeTableNode *firstNode = dynamic_cast<MyTreeTableNode*>(*selectedNodes.begin());
	std::string str = newname;
	str.erase(std::remove(str.begin(), str.end(), ' '), str.end()); //This removes whitespace
	if(str.length()>1)
	        firstNode->editWidget->setText(newname);
	firstNode->stopWidget->setTime(prevStop);
	bool first=true;
//Dwlete all others
	for (auto node:selectedNodes) {
		if (not first)
			node->parentNode()->removeChildNode(node);
		first = false;
	}

}


void deleteEmptyGroups(Wt::WTreeTable *markerTree) {
	std::set<Wt::WTreeNode*> selectedNodes = markerTree->tree()->selectedNodes();
	for (auto node:selectedNodes) {
		MyTreeTableNode *fragmentTTN = dynamic_cast<MyTreeTableNode*>(node);
		long start = fragmentTTN->startWidget->time();
		long stop = fragmentTTN->stopWidget->time();
		if (start != -1  or stop !=-1) {
			//this->log("info")<<"Trying to delete a none-group";
			return;
		}
		if(node->childNodes().size()>0) {
			//this->log("info")<<"Trying to delete a non-empty group";
			return;
		}
		node->parentNode()->removeChildNode(node);
	}

}

void loadGroup(Wt::WTreeTableNode *current_root, Wt::Json::Array fragments, bool mini) {
	//Recursively add the fragments to the treetable //does not need object at all
	//this->log("info") <<"Loading fragments";
	for(auto fragmentValue:fragments) {
		const Wt::Json::Array& fragment = fragmentValue;
		std::string type = fragment[0];
		Wt::WString name = fragment[1];
		//this->log("debug") <<"Loading fragment "<<name<<" of type "<<type;
		if (type == "group") {

			auto node = current_root->addChildNode(std::make_unique <MyTreeTableNode> (name, -1, -1, mini));
			loadGroup(dynamic_cast <MyTreeTableNode *> (node), fragment[2], mini);
		}
		else if (type == "fragment") {
			long long start_time = fragment[2];
			long long stop_time = fragment[3];
			current_root->addChildNode(std::make_unique <MyTreeTableNode> (name, start_time, stop_time, mini));
		}
		else {
			//this->log("warn")<<"Node type not understood";
		}
	}
}

void loadFragments(Wt::WTreeTable *markerTree, bool mini, zmq::socket_t *socket) {
	//needs markerTree
	#ifdef PLOT
		waveformTimer->start();
	#endif
	bool disconnect = false;
	if (socket == 0) {
		socket = zmq_conn::connect();
		disconnect = true;
	}

	Wt::Json::Object response;

	response = zmq_conn::interact("title?",socket);

	Wt::WString trackname = response.get("title");
	markerTree->setTreeRoot(std::make_unique <MyTreeTableNode> (trackname, -1, -1, mini), trackname);
	auto root = markerTree->treeRoot();
	response = zmq_conn::interact("fragments?",socket);
	Wt::Json::Array fragments;
	fragments = response.get("fragments");

	loadGroup(root, fragments, mini);

	root->expand();
	if (disconnect)
		zmq_conn::disconnect(socket);
}





void mark_current_fragment(Wt::WTreeTable *markerTree, long long pos) {
	for (auto fragmentTTN:children_as_vector(markerTree->tree()->treeRoot()) ) {
		long start = fragmentTTN->startWidget->time();
		long stop = fragmentTTN->stopWidget->time();
		if(pos > start and pos < stop) {
			for( auto node:ancestors_as_vector(fragmentTTN))
				dynamic_cast<MyTreeTableNode*>(node)->startButton->addStyleClass("btn-info");
			//fragmentTTN->decorationStyle().setBackgroundColor(Wt::WColor(255,0,0)); //TODO: ?Make a proper style, and enlarge the font or something
			//TODO: Maybe get the parents too, if the current widget is not shown
		}
		else {
			dynamic_cast<MyTreeTableNode*>(fragmentTTN)->startButton->removeStyleClass("btn-info");
			//fragmentTTN->decorationStyle().setBackgroundColor(Wt::WColor(255,255,255)); //TODO: Properly remove the previously added style
		}
	}
}

void saveFragmentsTree(Wt::WTreeTable *markerTree) {
	Wt::Json::Value fragmentsval = saveFragments(dynamic_cast<MyTreeTableNode*>(markerTree->tree()->treeRoot() ));
	Wt::Json::Array& fragments = fragmentsval;
	std::string fragstring = Wt::Json::serialize(fragments);
	fragstring = "{ \"fragments\" : " + fragstring + "}";
	zmq_conn::interact(fragstring,true);
}

Wt::Json::Value saveFragments(MyTreeTableNode *root) {
	Wt::Json::Value retVal = Wt::Json::Value(Wt::Json::Type::Array);
	Wt::Json::Array& ret = retVal;
	Wt::WString name;
	name = root->text;
	std::string str = name.toUTF8();
	str.erase(std::remove(str.begin(), str.end(), ' '), str.end()); //This removes whitespace
	if(str.length()<1)
		name = Wt::WString("New Node");
	if(root->childNodes().size()>0) {
		ret.push_back(Wt::Json::Value(Wt::WString("group")));
		ret.push_back(Wt::Json::Value(name));
		Wt::Json::Value out_children_value = Wt::Json::Value(Wt::Json::Type::Array);
		Wt::Json::Array& out_children = out_children_value;
		for(auto mynode:root->childNodes()) //I wonder, what order do we get these in?
			out_children.push_back(saveFragments(dynamic_cast<MyTreeTableNode*>(mynode)));

		ret.push_back(out_children_value);
	}
	else {
		//TODO add room for annotations
		ret.push_back(Wt::Json::Value(Wt::WString("fragment")));
		ret.push_back(Wt::Json::Value(name));
		long long start = root->startWidget->time();
		long long stop = root->stopWidget->time();
		//this->log("info")<< "Saving fragment "<<std::to_string(start) << "  "<<std::to_string(stop);
		ret.push_back(Wt::Json::Value(start));
		ret.push_back(Wt::Json::Value(stop));
	}
	return retVal;
}

std::vector<MyTreeTableNode*> children_as_vector(Wt::WTreeNode *root) {
	return children_as_vector(dynamic_cast<MyTreeTableNode*>(root));
}

std::vector<MyTreeTableNode*> children_as_vector(MyTreeTableNode *root) {
	std::vector<MyTreeTableNode*> retval;
	std::vector<Wt::WTreeNode*> children = root->childNodes();
	for(auto node:children) {
		retval.push_back(dynamic_cast<MyTreeTableNode*>(node)); //FIXME? How does this do the ordering of things. Is push_back the right function?
		if(node->childNodes().size()>0) {
			std::vector<MyTreeTableNode*> grandchildren = children_as_vector(dynamic_cast<MyTreeTableNode*>(node));
			retval.insert(retval.end(),grandchildren.begin(),grandchildren.end());
		}
	}
	return retval;
}

std::vector<MyTreeTableNode*> ancestors_as_vector(MyTreeTableNode *child) {
	std::vector<MyTreeTableNode*> retval;
	MyTreeTableNode* currentGen = child;
	while(currentGen != child->tree()->treeRoot()) {
		retval.push_back(currentGen);
		currentGen = dynamic_cast<MyTreeTableNode*>(currentGen->parentNode());
	}
	return retval;
}
