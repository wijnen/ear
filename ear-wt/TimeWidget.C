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
		Wt::WString out = Wt::WString("{1}:{2}:{3}").arg(zero_pad(minutes,2)).arg(zero_pad(seconds,2)).arg(zero_pad(milliseconds,3));
		if (negative)
		{
			this->setText(Wt::WString("- {1}").arg(out));
		}
		else
		{
			this->setText(out);
		}
	}
	return true;
}


Wt::WString TimeWidget::zero_pad(int val, int width)
{
	std::stringstream sstream;
	sstream<<std::setw(width)<<std::setfill('0')<<val;
	return Wt::WString(sstream.str());

}

TimeWidget::TimeWidget()
 : Wt::WText()
{  }  

long long TimeWidget::time()
{
	return this->_time;
}


