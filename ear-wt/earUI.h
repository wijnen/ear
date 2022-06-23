// Copyright 2022 Bas Wijnen <wijnen@debian.org>

#ifndef _earUI
#define _earUI

#include "earUI_base.h"
#include <Wt/WBreak.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WLineEdit.h>
#include <Wt/WPushButton.h>
#include <Wt/WText.h>
#include <Wt/WString.h>
#include <Wt/WGroupBox.h>
#include <Wt/WVBoxLayout.h>
#include <Wt/WHBoxLayout.h>
#include <Wt/WAnimation.h>
#include <Wt/WStringListModel.h>
#include <Wt/WStandardItemModel.h>
#include <Wt/WComboBox.h>
#include <Wt/WSelectionBox.h>
#include <Wt/WPanel.h>
#include <Wt/WSlider.h>
#include <Wt/WTimer.h>
#include <Wt/Json/Array.h>
#include <Wt/Json/Parser.h>
#include <Wt/Json/Object.h>
#include <Wt/Json/Serializer.h>
#include <Wt/WSignal.h>
#include <Wt/WTreeTable.h>
#include <Wt/WTree.h>
#include <Wt/WTreeNode.h>
#include <Wt/WTreeTableNode.h>
#include <Wt/Chart/WCartesianChart.h>
#include <zmq.hpp>
#include <string>
#include <iostream>
#include <boost/range/adaptor/reversed.hpp>
#include <Wt/WCompositeWidget.h>
#include <Wt/WBootstrap3Theme.h>
#include <Wt/WEnvironment.h>

#include "earzmq.h"
#include "filteredStringModel.h"
#include "TimeWidget.h"
#include "fragmentTree.h"
#include <Wt/WLogger.h>
#include "trackSearchContainer.h"

class EarUI : public EarUI_base
{
public:
	EarUI(const Wt::WEnvironment& env);

	static long start_track_time;
	static long stop_track_time;
	static long time_speed;
	void updateInputs();
private:
	bool posSliderMoving;
	bool posSliderSetting;
	TimeWidget *posText;
	Wt::WLength width = Wt::WLength::Auto; 
	std::vector<MyTreeTableNode*> fragment_set;
	void clicked(Wt::WPushButton* source );
	Wt::WTreeTable *markerTree;
	MyTreeTableNode *addNode(Wt::WString name, const long start, const long stop );
	Wt::Json::Value saveFragments(MyTreeTableNode *root);

	Wt::WPushButton *playPauseButton;

	std::map<std::string, Wt::WText*> inputTexts;
	std::map<std::string, Wt::WSlider*> inputSliders;
	long current_track_time( zmq::socket_t *socket  =0  );
	Wt::WSlider *posSlider;
	Wt::WSlider *beforeSlider;

	// Waveform plotting.
	void loadWaveform();
	std::shared_ptr <Wt::WStandardItemModel> waveformModel;
	Wt::Chart::WCartesianChart *waveformChart;
	Wt::WText *chartText;
	Wt::WTimer *waveformTimer;

	int ui_track_idx;
	int max_tags;
};

#endif
