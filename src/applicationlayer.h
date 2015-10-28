/*Application Layer contains the communication protocols and interface methods used by hosts. */
#pragma once

#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>

#define PACKET_SIZE 50
#define TYPE_FILE_SIZE 0
#define TYPE_FILE_NAME 1
//function declarations
int sendFile(int,int);
int readFile(int,int);
int getSize(int);
unsigned char* makeControlPacket(unsigned int size, char* name, int end);


