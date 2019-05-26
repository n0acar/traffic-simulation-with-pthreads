#include <pthread.h>
#include <queue>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include "Policelog.h"
#include "Car.h"
#include "pthread_sleep.c"
#include <iostream>
#include <string>
#include<algorithm>
using namespace std;
using std::queue;
using std::max;

Policelog::Policelog(int time, int event){
	setTime(time);
	getTime();
	setEvent(event);
	getEvent();
}


void Policelog::setTime(int time2){
    time=time2;
}
void Policelog::setEvent(int event2){
    event=event2;
}

int Policelog::getTime(){
    return time;
}
int Policelog::getEvent(){
    return event;
}

// Queues for each lane
queue <Car> N;
queue <Car> S;
queue <Car> W;
queue <Car> E;

Car::Car(int carID, char direction, int arrivalTime, int crossTime, int waitTime)
{
    setID(carID);
    setDirection(direction);
    setArrivalTime(arrivalTime);
    setCrossTime(crossTime);
   	setWaitTime(waitTime);
	getID();
	getDirection();
	getArrivalTime();
	getCrossTime();
	getWaitTime();
}
/// SETTERS
void Car::setID(int ID){
    carID = ID;
}
void Car::setDirection(char dir){
    direction=dir;
}
void Car::setArrivalTime(int arr){
    arrivalTime=arr;
}
void Car::setCrossTime(int cross){
    crossTime=cross;
}
void Car::setWaitTime(int wait){
    waitTime=wait;
}
/// GETTERS
int Car::getID(){
    return carID;
}
char Car::getDirection(){
    return direction;
}
int Car::getArrivalTime(){
    return arrivalTime;
}
int Car::getCrossTime(){
    return crossTime;
}
int Car::getWaitTime(){
    return waitTime;
}

// SIGNATURES
void *laneAction(void* id);
void arriveRandomly(double probability);
void snapshot(int snapshotTime);
void initializeLanes();
// TODO: CHANGE ID HERE TO SOME OTHER PARAMETER
void *policeAction(void* id);
int pthread_sleep (int seconds);
void crossIntersection(queue <Car> q);
string secondsToTime(int seconds);
int getTimeSlice(int seconds, int choice);

//GLOBAL VARIABLES
int nextID=0;
double probability=0;
long policeChoice=0;
int simulationTime=0;
time_t initialTime = time(NULL);

//MUTEX
pthread_mutex_t creation_mutex= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t police_mutex= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t police_log_mutex= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t honk_cv =PTHREAD_COND_INITIALIZER;

// Essentials for logs
Car carLog[100];
Policelog policeLog[100];

int numberOfCrosses=0;
int numPhoneAndHonk=0;

