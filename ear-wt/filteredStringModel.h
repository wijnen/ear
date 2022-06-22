#ifndef _FilteredStringModel
#define _FilteredStringModel
#include <string>
#include <vector>
#include "earzmq.h"
#include <Wt/WApplication.h>
#include <Wt/WStringListModel.h>
#include <Wt/WString.h>
#include <Wt/WObject.h>
#include <Wt/Json/Array.h>
#include <Wt/Json/Parser.h>
#include <Wt/Json/Object.h>
#include <Wt/Json/Serializer.h>
#
class FilteredStringModel : public Wt::WStringListModel
{
 /*
Class to make a stringlistmodel that filters itsself based on filters set previously. Uses zeroMQ to talk to the Python Ear implementation and let Python (fuzzywuzzy) do the actual heavy lifting
*/
    public:
	FilteredStringModel(std::string zmqString = std::string("filteredTracks?")); 
	std::vector<Wt::WString> musts;
	std::vector<Wt::WString> mays;
	Wt::WString searchString;
	void update();
	Wt::WStringListModel parent;
    private:
	Wt::WString zmqString;

};
#endif
