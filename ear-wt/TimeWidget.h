// Copyright 2022 Bas Wijnen <wijnen@debian.org>

#ifndef _TimeWidget
#define _TimeWidget


#include <Wt/WText.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WString.h>
#include <sstream>
#include <iomanip>

class TimeWidget : public Wt::WText //This explicitly does not use the Wt::WTime class, as we want to use milliseconds as our base unit, especially when communicating with the Python side of this project. The Wt::Wtime supports only already quite correctly formatted times, and we don't need that. Additionally, it's meant for clock times, not intervals
{
    public:
	TimeWidget();
	//TimeWidget(double time);
	long long time();
	bool setTime(long time);
	static Wt::WString zero_pad(int val, int width);
    private:
	long _time;
};

#endif
