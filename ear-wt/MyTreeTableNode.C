#include "MyTreeTableNode.h"


MyTreeTableNode::MyTreeTableNode(const Wt::WString& labelText, Wt::WIconPair *labelIcon,  Wt::WTreeTableNode *parentNode) :  Wt::WTreeTableNode( labelText,  labelIcon , parentNode )
{

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




