/*
 * Example used: Copyright (C) 2008 Emweb bvba, Heverlee, Belgium.
 *
 * Code copyright Kasper Loopstra, 2017
 * Copyright 2022 Bas Wijnen <wijnen@debian.org>
 */
#include "earUI.h"
EarUI::EarUI(const Wt::WEnvironment& env) : EarUI_base(env) {
	ui_track_idx = -1;
	max_tags = 0;
	posSliderMoving = false;
	posSliderSetting = false;
	setTitle("Ear interface"); 

	auto theme = std::make_shared <Wt::WBootstrap3Theme> ();
	theme->setResponsive(true);
	setTheme(theme);

	auto selectPanel = root()->addWidget(std::make_unique <Wt::WPanel> ());
	selectPanel->setTitle("Select new track");
	selectPanel->resize(width, Wt::WLength::Auto);
	selectPanel->setCollapsible(true);
	selectPanel->setCollapsed(true); 

	//selectPanel->setObjectName("selectionPanel");
	//Wt::WAnimation animation(Wt::WAnimation::SlideInFromTop,
	//			 Wt::WAnimation::EaseOut,
	//			 100); //Animation breaks with a VBoxLayout. Maybe report?
	//selectPanel->setAnimation(animation);

	selectPanel->setCentralWidget(std::make_unique <TrackSearchContainer> (this));

	auto sliderPanel = root()->addWidget(std::make_unique <Wt::WPanel> ());
	sliderPanel->setTitle("Change settings");
	sliderPanel->resize(width, Wt::WLength::Auto);
	sliderPanel->setCollapsible(true);
	sliderPanel->setCollapsed(true); 

	sliderPanel->setCentralWidget(std::make_unique <Wt::WContainerWidget> ());
	auto inputContainer = dynamic_cast <Wt::WContainerWidget *> (sliderPanel->centralWidget());
	inputContainer->resize(width, 500);

	inputContainer->resize(width, Wt::WLength::Auto); //Auto does not work when in a collapsible box?
	auto inputbox = inputContainer->setLayout(std::make_unique <Wt::WVBoxLayout> ());

	Wt::Json::Object inputs;
	inputs = zmq_conn::interact("inputs?");
	Wt::WSlider *inputSlider;
	Wt::Json::Array inputSettings;
	Wt::WText *valueText;
	this->log("info") << "Making input boxes";
	for (auto input_name:inputs.names()) {
		this->log("info") << "Making input box for " << input_name;
		auto thisInputContainer = inputbox->addWidget(std::make_unique <Wt::WContainerWidget> ());
		auto thisInputBox = thisInputContainer->setLayout(std::make_unique <Wt::WVBoxLayout> ());

		// TODO, make sure we pad the buttons and not expand them
		auto textbox = thisInputBox->addLayout(std::make_unique <Wt::WHBoxLayout> ());

		auto thisSliderContainer = thisInputContainer->addWidget(std::make_unique <Wt::WContainerWidget> ());

		// Title text.
		textbox->addWidget(std::make_unique <Wt::WText> (input_name), 0);

		valueText = textbox->addWidget(std::make_unique <Wt::WText> (""), 0);
		textbox->addStretch();
		inputTexts[input_name] = valueText;
		//	valueText->setObjectName("inputText:" + input_name); //Soo, setting this makes the widget disappear, or at least act up. Report?

		inputSettings = inputs.get(input_name);
		inputSlider = thisSliderContainer->addWidget(std::make_unique <Wt::WSlider> ());
		inputSlider->resize(width, 50); 
		//inputSlider->resize( Wt::WLength(100, Wt::WLength::Unit::Percentage), 50); //This kind of works. We get a huge slider (which si good) but the ticks are all in 100px or something on one side, and so is the min/max
		//inputSlider->setNativeControl(true); //If we use this, and move the slider, the program freezes
		inputSliders[input_name] = inputSlider;
		int min, max;
		min = inputSettings[0];
		max = inputSettings[1]; 
		inputSlider->setMinimum(min); 
	
		inputSlider->setMaximum(max); 
		inputSlider->setTickInterval( (max - min) / 6);
		inputSlider->setValue(inputSettings[2]); 
		inputSlider->setTickPosition(Wt::WSlider::TickPosition::TicksAbove);
	
		auto inputButtonContainer = thisInputBox->addWidget(std::make_unique <Wt::WContainerWidget> ());
		auto buttonbox = inputButtonContainer->setLayout(std::make_unique <Wt::WHBoxLayout> ());
		buttonbox->addStretch();
		this->log("info") << "Handling special sliders";
		if (input_name == "before") {
			std::vector<int> beforeButtons = {0, 3, 6, 10};
			for (auto before:beforeButtons) {
				auto bbutton = buttonbox->addWidget(std::make_unique <Wt::WPushButton> (Wt::WString(std::to_string(before))), 0);
				buttonbox->addStretch();
				bbutton->clicked().connect(std::bind([=]() {
					zmq_conn::interact("input:before:" + std::to_string(before));
					updateInputs();
				}));
			}

			beforeSlider = inputSlider; //We need to reference this further on
		}
		if (input_name == "speed") {
			std::vector<int> speedButtons = {50, 75, 90, 100, 110, 120, 150, 175, 200};
			for (auto speed:speedButtons) {
				std::string title = std::to_string(speed);
				title += "%";
				auto sbutton = buttonbox->addWidget(std::make_unique <Wt::WPushButton> (Wt::WString(title)), 0);
				buttonbox->addStretch();
				sbutton->clicked().connect(std::bind([=]() {
					zmq_conn::interact("input:speed:" + std::to_string(speed));
					updateInputs();
				}));
			}
		}
		inputSlider->valueChanged().connect(std::bind([=]() {
			zmq_conn::interact("input:" + input_name + ":" + inputSlider->valueText());
			valueText->setText(inputSlider->valueText());
			updateInputs();
		}));
	}
	auto posContainer = inputbox->addWidget(std::make_unique <Wt::WContainerWidget> ());
	auto posInputBox = posContainer->setLayout(std::make_unique <Wt::WVBoxLayout> ());

	this->log("info") <<"Making pos slider";
	auto posTextBox = posInputBox->addLayout(std::make_unique <Wt::WHBoxLayout> ());
	posTextBox->addWidget(std::make_unique <Wt::WText> ("Position"));
	posText = posTextBox->addWidget(std::make_unique <TimeWidget> ());
	posText->setTime(0);
	posSlider = posInputBox->addWidget(std::make_unique <Wt::WSlider> ());
	posSlider->valueChanged().connect(std::bind([=]() {
		if (posSliderSetting) {
			// Ignore this event if it was caused by calling setValue().
			return;
		}
		this->posSliderMoving = false;
		this->log("info") <<"Updating pos because posSlider valueChanged";
		zmq_conn::interact("pos:" + posSlider->valueText());
		posText->setTime(posSlider->value());
		this->log("info") << "slider value changed " << posSlider->value();
	}));

#ifdef PLOT
	// Plot the waveform.
	auto chartContainer = root()->addWidget(std::make_unique <Wt::WContainerWidget> ());
	chartText = chartContainer->addWidget(std::make_unique <Wt::WText> ("chart"));
	this->log("info") << "Charting";
	waveformChart = chartContainer->addWidget(std::make_unique <Wt::Chart::WCartesianChart> ());
	waveformModel = std::make_shared <Wt::WStandardItemModel> (0, 3);
	loadWaveform();
	waveformChart->setModel(waveformModel);
	waveformChart->setXSeriesColumn(0);	// set the column that holds the category
	waveformChart->setType(Wt::Chart::ChartType::Scatter);
	waveformChart->setAutoLayoutEnabled(true);
	auto l = std::make_unique <Wt::Chart::WDataSeries> (1, Wt::Chart::SeriesType::Line);
	l->setShadow(Wt::WShadow(3, 3, Wt::WColor(0, 0, 0, 127), 3));
	waveformChart->addSeries(std::move(l));
	auto r = std::make_unique <Wt::Chart::WDataSeries> (2, Wt::Chart::SeriesType::Line);
	r->setShadow(Wt::WShadow(3, 3, Wt::WColor(255, 0, 0, 127), 3));
	waveformChart->addSeries(std::move(r));
	waveformChart->resize(width, 100);
	waveformTimer = new Wt::WTimer();
	waveformTimer->setSingleShot(true);
	waveformTimer->setInterval(std::chrono::milliseconds(50));
	waveformTimer->timeout().connect( std::bind([=]() {
		loadWaveform();
	}));
	waveformTimer->start();
	//posInputBox->addWidget(std::unique_ptr <Wt::WWidget> (chartContainer));
#endif

	auto posButtonContainer = posInputBox->addWidget(std::make_unique <Wt::WContainerWidget> ());
	auto seekButtonBox = posButtonContainer->setLayout(std::make_unique <Wt::WHBoxLayout> ());
	seekButtonBox->addStretch();
	this->log("info") << "Adding seek buttons";
	std::vector<int> seekButtons = {-10, -5, -1, 1, 5, 10};
	for (auto seek:seekButtons) {
		std::string title = std::to_string(seek);
		if (seek>0) {
			title = "+" + title;
		}
		title += "s";
		auto sbutton = seekButtonBox->addWidget(std::make_unique <Wt::WPushButton> (Wt::WString(title)), 0);
		seekButtonBox->addStretch();
		sbutton->clicked().connect(std::bind([=]() {
			zmq_conn::interact("pos:" + std::to_string(current_track_time() + seek * 1000));
		}));
	
	}
	posSlider->resize(width, 50);
	posSlider->setTickInterval(60000); //One minute in ms
	posSlider->setTickPosition(Wt::WSlider::TickPosition::TicksAbove);
	posSlider->sliderMoved().connect(std::bind([this](const int &v) { 
		this->posText->setTime(v); //So we can move and see where we'll end up before releasing
		if (posSliderSetting) {
			// Only handle user-caused events here.
			return;
		}
		this->log("info") << "slider moved " << v;
		this->posSliderMoving = true;
		show_backtrace("moving");
	}, std::placeholders::_1));

	auto markerContainer = root()->addWidget(std::make_unique <Wt::WContainerWidget> ());
	markerContainer->resize(width, Wt::WLength::Auto);
	auto fragmentButtonsContainer = markerContainer->addWidget(std::make_unique <Wt::WContainerWidget> ());

	playPauseButton = fragmentButtonsContainer->addWidget(std::make_unique <Wt::WPushButton> ("Play from start"));
	playPauseButton->clicked().connect(std::bind([=]() {
		zmq_conn::interact(std::string("event:pause"));
	}));
	auto stopButton = fragmentButtonsContainer->addWidget(std::make_unique <Wt::WPushButton> ("Stop"));
	stopButton->clicked().connect(std::bind([=]() {
		zmq_conn::interact(std::string("event:stop"));
	}));
	stopButton->setMargin(5, Wt::Side::Left);

	markerTree = markerContainer->addWidget(std::make_unique <Wt::WTreeTable> ());
	//markerTree->setObjectName("markertree"); //TODO: Still something weird with the columns, they are sometimes offset
	markerTree->tree()->setSelectionMode(Wt::SelectionMode::Extended); 
	markerTree->resize(750, 1000);
 
	markerTree->addColumn("", 400);
	markerTree->addColumn("", 100);
	markerTree->addColumn("", 100);

	auto groupButtonsContainer = fragmentButtonsContainer->addWidget(std::make_unique <Wt::WContainerWidget> ());
	auto splitJoinButtonsContainer = fragmentButtonsContainer->addWidget(std::make_unique <Wt::WContainerWidget> ());
	auto playSelectionButton = groupButtonsContainer->addWidget(std::make_unique <Wt::WPushButton> ("Play selection"));
	//playSelectionButton->setMargin(5, Wt::Left);
	playSelectionButton->clicked().connect(std::bind([=]() {
		playSelection(markerTree);
	}));

	// This randomises the order (?)
	auto tabButton = groupButtonsContainer->addWidget(std::make_unique <Wt::WPushButton> ("Group selection >>>>"));

	tabButton->setMargin(5, Wt::Side::Left);
	tabButton->clicked().connect(std::bind([=]() { 
		groupMarkers(markerTree);
	}));
	
	auto untabButton = groupButtonsContainer->addWidget(std::make_unique <Wt::WPushButton> ("Ungroup selection <<<<<"));
	untabButton->setMargin(5, Wt::Side::Left);
	untabButton->clicked().connect(std::bind([=]() {
		ungroupMarkers(markerTree);
	}));

	auto splitButton = splitJoinButtonsContainer->addWidget(std::make_unique <Wt::WPushButton> ("Split fragment here"));
	//splitButton->setMargin(5, Wt::Side::Left);
	splitButton->clicked().connect(std::bind([=]() {	
		long pos = current_track_time();
		splitFragment(markerTree, pos);
	}));

	auto joinButton = splitJoinButtonsContainer->addWidget(std::make_unique <Wt::WPushButton> ("Join selected fragments"));
	joinButton->setMargin(5, Wt::Side::Left);
	joinButton->clicked().connect(std::bind([=]() {
		joinSelectedFragments(markerTree);
	}));
 
	auto delgrpButton = groupButtonsContainer->addWidget(std::make_unique <Wt::WPushButton> ("Delete empty group"));
	delgrpButton->setMargin(5, Wt::Side::Left);
	delgrpButton->clicked().connect(std::bind([=]() {
		deleteEmptyGroups(markerTree);
	}));

	auto saveButton = splitJoinButtonsContainer->addWidget(std::make_unique <Wt::WPushButton> ("Save fragments"));
	saveButton->setMargin(5, Wt::Side::Left);
	saveButton->clicked().connect(std::bind([=]() {	
		saveFragmentsTree(markerTree);
	}));


	Wt::WTimer *inputtimer = new Wt::WTimer();
	inputtimer->setInterval(std::chrono::milliseconds(2500));
	inputtimer->timeout().connect(std::bind([=]() {
		updateInputs();
	}));
	inputtimer->start();

	Wt::WTimer *timer = new Wt::WTimer();
	timer->setInterval(std::chrono::milliseconds(100));
	timer->timeout().connect(std::bind([=]() {
		zmq::socket_t *socket = zmq_conn::connect();
		bool playing;
		Wt::Json::Object playingj;
		long long track_time = 	current_track_time(socket);
		playingj = zmq_conn::interact(std::string("playing?"), socket);
		zmq_conn::disconnect(socket);

		if (not posSliderMoving) {
			posSliderSetting = true;
			posSlider->setValue(track_time);
			posText->setTime(track_time);
			posSliderSetting = false;
		}
		
		playing = playingj.get("playing");
		if (playing) {
			playPauseButton->setText("Pause");
		}
		else {
			if (track_time > 0) {
				playPauseButton->setText("Continue");
			}
			else {
				playPauseButton->setText("Play from start");
			}
		}
		mark_current_fragment(markerTree, track_time); 
	}));

	timer->start();
	updateInputs();

	this->log("info") << "Done building EarUI";
}

