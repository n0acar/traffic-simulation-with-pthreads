#ifndef POLICELOG_H
#define POLICELOG_H
#include <string>
using std::string;

class Policelog
{
    public:
        Policelog(int=0,int=0);
	int time;
        int event;

	int getTime();
	int getEvent();

	void setTime(int);
	void setEvent(int);
    };

#endif
