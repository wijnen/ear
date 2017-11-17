
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
  Wt::WPushButton *playPauseButton;
  TimeWidget *posText;
};


EarMobileUI::EarMobileUI(const Wt::WEnvironment& env)
  : Wt::WApplication(env)
{
    setTitle("Ear interface"); 
    Wt::WBootstrapTheme *theme = new Wt::WBootstrapTheme();
    theme->setResponsive(true);
    theme->setVersion(Wt::WBootstrapTheme::Version3); 
    this->removeMetaHeader(Wt::MetaHeaderType::MetaName,"viewport");
    this->addMetaHeader("viewport",
			   "width=device-width, height=device-height, initial-scale=2");
    setTheme(theme);
    
    Wt::WContainerWidget *buttonContainer = new Wt::WContainerWidget(root());
    playPauseButton = new Wt::WPushButton("Play from start",buttonContainer);
    playPauseButton->clicked().connect(std::bind([=] ()
    {
	zmq_conn::interact(std::string("event:pause"));
    }));
    Wt::WPushButton *stopButton = new Wt::WPushButton("Stop",buttonContainer); 
    stopButton->clicked().connect(std::bind([=] ()
    {
	zmq_conn::interact(std::string("event:stop"));
    }));
    stopButton->setMargin(5, Wt::Left);
    posText = new TimeWidget(buttonContainer);
    posText->setMargin(5, Wt::Left);
   




    Wt::WContainerWidget *fragmentContainer = new Wt::WContainerWidget(root());
    fragmentTree = new Wt::WTreeTable(fragmentContainer);
//    fragmentTree->addColumn("",500);

   Wt::WTimer *inputtimer = new Wt::WTimer();  
   inputtimer->setInterval(2500);
   inputtimer->timeout().connect(std::bind([=] ()
   {
       updateInputs();
   }));
   inputtimer->start();
   updateInputs();
    Wt::WTimer *timer = new Wt::WTimer();  
    timer->setInterval(100);
    timer->timeout().connect(std::bind([=] ()
    {
	Wt::Json::Object posj ;
	bool playing;
	Wt::Json::Object playingj;


	zmq::socket_t *socket = zmq_conn::connect();
	posj = zmq_conn::interact(std::string("pos?"),socket);
	playingj = zmq_conn::interact(std::string("playing?"),socket);
	zmq_conn::disconnect(socket);


	Wt::Json::Value posjv = posj.get("pos");	
	const long long track_time = posjv;
	
	posText->setTime(track_time);
	mark_current_fragment(fragmentTree,track_time);
	playing = playingj.get("playing");
	if (playing)
	{
		playPauseButton->setText("Pause");
	}
	else
	{
		if(track_time > 0)
		{
			playPauseButton->setText("Continue");
		}
		else
		{
			playPauseButton->setText("Play from start");
		}
	}
    }));
    timer->start();





 
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

