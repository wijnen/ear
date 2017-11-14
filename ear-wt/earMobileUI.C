
#include <Wt/WLogger>
#include <Wt/WBootstrapTheme>
#include <Wt/WApplication>
#include <Wt/WContainerWidget>
#include <Wt/WTreeTable>
#include <Wt/WTimer>
#include <Wt/Json/Object>
#include "earzmq.h"
#include "fragmentTree.h"




class EarMobileUI : public Wt::WApplication
{
public:
  EarMobileUI(const Wt::WEnvironment& env);
  void updateInputs();
  int ui_track_idx;
private:
  Wt::WTreeTable *fragmentTree;
};


EarMobileUI::EarMobileUI(const Wt::WEnvironment& env)
  : Wt::WApplication(env)
{
    setTitle("Ear interface"); 
    setTheme(new Wt::WBootstrapTheme());
    Wt::WContainerWidget *fragmentContainer = new Wt::WContainerWidget(root());
    fragmentTree = new Wt::WTreeTable(fragmentContainer);
    fragmentTree->addColumn("",500);
    Wt::WTimer *inputtimer = new Wt::WTimer();  
   inputtimer->setInterval(2500);
   inputtimer->timeout().connect(std::bind([=] ()
   {
       updateInputs();
   }));
   inputtimer->start();


 
}
void EarMobileUI::updateInputs()
{	
	Wt::Json::Object response;
	response=zmq_conn::interact("track?");
	int server_track_idx = response.get("current");
	if(ui_track_idx != server_track_idx)
	{
		loadFragments(fragmentTree, true);
		ui_track_idx = server_track_idx;
	}	
		
}


Wt::WApplication *createApplication(const Wt::WEnvironment& env)
{
  return new EarMobileUI(env);
}




int main(int argc, char **argv)
{
    Wt::WString::setDefaultEncoding(Wt::UTF8);
    return Wt::WRun(argc, argv, &createApplication);
}

