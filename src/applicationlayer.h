/*Application Layer contains the communication protocols and interface methods used by hosts. */
#pragma once

#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>

#define PACKET_SIZE 100
#define TYPE_FILE_SIZE 0
#define TYPE_FILE_NAME 1

#define CONTROL_PACKET_BEGIN 1
#define CONTROL_PACKET_END 2
#define DATA_PACKET 0

#define E_NOTCONTROL -400
#define E_NOTDATA    -429

typedef struct  {
	char* filename;
	int size;
	int end;	
} ControlPacket;

typedef struct {
	unsigned char sequenceNumber;
	int size;
	char* data;	
} DataPacket;


//function declarations
int sendFile(int,int, char*);
int readFile(int);
int getSize(int);
unsigned char* makeControlPacket(unsigned int size, char* name, int end, int* length);
int getControlPacket(int port, ControlPacket* packet);
unsigned char *makeDataPacket(int packetSize, unsigned char *buffer, int *length);
int getDataPacket(int port, DataPacket* packet);
char *updateProgressBar(int completion, int totalSize, float *percentage);
int printProgressBar(char *progressBar, float);



