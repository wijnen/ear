#ifndef _FilteredStringModel
#define _FilteredStringModel
#include <string>
#include <vector>
#include "earzmq.h"
#include <Wt/WApplication>
#include <Wt/WStringListModel>
#include <Wt/WString>
#include <Wt/WObject>
#include <Wt/Json/Array>
#include <Wt/Json/Parser>
#include <Wt/Json/Object>
#include <Wt/Json/Serializer>
#
class FilteredStringModel : public Wt::WStringListModel
{
 /*
Class to make a stringlistmodel that filters itsself based on filters set previously. Uses zeroMQ to talk to the Python Ear implementation and let Python (fuzzywuzzy) do the actual heavy lifting
*/
    public:
	FilteredStringModel(std::string zmqString = std::string("filteredTracks?"), WObject *parent=0); 
	std::vector<Wt::WString> musts;
	std::vector<Wt::WString> mays;
	Wt::WString searchString;
	void update();
	Wt::WStringListModel parent;
    private:
	Wt::WString zmqString;

};
#endif
