// Copyright 2022 Bas Wijnen <wijnen@debian.org>

#ifndef _MyTreeTableNode
#define _MyTreeTableNode
#include <Wt/WApplication.h>
#include <Wt/WString.h>
#include <Wt/WTree.h>
#include <Wt/WTreeNode.h>
#include <Wt/WTreeTable.h>
#include <Wt/WTreeTableNode.h>
#include <Wt/WInPlaceEdit.h>
#include <Wt/WPushButton.h>
#include <Wt/Json/Array.h>
#include <Wt/WEvent.h>
#include "TimeWidget.h"
#include "earzmq.h"

class MyTreeTableNode : public Wt::WTreeTableNode
{
public:
	MyTreeTableNode(const Wt::WString& labelText, const long start, const long stop, bool mini);
	using Wt::WTreeTableNode::labelArea;
	Wt::WString text;
	Wt::WInPlaceEdit *editWidget;
	static MyTreeTableNode *addNode(Wt::WTreeTableNode *parentNode, Wt::WString name, const long start, const long stop, bool mini = false ) ;
	Wt::WPushButton *startButton;
	TimeWidget *startWidget;
	TimeWidget *stopWidget;
};



 //TODO: Namespace this? Re-factor everything into a class?
bool fragmentAbeforeB(Wt::WTreeNode* A, Wt::WTreeNode* B); //Needs renaming)
std::vector<MyTreeTableNode*> children_as_vector(MyTreeTableNode *root);
std::vector<MyTreeTableNode*> children_as_vector(Wt::WTreeNode *root);
std::vector<MyTreeTableNode*> ancestors_as_vector(MyTreeTableNode *child);

//Functions below should go into a new MarkerTree class, at one point
void playSelection(Wt::WTreeTable *markerTree);
void groupMarkers(Wt::WTreeTable *markerTree);
void ungroupMarkers(Wt::WTreeTable *markerTree);
void splitFragment(Wt::WTreeTable *markerTree, long pos);
void joinSelectedFragments(Wt::WTreeTable *markerTree);
void deleteEmptyGroups(Wt::WTreeTable *markerTree);
void saveFragmentsTree(Wt::WTreeTable *markerTree);
void loadGroup(Wt::WTreeTableNode *current_root, Wt::Json::Array fragments, bool mini = false);
void loadFragments(Wt::WTreeTable *markerTree, bool mini = false, zmq::socket_t *socket = 0 );
void mark_current_fragment(Wt::WTreeTable *markerTree, long long track_time);
Wt::Json::Value saveFragments(MyTreeTableNode *root);
#endif
