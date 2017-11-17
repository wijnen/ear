#include "trackSearchContainer.h"

TrackSearchContainer::TrackSearchContainer(EarUI *parentUI, Wt::WContainerWidget *parent) : Wt::WContainerWidget(parent)
{
this->parentUI = parentUI;
//Wt::WContainerWidget *trackSearchContainer = new Wt::WContainerWidget(); 
//resize(parentUI->width,500);
//Make this a GridLayout and add a second column showing the track settings TODO

Wt::WVBoxLayout *vbox = new Wt::WVBoxLayout();
setLayout(vbox); //TODO


Wt::WLineEdit *searchBox = new Wt::WLineEdit(); 
vbox->addWidget(searchBox);
searchBox->setPlaceholderText("Type to search");
Wt::WSelectionBox *trackSelectionBox = new Wt::WSelectionBox();
vbox->addWidget(trackSelectionBox,1);
FilteredStringModel *trackModel = new FilteredStringModel();
#ifndef OLD_WT
searchBox->textInput().connect(std::bind([=] ()
{
	trackModel->searchString = searchBox->text();
	parentUI->log("notice")<<"Updating track search model";
	trackModel->update();
	trackSelectionBox->setModel(trackModel);
	trackSelectionBox->setCurrentIndex(0);
	parentUI->log("notice")<<"Updated track search model";
}));
#endif
#ifdef OLD_WT
//<3.3.6 I think
searchBox->keyPressed().connect(std::bind([=] ()
{
	trackModel->searchString = searchBox->text();
	trackModel->update();
	trackSelectionBox->setModel(trackModel);
	trackSelectionBox->setCurrentIndex(0);
}));
#
#endif

trackModel->update();
trackSelectionBox->setModel(trackModel);
trackSelectionBox->setVerticalSize(10);
trackSelectionBox->setSelectionMode(Wt::SingleSelection);
Wt::WPushButton *selectButton = new Wt::WPushButton("Select track");
vbox->addWidget(selectButton);
selectButton->clicked().connect(std::bind([=] ()
{
	int row = trackSelectionBox->currentIndex();
	int tracknumber = boost::any_cast<int>(trackModel->data(trackModel->index(row,0),Wt::UserRole)); 
	zmq_conn::interact(Wt::WString("track:"+std::to_string(tracknumber)));
	if (parentUI !=0)
	{
		parentUI->updateInputs();
	}
}));

    Wt::WContainerWidget *filterContainer = new WContainerWidget(this);
vbox->addWidget(filterContainer);
    Wt::WContainerWidget *searchContainer = new WContainerWidget(filterContainer);
    Wt::WContainerWidget *filtersContainer = new WContainerWidget(filterContainer);
    Wt::WLineEdit *filterbox = new Wt::WLineEdit(searchContainer);
    filterbox->setPlaceholderText("Filter"); 
    Wt::WPushButton *addFilter = new Wt::WPushButton("Add",searchContainer);
    addFilter->clicked().connect(std::bind([=] ()
    {
	Wt::WContainerWidget *thisFilter = new Wt::WContainerWidget(filtersContainer); 
	Wt::WString filterName = filterbox->text();
	new Wt::WText(filterName,thisFilter);
	Wt::WPushButton *removeButton = new Wt::WPushButton("X",thisFilter);
		removeButton->clicked().connect(std::bind([=] ()
		{
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
	filterbox->setText("");
	filterbox->setPlaceholderText("Filter"); //TODO: Autofill/autocomplete
     }));
Wt::WPushButton *refreshButton = new Wt::WPushButton("Refresh database");
refreshButton->clicked().connect( std::bind([=] ()
{
	zmq_conn::interact("tracks_refresh?");
}));
vbox->addWidget(refreshButton);

}
