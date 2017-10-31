
#include <Wt/WLogger>
#include <Wt/WBootstrapTheme>
#include <Wt/WApplication>



class EarMobileUI : public WApplication
{
public:
  EarMobileUI(const WEnvironment& env);


};


EarUI::EarUI(const WEnvironment& env)
  : WApplication(env)
{
    setTitle("Ear interface"); 
//setCssTheme("polished"); //This fixes the columns of the treetable, soimehow
setTheme(new WBootstrapTheme());
 
}

int main(int argc, char **argv)
{
    zmq_port = getenv("ZMQ_PORT");
    WString::setDefaultEncoding(UTF8);
    return WRun(argc, argv, &createApplication);
}

