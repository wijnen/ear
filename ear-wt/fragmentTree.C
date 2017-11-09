#include "fragmentTree.h"


MyTreeTableNode::MyTreeTableNode(const Wt::WString& labelText, Wt::WIconPair *labelIcon,  Wt::WTreeTableNode *parentNode) :  Wt::WTreeTableNode( labelText,  labelIcon , parentNode )
{

}
MyTreeTableNode *MyTreeTableNode::addNode(MyTreeTableNode *parent, Wt::WString name, const long start, const long stop ) {
	MyTreeTableNode *node = new MyTreeTableNode(name, 0, parent);
	Wt::WContainerWidget *labelArea = node->labelArea();
	Wt::WWidget *labelWidget = labelArea->widget(0); //See source of WTreeNode.
	labelArea->removeWidget(labelWidget);
	node->editWidget = new Wt::WInPlaceEdit(name);
	
	node->editWidget->valueChanged().connect(std::bind([=]() {
		node->text = node->editWidget->text();
	}));
	node->text = node->editWidget->text();
	labelArea->addWidget(node->editWidget );
	TimeWidget *startWidget = new TimeWidget();
	startWidget->setTime(start);
	node->setColumnWidget(1, startWidget); 
//todo: add doubleclick trick to allow modal edit
	Wt::WPushButton *startButton = new Wt::WPushButton("|>");
	startButton->clicked().connect(std::bind([=]() {
//this->log("debug")<<"Handlign a startbutton click from the markertree";
		MyTreeTableNode* mynode =  dynamic_cast<MyTreeTableNode*> (node);
		long mystart = start;	
		while(mystart == -1) //We've clicked the startbutton on a group, so we need to find the first non-group widget to get a start time. This should probably be recursive at some point 
		{ 
			if (node->childNodes().size() > 0)
			{
				MyTreeTableNode *myttn = dynamic_cast<MyTreeTableNode*> (*(mynode->childNodes()).begin());
				mystart = dynamic_cast<TimeWidget*>(myttn->columnWidget(1))->time();
				mynode = myttn;

			}
			else
			{ //Where in a childless group, so we cannot play anything!
				return;
			}
		}
		Wt::Json::Object jStartBefore = zmq_conn::interact("inputs?"); 
		Wt::Json::Array aStartBefore = jStartBefore.get("before");
		signed long long startBefore = aStartBefore[2];
		zmq_conn::interact(Wt::WString("event:stop")); //Probably needed to help stop the track from stopping the middle of a play

		Wt::WString command="play:"+std::to_string(mystart - startBefore); 
		zmq_conn::interact(command);
	}));
	node->setColumnWidget(3, startButton);

	TimeWidget *stopWidget = new TimeWidget();
	stopWidget->setTime(stop);
	node->setColumnWidget(2, stopWidget);
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
				start -= startBefore;
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
		
zmq_conn::send(ret);


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




