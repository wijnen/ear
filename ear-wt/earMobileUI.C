
#include <Wt/WLogger.h>
#include <Wt/WBootstrapTheme.h>
#include <Wt/WApplication.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WTreeTable.h>
#include <Wt/WTimer.h>
#include <Wt/WTemplate.h>
#include <Wt/Json/Object.h>
#include "earzmq.h"
#include "fragmentTree.h"

class EarMobileUI : public Wt::WApplication {
public:
	EarMobileUI(const Wt::WEnvironment& env);
	void updateInputs();
private:
	int ui_track_idx;
	Wt::WTreeTable *fragmentTree;
	Wt::WPushButton *playPauseButton;
	TimeWidget *posText;
	Wt::WTemplate *html;
};

EarMobileUI::EarMobileUI(const Wt::WEnvironment& env) : Wt::WApplication(env) {
	ui_track_idx = -1;
	setTitle("Ear Mobile interface"); 
	this->log("notice")<<"Making mobile UI";
	Wt::WBootstrapTheme *theme = new Wt::WBootstrapTheme();
	//    theme->setResponsive(true);
	theme->setVersion(Wt::BootstrapVersion::v3); 
	// this->removeMetaHeader(Wt::MetaHeaderType::MetaName,"viewport");
	//this->addMetaHeader("viewport",
	//			   "width=device-width, height=device-height, initial-scale=2");
	setTheme(std::shared_ptr <Wt::WTheme> (theme));

	html = root()->addWidget(std::make_unique <Wt::WTemplate> (Wt::WString(
	"<div width='device-width' scale='4'>"
		"<div>"
			"${start-button} ${stop-button} ${play-time}"
		"</div>"
		"<div>"
			"${fragment-tree}"
		"</div>"
	"</div>"
	)));

	this->log("notice")<<"Making button container";
	playPauseButton = html->bindWidget("start-button", std::make_unique <Wt::WPushButton> ("Play from start"));
	playPauseButton->clicked().connect(std::bind([=] () {
		this->log("notice")<<"interactnig to pause";
		zmq_conn::interact(std::string("event:pause"));
	}));

	auto stopButton = html->bindWidget("stop-button", std::make_unique <Wt::WPushButton> ("Stop"));
	stopButton->clicked().connect(std::bind([=] () {
		this->log("notice")<<"interactnig to stop";
		zmq_conn::interact(std::string("event:stop"));
	}));
	stopButton->setMargin(5, Wt::Side::Left);

	posText = html->bindWidget("play-time", std::make_unique <TimeWidget> ());
	posText->setMargin(5, Wt::Side::Left);

	auto fragmentContainer = html->bindWidget("fragment-tree", std::make_unique <Wt::WContainerWidget> ());
	fragmentTree = fragmentContainer->addWidget(std::make_unique <Wt::WTreeTable> ());
	//    fragmentTree->addColumn("",500);
	//    fragmentTree->resize(
	//			Wt::WLength(100,Wt::WLength::Unit::Percentage), //Width
	//			Wt::WLength(80,Wt::WLength::Unit::Percentage)); //Heigth
	/*   Wt::WTimer *inputtimer = new Wt::WTimer();   
	inputtimer->setInterval(2500); //For some reason, this actually segfaults (on a RPi) when it fires, before calling updateInputs. Calling updateInputs from any other place works, including the other timer. I'll try and find out the cause later, now it runs
	inputtimer->timeout().connect(std::bind([=] () {
		std::cout<< "updating inputs " <<std::endl;
		updateInputs();
	}));
	inputtimer->start();
	*/

	updateInputs();

	Wt::WTimer *timer = new Wt::WTimer();  
	timer->setInterval(std::chrono::milliseconds(100));
	timer->timeout().connect(std::bind([=] () {
		Wt::Json::Object posj ;
		bool playing;
		Wt::Json::Object playingj;

		zmq::socket_t *socket = zmq_conn::connect();
		posj = zmq_conn::interact(std::string("pos?"), socket);
		playingj = zmq_conn::interact(std::string("playing?"), socket);
		zmq_conn::disconnect(socket);

		Wt::Json::Value posjv = posj.get("pos");	
		const long long track_time = posjv;
		posText->setTime(track_time);
		mark_current_fragment(fragmentTree, track_time);
		playing = playingj.get("playing");
		if (playing) {
			playPauseButton->setText("Pause");
		}
		else
		{
			if(track_time > 0) {
				playPauseButton->setText("Continue");
			}
			else {
				playPauseButton->setText("Play from start");
			}
		}
		updateInputs();
	}));
	timer->start();
}

void EarMobileUI::updateInputs() {	
	Wt::Json::Object response;
	response = zmq_conn::interact("track?");
	int server_track_idx = response.get("current");
	if (ui_track_idx != server_track_idx) {
		loadFragments(fragmentTree, true);
		ui_track_idx = server_track_idx;
	}	

}

int main(int argc, char **argv) {
	Wt::WString::setDefaultEncoding(Wt::CharEncoding::UTF8);
	return Wt::WRun(argc, argv, [](Wt::WEnvironment const &env) {
		return std::make_unique <EarMobileUI> (env);
	});
}
