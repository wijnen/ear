// Copyright 2022 Bas Wijnen <wijnen@debian.org>

#ifndef _trackSearchContainer
#define _trackSearchContainer

#include <Wt/WContainerWidget.h>
#include <Wt/WPushButton.h>
#include <Wt/WSelectionBox.h>
#include <Wt/WVBoxLayout.h>
#include <Wt/WText.h>
#include <Wt/WString.h>
#include "earzmq.h"
#include <Wt/WLineEdit.h>
#include "filteredStringModel.h"
#include <Wt/WLogger.h>
#include "earUI_base.h"


class TrackSearchContainer : public Wt::WContainerWidget
{
	public:
		TrackSearchContainer(EarUI_base *parentUI);
	private:
		EarUI_base *parentUI;
		std::shared_ptr <FilteredStringModel> trackModel;

};

#endif