int main(int argc, char *argv[]){

	// Arguments: simulationTime, probability, snapshotTime, seed


	simulationTime=atoi(argv[1]);
	probability=atof(argv[2])*100.0;
	// snapshotTime in seconds
	int snapshotTime=atoi(argv[3]);
	int seed=atoi(argv[4]);

	//seed determination
	srand(seed);


	FILE *carOutput = fopen("car.log","w+");
	FILE *policeOutput = fopen("police.log","w+");
	fprintf(carOutput,"CarID\tDirection\tArrival-Time\tCross-Time\tWait-Time \n");
	fprintf(policeOutput,"Time\tEvent\t\n");

	// MUTEX Initialization
	pthread_mutex_init(&creation_mutex, NULL);
    	pthread_mutex_init(&police_mutex, NULL);
	pthread_mutex_init(&police_log_mutex, NULL);
	//TODO: Last parameter of police
	pthread_t police;
	pthread_t n_thread,e_thread,s_thread,w_thread;


	initialTime = time(NULL);
	int tempTime = snapshotTime;
	

	initializeLanes();
	pthread_create(&police,NULL,policeAction,(void*) policeChoice);
	pthread_create(&n_thread,NULL,laneAction,(void*) 1); //northID determines if it is sleeping or sending cars
	pthread_create(&s_thread,NULL,laneAction,(void*) 2);
	pthread_create(&w_thread,NULL,laneAction,(void*) 3);
	pthread_create(&e_thread,NULL,laneAction,(void*) 4);
	pthread_cond_init (&honk_cv, NULL);

	do{	
		pthread_sleep(1);
		if(snapshotTime+initialTime == time(NULL) || snapshotTime+initialTime+1 == time(NULL) || snapshotTime+initialTime+2 == time(NULL)){
			snapshot(time(NULL));
			if(snapshotTime+initialTime+2 == time(NULL)) snapshotTime+=tempTime;
		}
	}while(time(NULL)<simulationTime+initialTime);

	pthread_join(police, NULL);
	pthread_join(n_thread, NULL);
	pthread_join(s_thread, NULL);
	pthread_join(w_thread, NULL);
	pthread_join(e_thread, NULL);

	pthread_mutex_destroy(&creation_mutex);
        pthread_mutex_destroy(&police_mutex);
	pthread_cond_destroy (&honk_cv);
	pthread_mutex_destroy (&police_log_mutex);

	// Write to the logs
	for(int ind = 0; ind < numberOfCrosses; ind++){
		fprintf(carOutput,"%d\t",carLog[ind].getID());
		fprintf(carOutput,"%c\t\t",carLog[ind].getDirection());
		fprintf(carOutput,"%d:%d:%d\t\t",getTimeSlice(carLog[ind].getArrivalTime(),0),
						getTimeSlice(carLog[ind].getArrivalTime(),1),
						getTimeSlice(carLog[ind].getArrivalTime(),2));
		fprintf(carOutput,"%d:%d:%d\t\t",getTimeSlice(carLog[ind].getCrossTime(),0),
						getTimeSlice(carLog[ind].getCrossTime(),1),
						getTimeSlice(carLog[ind].getCrossTime(),2));
		fprintf(carOutput,"%d\n",carLog[ind].getWaitTime());
	}

		//TODO: Change time
	for(int ind = 0; ind < numPhoneAndHonk; ind++){
		string event=" ";
		if(policeLog[ind].getEvent()==0)
			fprintf(policeOutput,"%d:%d:%d\tCell Phone\n",
							getTimeSlice(policeLog[ind].getTime(),0),
							getTimeSlice(policeLog[ind].getTime(),1),
							getTimeSlice(policeLog[ind].getTime(),2));
		else 
			fprintf(policeOutput,"%d:%d:%d\tHonk\n",
							getTimeSlice(policeLog[ind].getTime(),0),
							getTimeSlice(policeLog[ind].getTime(),1),
							getTimeSlice(policeLog[ind].getTime(),2));
		
	}
}
void *laneAction(void *id){
	int myID = (long)id;
	int northWaitStart=0;
	int northID=1;
	int northIDFlag=1;
	do{
	pthread_sleep(1);
	
	
	if (northIDFlag==1 && northID==0) {
			northIDFlag=0;
			northWaitStart=time(NULL);

	}
	if(northWaitStart + 20 == time(NULL)) {
			northID=5;
			northIDFlag=1;
	}
	if (myID==1 && northID==0);
	else if (myID==1&&northID==5) {
		pthread_mutex_lock(&creation_mutex);
		N.push(Car(nextID++, 'N',time(NULL), 0, 0));
		pthread_mutex_unlock(&creation_mutex);
		northID=1;
		cout<<"new N is entered";
	}
	else if (myID==1 && northID==1) {
		double rand_N = rand() % 100 +1;
		if (rand_N >= probability) {
			pthread_mutex_lock(&creation_mutex);
			N.push(Car(nextID++, 'N',time(NULL), 0, 0));
			
			if(numPhoneAndHonk%2==1){
 				pthread_cond_signal(&honk_cv);
				pthread_mutex_lock(&police_log_mutex);
				
					policeLog[numPhoneAndHonk]=Policelog(time(NULL),1);
					numPhoneAndHonk++;
				
				pthread_mutex_unlock(&police_log_mutex);
			}
			pthread_mutex_unlock(&creation_mutex);
			northID=1;
		}
		else {
			northID=0;
		}
	}
	if (myID==2) {

		double rand_S = rand() % 100 +1;
		if (rand_S <= probability) {

			pthread_mutex_lock(&creation_mutex);
			S.push(Car(nextID++, 'S',time(NULL), 0, 0));
		
			if(numPhoneAndHonk%2==1){
				pthread_cond_signal(&honk_cv);
				pthread_mutex_lock(&police_log_mutex);
				
					policeLog[numPhoneAndHonk]=Policelog(time(NULL),1);
					numPhoneAndHonk++;
				
				pthread_mutex_unlock(&police_log_mutex);
			}
			pthread_mutex_unlock(&creation_mutex);
		}
	}
	if (myID==3) {
		double rand_W = rand() % 100 +1;
		if (rand_W <= probability) {

			pthread_mutex_lock(&creation_mutex);
			W.push(Car(nextID++, 'W',time(NULL), 0, 0));
			
			if(numPhoneAndHonk%2==1){
				pthread_cond_signal(&honk_cv);
				pthread_mutex_lock(&police_log_mutex);
						
				policeLog[numPhoneAndHonk]=Policelog(time(NULL),1);
				numPhoneAndHonk++;
								
				pthread_mutex_unlock(&police_log_mutex);
			}
			pthread_mutex_unlock(&creation_mutex);	
		}
	}
	if (myID==4) {
		double rand_E = rand() % 100 +1;
		if (rand_E <= probability) {

			pthread_mutex_lock(&creation_mutex);
			E.push(Car(nextID++, 'E',time(NULL), 0, 0));
			
			if(numPhoneAndHonk%2==1){
				pthread_cond_signal(&honk_cv);
				pthread_mutex_lock(&police_log_mutex);
				
					policeLog[numPhoneAndHonk]= Policelog(time(NULL),1);
					numPhoneAndHonk++;
				
				pthread_mutex_unlock(&police_log_mutex);
			}
			pthread_mutex_unlock(&creation_mutex);
		}
	
	}
	}while(time(NULL)<simulationTime+initialTime);

}

