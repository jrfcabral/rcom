#include "alarm.h"
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

int alarmOff = 0;

void alarmHandler(int signal){
	if(signal != SIGALRM)
		return;

	alarmOFF = TRUE;
	//aqui é necessario incrementar os stats das laylinks(ll)
	printf("Connection is lost!\n Trying again:\n");
}


void setAlarm(){
	struct sigaction action; //this struct will examine and change the signal action
	action.sa_handler = alarmHandler; //action associated to the specified handler -> alarmHandler
	sigemptyset(&action.sa_mask); //initialized the signal set
	action.sa_flags = 0; //identifies a set of signales that will 
						 //will be added to the process before the handler is invoked.

sigaction(SIGALRM, &action, NULL);

alarmOFF = FALSE;


}


void stopAlarm(){
	struct sigaction action;
	action.sa_handler = NULL;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;

	sigaction(SIGALRM, &action, NULL);

	alarm(0);
}