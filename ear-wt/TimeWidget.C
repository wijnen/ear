#include "TimeWidget.h"

bool TimeWidget::setTime(long time)
{
		this->_time = time; 
	if(time != -1)
	{
		bool negative = false;
		if (time < 0)
		{
			negative = true;
			time *= -1; 
		}
		int minutes, seconds, milliseconds;
		seconds =  ((time / 1000) % 60) ;
		minutes =  ((time / (1000*60)) % 60);
		milliseconds = time - seconds*1000 - minutes*60*1000; 
		if (negative)
		{
			this->setText(Wt::WString("- {1}:{2}:{3}").arg(minutes).arg(seconds).arg(milliseconds)); //TODO: zero-pad this string
		}
		else
		{
			this->setText(Wt::WString("{1}:{2}:{3}").arg(minutes).arg(seconds).arg(milliseconds)); //TODO: zero-pad this string
		}
	}
	return true;
}



TimeWidget::TimeWidget(Wt::WContainerWidget *parent )
 : Wt::WText(parent)
{  }  

long long TimeWidget::time()
{
	return this->_time;
}