// TODO: YOU MAY WANT TO CHANGE ID TO ANOTHER PARAMETER
void *policeAction(void *id){

    cout<<"police choice:";
    
    do {pthread_sleep(1);
    cout<<"\n";
    int choice = policeChoice;
    pthread_mutex_lock(&creation_mutex);
    if (N.size()!=0) N.front().setWaitTime(time(NULL)-N.front().getArrivalTime());
    if (S.size()!=0) S.front().setWaitTime(time(NULL)-S.front().getArrivalTime());
    if (E.size()!=0) E.front().setWaitTime(time(NULL)-E.front().getArrivalTime());
    if (W.size()!=0) W.front().setWaitTime(time(NULL)-W.front().getArrivalTime());	
    pthread_mutex_unlock(&creation_mutex);
    int nSize=N.size();
    int sSize=S.size();
    int wSize=W.size();
    int eSize=E.size();
    int maxCars;
    maxCars= max(max(nSize,sSize),max(eSize,wSize));


    if(maxCars==0) {
	 //max cars is 0 tell police to be warned about honks
		pthread_mutex_lock(&police_log_mutex);	
	 	policeLog[numPhoneAndHonk]=Policelog(time(NULL),0);
		numPhoneAndHonk++;
		pthread_mutex_unlock(&police_log_mutex);
		pthread_cond_wait(&honk_cv,&creation_mutex);
		pthread_mutex_unlock(&creation_mutex);
		pthread_sleep(3);
	}

    else if (maxCars !=0) {
    if (choice == 0) {
			if (maxCars == nSize) {
				cout << "before" <<N.size();
				crossIntersection(N);
				pthread_mutex_lock(&creation_mutex);
				N.pop();
				pthread_mutex_unlock(&creation_mutex);

				if (N.size() == 0 || S.size() >= 5 || E.size() >= 5 || W.size() >= 5 ){
					policeChoice=0;
					if (N.front().getWaitTime()>=20)
						policeChoice=1;

					else if (E.front().getWaitTime()>=20)
						policeChoice=2;

					else if (S.front().getWaitTime()>=20)
						policeChoice=3;

					else if (W.front().getWaitTime()>=20)
						policeChoice=4;


				}

				else {
					policeChoice = 1;
					if (N.front().getWaitTime()>=20)
						policeChoice=1;

					else if (E.front().getWaitTime()>=20)
						policeChoice=2;

					else if (S.front().getWaitTime()>=20)
						policeChoice=3;

					else if (W.front().getWaitTime()>=20)
						policeChoice=4;

				}

	    		}
	    	else if (maxCars == eSize) {

					crossIntersection(E);
					E.pop();

					if (E.size() == 0 || S.size() >= 5 || N.size() >= 5 || W.size() >= 5) {
						policeChoice=0;
						if (N.front().getWaitTime()>=20)
							policeChoice=1;

						else if (E.front().getWaitTime()>=20)
							policeChoice=2;

						else if (S.front().getWaitTime()>=20)
							policeChoice=3;

						else if (W.front().getWaitTime()>=20)
							policeChoice=4;

					}
					else {
						 policeChoice = 2;
						 if (N.front().getWaitTime()>=20)
							policeChoice=1;

						else if (E.front().getWaitTime()>=20)
							policeChoice=2;

						else if (S.front().getWaitTime()>=20)
							policeChoice=3;

						else if (W.front().getWaitTime()>=20)
							policeChoice=4;

					}

	    		}
    		else if (maxCars == sSize) {

				crossIntersection(S);
				pthread_mutex_lock(&creation_mutex);
				S.pop();
				pthread_mutex_unlock(&creation_mutex);
				

				if (S.size() == 0 || N.size() >= 5 || E.size() >= 5 || W.size() >= 5){
					policeChoice=0;
					if (N.front().getWaitTime()>=20)
						policeChoice=1;

					else if (E.front().getWaitTime()>=20)
						policeChoice=2;

					else if (S.front().getWaitTime()>=20)
						policeChoice=3;

					else if (W.front().getWaitTime()>=20)
						policeChoice=4;

				}
				else{

					policeChoice = 3;
					if (N.front().getWaitTime()>=20)
						policeChoice=1;

					else if (E.front().getWaitTime()>=20)
						policeChoice=2;

					else if (S.front().getWaitTime()>=20)
						policeChoice= 3;

					else if (W.front().getWaitTime()>=20)
						policeChoice= 4;

				}
    		}
    		else if (maxCars == wSize) {
			crossIntersection(W);
			pthread_mutex_lock(&creation_mutex);
			W.pop();
			pthread_mutex_unlock(&creation_mutex);

			if (W.size() == 0 || S.size() >= 5 || E.size() >= 5 || N.size() >= 5){
				policeChoice=0;
				if (N.front().getWaitTime()>=20)
					policeChoice= 1;

				else if (E.front().getWaitTime()>=20)
					policeChoice= 2;

				else if (S.front().getWaitTime()>=20)
					policeChoice= 3;

				else if (W.front().getWaitTime()>=20)
					policeChoice=4;

			}
			else {
				policeChoice = 4;
				if (N.front().getWaitTime()>=20)
					policeChoice=1;

				else if (E.front().getWaitTime()>=20)
					policeChoice=2;

				else if (S.front().getWaitTime()>=20)
					policeChoice=3;

				else if (W.front().getWaitTime()>=20)
					policeChoice=4;

			}
    		}
	}
	else {
		if(choice == 1) {

			crossIntersection(N);
			pthread_mutex_lock(&creation_mutex);
			N.pop();
			pthread_mutex_unlock(&creation_mutex);
			if (N.size() == 0 || S.size() >= 5 || E.size() >= 5 || W.size() >= 5) {

				policeChoice=0;
				if (N.front().getWaitTime()>=20)
					policeChoice=1;

				else if (E.front().getWaitTime()>=20)
					policeChoice=2;

				else if (S.front().getWaitTime()>=20)
					policeChoice=3;

				else if (W.front().getWaitTime()>=20)
					policeChoice=4;

			}
			else {
				policeChoice = 1;
				if (N.front().getWaitTime()>=20)
					policeChoice=1;

				else if (E.front().getWaitTime()>=20)
					policeChoice=2;

				else if (S.front().getWaitTime()>=20)
					policeChoice=3;

				else if (W.front().getWaitTime()>=20)
					policeChoice=4;

			}
		}
		else if(choice == 2) {

			crossIntersection(E);
			pthread_mutex_lock(&creation_mutex);
			E.pop();
			pthread_mutex_unlock(&creation_mutex);

			if (E.size() == 0 || S.size() >= 5 || N.size() >= 5 || W.size() >= 5) {
				policeChoice=0;
				if (N.front().getWaitTime()>=20)
					policeChoice=1;

				else if (E.front().getWaitTime()>=20)
					policeChoice=2;

				else if (S.front().getWaitTime()>=20)
					policeChoice=3;

				else if (W.front().getWaitTime()>=20)
					policeChoice=4;

			}
			else {
				policeChoice = 2;
				if (N.front().getWaitTime()>=20)
					policeChoice=1;

				else if (E.front().getWaitTime()>=20)
					policeChoice=2;

				else if (S.front().getWaitTime()>=20)
					policeChoice=3;

				else if (W.front().getWaitTime()>=20)
					policeChoice=4;

			}
		}
		else if(choice == 3) {

			crossIntersection(S);
			pthread_mutex_lock(&creation_mutex);
			S.pop();
			pthread_mutex_unlock(&creation_mutex);

			if (S.size() == 0 || N.size() >= 5 || E.size() >= 5 || W.size() >= 5) {
				policeChoice=0;
				if (N.front().getWaitTime()>=20)
					policeChoice=1;

				else if (E.front().getWaitTime()>=20)
					policeChoice=2;

				else if (S.front().getWaitTime()>=20)
					policeChoice=3;

				else if (W.front().getWaitTime()>=20)
					policeChoice=4;

			}
			else {
				policeChoice = 3;
				if (N.front().getWaitTime()>=20)
					policeChoice=1;

				else if (E.front().getWaitTime()>=20)
					policeChoice=2;

				else if (S.front().getWaitTime()>=20)
					policeChoice=3;

				else if (W.front().getWaitTime()>=20)
					policeChoice=4;

			}
		}
		else if(choice == 4) {

			crossIntersection(W);
			pthread_mutex_lock(&creation_mutex);
			W.pop();
			pthread_mutex_unlock(&creation_mutex);

			if (W.size() == 0 || S.size() >= 5 || E.size() >= 5 || N.size() >= 5) {
				policeChoice=0;
				if (N.front().getWaitTime()>=20)
					policeChoice=1;

				else if (E.front().getWaitTime()>=20)
					policeChoice=2;

				else if (S.front().getWaitTime()>=20)
					policeChoice=3;

				else if (W.front().getWaitTime()>=20)
					policeChoice=4;

			}
			else{
				 policeChoice = 4;
				if (N.front().getWaitTime()>=20)
					policeChoice=1;

				else if (E.front().getWaitTime()>=20)
					policeChoice=2;

				else if (S.front().getWaitTime()>=20)
					policeChoice=3;

				else if (W.front().getWaitTime()>=20)
					policeChoice=4;

			}
		}
	}
	}

	cout<<"\n N waiting time\n";
	cout<<N.front().getWaitTime();
        cout<<"\n N waiting time\n";
	
	}while(time(NULL)<simulationTime+initialTime);

	cout<<"next police choice:";
    	cout<<policeChoice;
	cout<<"\n";
}

