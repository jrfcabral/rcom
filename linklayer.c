#include "linklayer.h"

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

LinkLayer* ll;

int linkLayerInit(char* port, ConnectionMode mode, int baudrate, int msgMaxSize, int numTries, int timeOut){

}


int CurrentSettingsNewTermios(){

}

int saveSettings(){

}

int setTermios(){

}

//openserialport
//closeSerialPort
//int llopen
//int llwrite
//int llread
//int llclose