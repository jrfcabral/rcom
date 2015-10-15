/*In computer networking, the link layer is the lowest layer in the Internet Protocol Suite. 
The link layer is the group of methods and communications protocols that only operate on the link that a host is physically connected to. 
*/
#pragma once 
#include <termios.h>
#include <stdio.h>

unsigned int info;

#define FALSE 0
#define TRUE 1

#define MAX_SIZE 256
#define BIT(n) (0x01 << n)


/*FRAME FORMAT AND TYPES*/

typedef enum {
	I_START, I_FLAG, I_A, I_C, I_BCC, I_STOP
} State; //tramas de informacao

typedef enum {
	SET, //set up
	UA,  //unnumbered acknowledgement
	RR,  //receiver ready 
	REJ, //reject / negative ACK
	DISC //disconnect
} Control;

typedef enum {
	C_SET = 0x03, C_UA = 0x07, C_RR = 0x05, C_REJ = 0x01, C_DISC = 0x0B
} ControlField; //campo de controlo das tramas de supervisão e nao numeradas 

typedef struct{
	char port[50] //port : format /dev/ttySx
	int baudRate;
	int msgMaxSize;
	info sequenceNumber; //frame sequece number
	info timeOut; //timeout value
	info numTransmissions //number of retries if it fails

	char frame[MAX_SIZE]

	//termios

	struct termios oldtio, newtio;
	


}LinkLayer;

extern LinkLayer* ll;

