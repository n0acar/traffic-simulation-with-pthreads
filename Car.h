#ifndef CAR_H
#define CAR_H

class Car
{
    public:
        Car(int=0,char=' ',int=0,int=0,int=0);

		int carID;
        char direction;
        int arrivalTime;
        int crossTime;
		int waitTime;

		int getID();
		char getDirection();
		int getArrivalTime();
		int getCrossTime();
		int getWaitTime();

		void setID(int);
	    void setDirection(char);
	    void setArrivalTime(int);
	    void setCrossTime(int);
	   	void setWaitTime(int);

    };

#endif