long EarUI::current_track_time( zmq::socket_t *socket ) {
	Wt::Json::Object posj;
	posj = zmq_conn::interact(std::string("pos?"), socket);
	Wt::Json::Value posjv = posj.get("pos");
	const long long pos = posjv;
	return pos;
}

#ifdef PLOT
void EarUI::loadWaveform() {
	Wt::Json::Object ret = zmq_conn::interact(std::string("waveform?"));
	Wt::Json::Array waveform = ret.get("waveform");
	chartText->setText(Wt::WString(std::to_string(waveform.size())));
	waveformModel->removeRows(0, waveformModel->rowCount());
	waveformModel->insertRows(0, waveform.size());
	if (waveform.size() < 10) {
		waveformModel->insertRows(0, 10);
		waveformTimer->start();
		for (int i = 0; i < 10 + 1; i++) {
			waveformModel->setData(i, 0, i);
			waveformModel->setData(i, 1, i);
			waveformModel->setData(i, 2, i);
		}
		return;
	}
	int i = 0;
	for (auto item: waveform) {
		Wt::Json::Array foo = item;
		double timestamp = foo[0];
		double l = foo[1];
		double r = foo[2];
		waveformModel->setData(i, 0, timestamp);
		waveformModel->setData(i, 1, l);
		waveformModel->setData(i, 2, r);
		i++;
		//std::cerr << "Adding waveform row " << i << " " << timestamp << " " << l << " " << r << std::endl;
	}
	
	waveformChart->resize(width, 100); //This is needed to render the modlel, but causes a crash on my laptop. This is because of some font issues with libharu. Retry on more recent OS's first to see if that fixes things.
	//https://sourceforge.net/p/witty/mailman/message/30272114/ //Won't work on Debian for now
	//http://witty-interest.narkive.com/1FcaBlfE/wt-interest-wpdfimage-error-102b
}
#endif

