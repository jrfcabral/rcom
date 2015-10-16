#include "linklayer.h"
#include "ApplicationLayer.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "alarm.h"

const int FLAG = 0x7E;
const int A = 0x03;
const int E = 0x7D;

LinkLayer* l1;

struct termios oldtio, newtio; //old and new termios

int linkLayerInit(char* port, ConnectionMode mode, int baudrate, int msgMaxSize, int numTries, int timeOut){

}


int CurrentSettingsNewTermios(){

}

int saveSettings(){

}

int setTermios(){

	bzero(&l1->newtio, sizeof(&l1->newtio)); 
	l1->newtio.c_cflag = B38400|CS8|CLOCAL|CREAD;
	l1->newtio.c_iflag = IGNPAR|ICRNL;
	l1->newtio.c_oflag = 0;
	l1->newtio.c_lflag = ICANON;
	l1->newtio.c_cc[VTIME] = 3; /* temporizador entre   
								caracteres*/
	l1->newtio.c_cc[VMIN] = 0;  /* bloqueia até ler 5 
								caracteres */
	tcflush(fd, TCIFLUSH);
	tcsetattr(fd,TCSANOW,&l1->newtio);

}

int openSerialPort(char * port){

	int fdport = open(port, O_RDWR | O_NOCTTY);

	return fdport; //returns, if successfully, the file descriptor of the port we're trying to open. 
}


int closeSerialPort() {

	if (tcsetattr(al->fd, TCSANOW, &l1->oldtio) == -1) { //sets the parameters (immediately) refered by the application layer fd 
														 // from the old termios 
		perror("tcsetattr");
		return 0;
	}

	close(al->fd); //closes the application layer

	return 1;
}

unsigned char* createCommand(ControlField C) {
	CMD_SIZE = 5*sizeof(char);
	unsigned char* command = malloc(CMD_SIZE);

	command[0] = FLAG;
	command[1] = A;
	command[2] = C;
	if (C == C_REJ || C == C_RR)
		command[2] |= (ll->sequenceNumber << 7);
	command[3] = command[1] ^ command[2];
	command[4] = FLAG;

	return command;
}



//openserialport
//closeSerialPort
//int llopen
//int llwrite
//int llread
//int llclose