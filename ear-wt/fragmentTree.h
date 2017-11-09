#ifndef _MyTreeTableNode
#define _MyTreeTableNode
#include <Wt/WString>
#include <Wt/WTree>
#include <Wt/WTreeNode>
#include <Wt/WTreeTable>
#include <Wt/WTreeTableNode>
#include <Wt/WInPlaceEdit>
#include <Wt/WPushButton>
#include "TimeWidget.h"
#include "earzmq.h"
class MyTreeTableNode : public Wt::WTreeTableNode
{
	public:
		MyTreeTableNode(const Wt::WString& labelText, Wt::WIconPair *labelIcon = 0,  Wt::WTreeTableNode *parentNode = 0) ;
	using Wt::WTreeTableNode::labelArea;
	Wt::WString text;		
	Wt::WInPlaceEdit*  editWidget;
	static MyTreeTableNode *addNode(MyTreeTableNode *parent, Wt::WString name, const long start, const long stop ) ;
};



 //TODO: Namespace this? Re-factor everything into a class?
bool fragmentAbeforeB(Wt::WTreeNode* A, Wt::WTreeNode* B); //Needs renaming)
std::vector<MyTreeTableNode*> children_as_vector(MyTreeTableNode *root);
std::vector<MyTreeTableNode*> children_as_vector(Wt::WTreeNode *root);
std::vector<MyTreeTableNode*> ancestors_as_vector(MyTreeTableNode *child);
void playSelection(Wt::WTreeTable *markerTree);
void groupMarkers(Wt::WTreeTable *markerTree);
void ungroupMarkers(Wt::WTreeTable *markerTree);
void splitFragment(Wt::WTreeTable *markerTree, long pos);
void joinSelectedFragments(Wt::WTreeTable *markerTree);
#endif
