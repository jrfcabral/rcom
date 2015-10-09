#pragma once 
#include <termios.h>
#include <stdio.h>

unsigned int info;

#define FALSE 0
#define TRUE 1

#define MAX_SIZE 256
#define BIT(n) (0x01 << n)


typedef enum {
	SEND, RECEIVE
} ConnnectionMode;

typedef struct{
	char port[50] //port : format /dev/ttySx
	int baudRate;
	int msgMaxSize;
	info ns; //frame sequece number
	info timeOut; //timeout value
	info numTries //number of retries if it fails

	char frame[MAX_SIZE]

	//termios

	struct termios oldtio, newtio;
	


}LinkLayer;

LinkLayer ll;

