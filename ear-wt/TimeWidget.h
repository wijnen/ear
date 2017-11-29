#ifndef _TimeWidget
#define _TimeWidget


#include <Wt/WText>
#include <Wt/WContainerWidget>
#include <Wt/WString>
#include <sstream>
#include <iomanip>

class TimeWidget : public Wt::WText //This explicitly does not use the Wt::WTime class, as we want to use milliseconds as our base unit, especially when communicating with the Python side of this project. The Wt::Wtime supports only already quite correctly formatted times, and we don't need that. Additionally, it's meant for clock times, not intervals
{
    public:
	TimeWidget(Wt::WContainerWidget *parent = 0);
//  TimeWidget(double time, WContainerWidget *parent = 0);
	long long time();
	bool setTime(long time);
	static Wt::WString zero_pad(int val, int width);
    private:
	long _time;
};

#endif
