
#include <Wt/WLogger>
#include <Wt/WBootstrapTheme>
#include <Wt/WApplication>
//#include <Wt/WContainerWidget>
#include <Wt/WTreeTable>
#include <Wt/WTimer>
#include <Wt/Json/Object>
//#include <Wt/WHBoxLayout>
//#include <Wt/WVBoxLayout>
#include <Wt/WTemplate>
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
    setTitle("Ear Mobile interface"); 
this->log("notice")<<"Making mobile UI";
    Wt::WBootstrapTheme *theme = new Wt::WBootstrapTheme();
///    theme->setResponsive(true);
    theme->setVersion(Wt::WBootstrapTheme::Version3); 
   // this->removeMetaHeader(Wt::MetaHeaderType::MetaName,"viewport");
    //this->addMetaHeader("viewport",
//			   "width=device-width, height=device-height, initial-scale=2");
//  this->addMetaHeader("viewport",
//			   "width=1024");
  setTheme(theme);
   

Wt::WTemplate *html = new Wt::WTemplate(Wt::WString(
"<div width='device-width' scale='4'   >"
	"<div>"
		"${start-button} ${stop-button} ${play-time}"
	"</div>"
	"<div>"
		"${fragment-tree}"
	"</div>"
"</div>"
),root());

 
this->log("notice")<<"Making button container";
    //Wt::WContainerWidget *buttonContainer = new Wt::WContainerWidget(root());
    //buttonContainer->setMaximumSize(500,Wt::WLength::Auto);
    //Wt::WHBoxLayout *buttonBox = new Wt::WHBoxLayout();
    //buttonContainer->setLayout(buttonBox);
    playPauseButton = new Wt::WPushButton("Play from start");
    //buttonBox->addWidget(playPauseButton);
    playPauseButton->clicked().connect(std::bind([=] ()
    {
this->log("notice")<<"interactnig to pause";
	zmq_conn::interact(std::string("event:pause"));
    }));
    html->bindWidget("start-button",playPauseButton);
    Wt::WPushButton *stopButton = new Wt::WPushButton("Stop"); 
    //buttonBox->addWidget(stopButton);
    html->bindWidget("stop-button",stopButton);

    stopButton->clicked().connect(std::bind([=] ()
    {
this->log("notice")<<"interactnig to stop";
	zmq_conn::interact(std::string("event:stop"));
    }));
    stopButton->setMargin(5, Wt::Left);
    posText = new TimeWidget();
    posText->setMargin(5, Wt::Left);
    html->bindWidget("play-time",posText);
    //buttonBox->addWidget(posText);
   


//    Wt::WContainerWidget *fragmentContainer = new Wt::WContainerWidget(root());
    fragmentTree = new Wt::WTreeTable();
 html->bindWidget("fragment-tree",fragmentTree);
//    fragmentTree->addColumn("",500);
//    fragmentTree->resize(
//			Wt::WLength(100,Wt::WLength::Unit::Percentage), //Width
//			Wt::WLength(80,Wt::WLength::Unit::Percentage)); //Heigth
/*   Wt::WTimer *inputtimer = new Wt::WTimer();   
   inputtimer->setInterval(2500); //For some reason, this actually segfaults (on a RPi) when it fires, before calling updateInputs. Calling updateInputs from any other place works, including the other timer. I'll try and find out the cause later, now it runs
   inputtimer->timeout().connect(std::bind([=] ()
   {
std::cout<< "updating inputs " <<std::endl;
       updateInputs();
   }));
   inputtimer->start();
*/

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
updateInputs();
    }));
    timer->start();





 
}
void EarMobileUI::updateInputs()
{	
	Wt::Json::Object response;
	response = zmq_conn::interact("track?");
	int server_track_idx = response.get("current");
	if(ui_track_idx != server_track_idx)
	{
		loadFragments(fragmentTree, true);
		ui_track_idx = server_track_idx;
	}	
		
}


Wt::WApplication *createApplication(const Wt::WEnvironment& env)
{
  Wt::WApplication *ui =  new EarMobileUI(env);
//    ui->addMetaHeader("viewport",
//			   "width=1024");


return ui;
}




int main(int argc, char **argv)
{
    Wt::WString::setDefaultEncoding(Wt::UTF8);
    return Wt::WRun(argc, argv, &createApplication);
}

