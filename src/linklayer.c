#include "alarm.h"
#include "linklayer.h"


state verifyByte(unsigned char expected, unsigned char read, state ifSucc, state ifFail){
	state toGo;
	if(expected == read){
		toGo = (state) ifSucc;
		printf("Sucesso, vou pro estado %d\n", toGo);
	}
	else if(read == FLAG){
		toGo = WAIT_A;
		printf("Flag, vou pro estado %d\n", toGo);
	}
	else{
		toGo = ifFail;
		printf("Falhanço, vou pro estado %d\n", toGo);
	}
	return toGo;
}

int main(int argc, char **argv){

	strcpy(ll.port, argv[1]);
	int mode = atoi(argv[2]);
	if(argc != 3 ||( mode != SEND && mode != RECEIVE)) {
		printf("Usage: %s /dev/ttySx\n x = port num\n", argv[0]);
	}

	ll.timeOut = 10;
	ll.sequenceNumber = 0;
	ll.numTransmissions  = 3;
//	char buffer[] = {FLAG, FLAG, ESCAPE, ESCAPE, 0x6e};
//	char buffer[] = {0, 0, 1, 1, 1,FLAG,2,3};

	int fd = llopen(0, mode);
	if(mode == SEND){

		char message[] = "Uma bela~ mensag~em cheia de ~til~es";
		llwrite(fd, message, strlen(message));
		//llclose(fd);
	}

	else if (mode == RECEIVE){
	char *bufferino = (char *) malloc(1);

	llread(fd, bufferino);
	}
	return 0;


}

int byteStuffing(const char* buffer, const int length, char** stuffedBuffer){
	int n;
	*stuffedBuffer = (char*) malloc(1);
	int newLength = 0;
	for(n = 0; n < length; n++){
		if (buffer[n] == FLAG || buffer[n] == ESCAPE){
			newLength+=2;
			*stuffedBuffer = realloc(*stuffedBuffer, newLength);
			stuffedBuffer[0][newLength-2]=ESCAPE;
			stuffedBuffer[0][newLength-1]= buffer[n]^0x20;
		}
		else{
			*stuffedBuffer = realloc(*stuffedBuffer, ++newLength);
			stuffedBuffer[0][newLength-1] = buffer[n];
		}
	}
	write(STDOUT_FILENO, *stuffedBuffer, newLength);

	return newLength;

}


int byteDeStuffing(unsigned char** buf, int length) {
	int i;
	for (i = 0; i < length; ++i) {
		if ((*buf)[i] == ESCAPE) {
			memmove(*buf + i, *buf + i + 1, length - i - 1);
			length--;
			(*buf)[i] ^= 0x20;
		}
	}
	*buf = (unsigned char*) realloc(*buf, length);
	return length;
}

char generateBCC(const char* buffer, const int length){
	int i;
	char bcc = 0;
	for(i = 0; i < length; i++)
		bcc ^=buffer[i];
	return bcc;
}


int llwrite(int fd, char* buffer, int length){
	char dataBCC = generateBCC(buffer, length);
	char *bufferStuffed;
	char header[] = { FLAG, 0x03, (ll.sequenceNumber << 5), header[1]^header[2] };
	int n= byteStuffing(buffer,  length, &bufferStuffed	);
	char* message = (char*)  malloc(n+6);
	memcpy(message, header, 4);
	memcpy(message+4, bufferStuffed, n);
	message[n+4] = dataBCC;
	message[n+5] = FLAG;
	int wrote = write(fd, message, n+6);
	return wrote;
	//todo ARQ
}

int llread(int fd, char *buffer){



}

int verifyBCC(char* data, int datalength, char correctBCC){
	char actualBCC = 0;
	int i;
	for(i=0;i<datalength-1;i++){
		actualBCC ^= data[i];
		printf("%d, 0x%02x\n", i, actualBCC);
	}

	if (actualBCC != correctBCC){
		printf("calculated BCC to be 0x%02x, expected it to be 0x%02x\n", actualBCC, correctBCC);
	}

	return actualBCC == correctBCC;
}

int readData(int fd, char** buffer){
	*buffer = malloc(1);
	int length = 0;
	int escaped = 0;
	int done = 0;

	while(!done){
		char in;
		int n = 0;
		while(!n)
			n = read(fd, &in, 1);

		if (!escaped && in == FLAG)
			done = 1;

		*buffer = realloc(*buffer, ++length);
		(*buffer)[length-1] = in;
		printf("ReadData read 0x%02x, ASCII: %c\n", in, in);

		if (!escaped && in == ESCAPE){
			escaped = 1;

		}

		else if (escaped){
			escaped = 0;
		}
	}
	return length;

}


int getHeader(int fd){
	unsigned char header[3], input;
	int r = 0;
	puts("getheader called");
	while(r == 0){
		r = read(fd, &input, 1);
		printf("getHeader read 0x%02x\n", input);
	}

	if(input != FLAG)
		return -1;

	int i = 0;
	for(i = 0; i < 2; i++){
		r = read(fd, (header+i), 1);
		if(!r){
			i--;
			continue;
		}
		printf("read: %x\n", header[i]);
		if(header[i] == FLAG){
			i = -1;
			continue;
		}
	}
	r = 0;
	while(!r)
		r = read(fd, (header+2), 1);

	if(header[2] == (header[1]^header[0])){
		return (int) header[1];
	}
	return -1;
}

