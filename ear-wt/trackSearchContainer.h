#ifndef _trackSearchContainer
#define _trackSearchContainer

#include <Wt/WContainerWidget>
#include <Wt/WPushButton>
#include <Wt/WSelectionBox>
#include <Wt/WVBoxLayout>
#include <Wt/WText>
#include <Wt/WString>
#include "earzmq.h"
#include <Wt/WLineEdit>
#include "filteredStringModel.h"
#include <Wt/WLogger>
#include "earUI.h"


class TrackSearchContainer : public Wt::WContainerWidget 
{
	public:
		TrackSearchContainer(EarUI *parentUI = 0, Wt::WContainerWidget *parent = 0); 
	private:
		EarUI *parentUI;
		
};

#endif
