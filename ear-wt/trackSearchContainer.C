#include "trackSearchContainer.h"

TrackSearchContainer::TrackSearchContainer(EarUI *parentUI)
{
	this->parentUI = parentUI;
	//Wt::WContainerWidget *trackSearchContainer = new Wt::WContainerWidget(); 
	//resize(parentUI->width,500);
	//Make this a GridLayout and add a second column showing the track settings TODO

	auto vbox = setLayout(std::make_unique <Wt::WVBoxLayout> ());

	auto searchBox = vbox->addWidget(std::make_unique <Wt::WLineEdit> ());
	searchBox->setPlaceholderText("Type to search");
	auto trackSelectionBox = vbox->addWidget(std::make_unique <Wt::WSelectionBox> (), 1);
	trackModel = std::make_shared <FilteredStringModel> ();
	searchBox->textInput().connect(std::bind([=] () {
		trackModel->searchString = searchBox->text();
		parentUI->log("notice")<<"Updating track search model";
		trackModel->update();
		trackSelectionBox->setModel(trackModel);
		trackSelectionBox->setCurrentIndex(0);
		parentUI->log("notice")<<"Updated track search model";
	}));

	trackModel->update();
	trackSelectionBox->setModel(std::shared_ptr <Wt::WAbstractItemModel> (trackModel));
	trackSelectionBox->setVerticalSize(10);
	trackSelectionBox->setSelectionMode(Wt::SelectionMode::Single);
	auto selectButton = vbox->addWidget(std::make_unique <Wt::WPushButton> ("Select track"));
	selectButton->clicked().connect(std::bind([=] () {
		int row = trackSelectionBox->currentIndex();
		int tracknumber = Wt::cpp17::any_cast <int> (trackModel->data(trackModel->index(row,0),Wt::ItemDataRole::User));
		zmq_conn::interact(Wt::WString("track:"+std::to_string(tracknumber)));
		if (parentUI !=0)
		{
			parentUI->updateInputs();
		}
	}));

	auto filterContainer = vbox->addWidget(std::make_unique <WContainerWidget> ());
	auto searchContainer = filterContainer->addWidget(std::make_unique <WContainerWidget> ());
	auto filtersContainer = filterContainer->addWidget(std::make_unique <WContainerWidget> ());
	auto filterBox = searchContainer->addWidget(std::make_unique <Wt::WLineEdit> ());
	filterBox->setPlaceholderText("Filter"); 
	auto addFilter = searchContainer->addWidget(std::make_unique <Wt::WPushButton> ("Add"));
	addFilter->clicked().connect(std::bind([=] () {
		auto thisFilter = filtersContainer->addWidget(std::make_unique <Wt::WContainerWidget> ());
		Wt::WString filterName = filterBox->text();
		thisFilter->addWidget(std::make_unique <Wt::WText> (filterName));
		auto removeButton = thisFilter->addWidget(std::make_unique <Wt::WPushButton> ("X"));
		removeButton->clicked().connect(std::bind([=] () {
			filtersContainer->removeWidget(thisFilter);
			thisFilter->clear();
			std::vector<Wt::WString>::iterator pos = std::find(trackModel->musts.begin(), trackModel->musts.end(), filterName);
			if(pos!=trackModel->musts.end())
			{
				trackModel->musts.erase(pos);
				trackModel->update();
			}
		}));
		trackModel->musts.push_back(filterName);  
					trackModel->update();
		filterBox->setText("");
		filterBox->setPlaceholderText("Filter"); //TODO: Autofill/autocomplete
	}));
	auto refreshButton = vbox->addWidget(std::make_unique <Wt::WPushButton> ("Refresh database"));
	refreshButton->clicked().connect( std::bind([=] () {
		zmq_conn::interact("tracks_refresh?");
	}));
}
