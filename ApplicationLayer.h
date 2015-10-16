/*Application Layer contains the communication protocols and interface methods used by hosts. */

#pragma once


#include "Status.h"
#include <stdio.h>
#include <termios.h>

typedef struct {
	
	int fd; //file descriptor of the serial port
	Status status; /*TRANSMITTER/RECEIVER*/

	char* fileName; //file to be sent
} ApplicationLayer;

extern int* al;
