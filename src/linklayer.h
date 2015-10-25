/*In computer networking, the link layer is the lowest layer in the Internet Protocol Suite.
The link layer is the group of methods and communications protocols that only operate on the link that a host is physically connected to.
*/
#pragma once
#include <termios.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>


#define FALSE 0
#define TRUE 1

#define MAX_SIZE 256
#define BIT(n) (0x01 << n)


#define BAUDRATE B38400
#define MAX_RESEND 3
#define TIMEOUT 3
#define FLAG 0x7E
#define A_SEND 0x03
#define A_RECEIVE 0x01
#define C_UA 0x03
#define C_SET 0x07
#define C_DISC 0x0B
#define SEND 0
#define RECEIVE 1
#define ESCAPE 0x10
#define I(n) ( n << 5 )
#define RR(n)  ( RR_0 | n )
#define REJ(n) ( REJ_0 | n )

//Error codes
#define E_TIMEOUT -20
#define E_ABORT -50
#define E_CLOSED -100
#define E_GENERIC -200
#define E_AGAIN -300

unsigned int info;
/*FRAME FORMAT AND TYPES*/

typedef enum {
	WAIT_FLAG,
	WAIT_A,
	WAIT_C,
	WAIT_BCC,
	BCC_OK,
	EXIT
} state;

typedef enum {
	I_START, I_FLAG, I_A, I_C, I_BCC, I_STOP
} State; //tramas de informacao

typedef enum {
	SET = 0x07, //set up
	UA = 0x03,  //unnumbered acknowledgement
	RR_1 = 0x21,  //receiver ready
	RR_0 = 0x20,
	REJ_1 = 0x25,
	REJ_0 = 0x05, //reject / negative ACK
	DISC = 0x0B,
	I_1 = 0x20,
	I_0 = 0x00 //disconnect
} Control;

#define NUM_COMMANDS 9


typedef struct{
	char port[50]; //port : format /dev/ttySx
	int baudRate;
	int sequenceNumber; //frame sequece number
	int timeOut; //timeout value
	int numTransmissions; //number of retries if it fails

	char frame[MAX_SIZE];

	//termios

	struct termios oldtio, newtio;



}LinkLayer;

typedef struct {
	unsigned char command;
	unsigned char* data;
	unsigned char address;
	unsigned int size;
} Command;


LinkLayer ll;

//declarations
int byteStuffing(const char* buffer, const int length, char** stuffedBuffer);
int byteDeStuffing(unsigned char** buf, int length);
int llopen(int port, int mode);
int llwrite(int fd, char* buffer, int length);
int llclose(int fd);
int llread(int fd, char *buffer);
int verifyBCC(unsigned char* data, int datalength, char correctBCC);
int sendByte(int fd, char, char );
Command receiveCommand(int fd);