// The method for lanes to perform
void crossIntersection(queue <Car> q){

	carLog[numberOfCrosses]=q.front();
	carLog[numberOfCrosses].setCrossTime(time(NULL));
	carLog[numberOfCrosses].setWaitTime(carLog[numberOfCrosses].getCrossTime()-
						carLog[numberOfCrosses].getArrivalTime());
	numberOfCrosses++;
	cout<<numberOfCrosses;
}

void initializeLanes(){
	N.push(Car(nextID++, 'N',time(NULL), 0, 0));
	S.push(Car(nextID++, 'S',time(NULL), 0, 0));
	W.push(Car(nextID++, 'W',time(NULL), 0, 0));
	E.push(Car(nextID++, 'E',time(NULL), 0, 0));

}

// Terminal output for snapshot
void snapshot(int snapshotTime){
	cout<<"-------------------------------------\n";
	cout<<"At "<<secondsToTime(snapshotTime)<<"\n";
	printf("   %lu    \n", N.size());
        printf("%lu     %lu\n", W.size(),E.size());
	printf("   %lu    \n", S.size());
	cout<<"-------------------------------------\n";
}

int getTimeSlice(int seconds, int choice){
	time_t now;
	now= seconds;
	struct tm *time;
	time=localtime(&now);
	if(choice==0) return time->tm_hour;
	else if (choice==1) return time->tm_min;
	else return time->tm_sec;
	}

//Method for converting seconds to hour, minute and seconds
string secondsToTime(int seconds){
	string timeFormat("");

	timeFormat = to_string(getTimeSlice(seconds,0))+":"+ to_string(getTimeSlice(seconds,1))+":"+ to_string(getTimeSlice(seconds,2));

	return timeFormat;

}