void EarUI::updateInputs() {	
	//this->log("debug") <<"Updating inputs";
	zmq::socket_t *socket = zmq_conn::connect();
	Wt::Json::Object responses;
	responses = zmq_conn::interact(std::string("inputs?"), socket);
	Wt::Json::Array inputSettings;
	Wt::WSlider *sliderWidget;
	Wt::WText *textWidget;
	for (auto name: responses.names()) {
	 	if (name == "pos") {
			if (this->posSliderMoving)
				continue;
		}
		inputSettings = responses.get(name);
		sliderWidget = inputSliders[name];
		if (sliderWidget->value() != int(inputSettings[2]))
			sliderWidget->setValue(inputSettings[2]);
		textWidget = inputTexts[name];
		textWidget->setText(sliderWidget->valueText());

	}
	Wt::Json::Object response;
	response = zmq_conn::interact("track?", socket);
	
	int server_track_idx = response.get("current");
	if (ui_track_idx != server_track_idx) {
		loadFragments(markerTree, false, socket);

		TimeWidget *firstW = (*children_as_vector(markerTree->tree()->treeRoot()).begin())->startWidget ;
		TimeWidget *lastW = (*children_as_vector(markerTree->tree()->treeRoot()).rbegin())->stopWidget;
		posSlider->setMinimum(firstW->time());
		posSlider->setMaximum(lastW->time());

		ui_track_idx = server_track_idx;
	}	
	zmq_conn::disconnect(socket);
}

int main(int argc, char **argv) {
	Wt::WString::setDefaultEncoding(Wt::CharEncoding::UTF8);
	return Wt::WRun(argc, argv, [](Wt::WEnvironment const &env) {
		return std::make_unique <EarUI> (env);
	});
}