int sendByte(int fd, char address, char command){
	unsigned char FRAME[5] = {FLAG, address, command, FRAME[1]^FRAME[2], FLAG};
	int wrote = write(fd, FRAME, 5);
	if(!wrote)
		return -1;
	puts("FRAME with command %02x sent\n");
	return 1;

}

int waitForByte(int fd, char expectedCommand){
	int command = -1;
	while(command != expectedCommand){
		command = getHeader(fd);
		if(command == -1)
			continue;
		unsigned char in;
		int n = 0;
		while(!n)
			n = read(fd, &in, 1);

		if(in == FLAG){
			return 1;
		}
		else{
			command = -1;
			continue;
		}
	}
	return 0;
}

Command receiveCommand(int fd){
	state currentState = WAIT_FLAG;
	Command command;
	command.data = (unsigned char*) malloc(1);
	command.size = 0;
	int isCommand = 0;
	int j;

	while(currentState != EXIT){
		char byte;
		int escaped = 0;

		while(!read(fd, &byte, 1))
			continue;

		switch(currentState){

			case WAIT_FLAG:
					if (byte == FLAG)
						currentState = WAIT_A;
					continue;
				break;

			case WAIT_A:
				if (byte == A_SEND || byte == A_RECEIVE){
					currentState = WAIT_C;
					command.address = byte;
				}
				else if (byte == FLAG)
					currentState = WAIT_A;
				else
					currentState = WAIT_FLAG;
				continue;
				break;

			case WAIT_C:
					for(j= 0; j < NUM_COMMANDS && !isCommand; j++){
						isCommand = (command_possible[j] == byte);
					}
					if (isCommand){
						command.command = byte;
						currentState = WAIT_BCC;
					}
					else if (byte == FLAG)
						currentState = WAIT_A;
					else
						currentState = WAIT_FLAG;
					continue;
				break;

			case WAIT_BCC:
				if (byte != (command.command^command.address))
					currentState = WAIT_FLAG;
				else
					currentState = BCC_OK;
				continue;
				break;

			case BCC_OK:
					if(byte == FLAG && !escaped){
						currentState = EXIT;
						continue;
					}

					command.data = realloc(command.data, ++command.size);

					if (!escaped && byte == ESCAPE){
						escaped = 1;
						command.data[command.size-1] = byte;
					}


					if (escaped){
						command.data[command.size-1] = byte^0x20;
						escaped = 0;
					}

					else
					command.data[command.size-1] = byte;


			default:
				perror("Something very strange happened\n");
				exit(-3);
				break;
		}
	}
	return command;
}

int llclose(int fd){
	if(!sendByte(fd, A_SEND, C_DISC))
		return -1;
	if(!waitForByte(fd, C_DISC)){
		return -1;
	}
	if(!sendByte(fd, A_RECEIVE, C_UA))
		return -1;
	return 1;
}


int llopen(int port, int mode){


	installAlarm();
	int fd;

	char fileName[15];
	fd = open(ll.port, O_RDWR|O_NOCTTY);
	if(fd <0){
		perror(fileName);
		exit(-1);
	}

	ll.baudRate = BAUDRATE;
	ll.timeOut = 3;
	ll.numTransmissions = 3;

	struct termios oldTio, newTio;

	if(tcgetattr(fd, &oldTio) == -1){
		perror("tcgetattr error");
		exit(-1);
	}
	ll.oldtio = oldTio;

	memset(&newTio, 0, sizeof(newTio)); //possible error here. if error use bzero instead
	newTio.c_cflag = ll.baudRate | CS8 | CLOCAL | CREAD;
	newTio.c_iflag = IGNPAR;
	newTio.c_oflag = 0;

	/* set input mode (non-canonical, no echo,...) */


	if(mode == RECEIVE){
		newTio.c_lflag = 0;

		newTio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
		newTio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */

		tcflush(fd, TCIOFLUSH);

		if ( tcsetattr(fd,TCSANOW,&newTio) == -1) {
			perror("tcsetattr");
			exit(-1);
		}

		printf("Ready to read\n");

	  Command command = receiveCommand(fd);





	}
	else if(mode == SEND){
		newTio.c_lflag = 0;

		newTio.c_cc[VTIME]    = 10;   /* inter-character timer unused */
		newTio.c_cc[VMIN]     = 0;   /* blocking read until 5 chars received */

		tcflush(fd, TCIOFLUSH);

		if ( tcsetattr(fd,TCSANOW,&newTio) == -1) {
			perror("tcsetattr");
			exit(-1);
		}

send: ;
	resend = 0;
	unsigned char SET[5] = {FLAG, A_SEND, C_SET, SET[1]^SET[2], FLAG};
	if(write(fd, SET, 5) != 5)
	      return -1;
	printf("escrevi\n");
	alarm(ll.timeOut);
	state currentState = WAIT_FLAG;
 	Command command = receiveCommand(fd);

	return fd;
	}
}
