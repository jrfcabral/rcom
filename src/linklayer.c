#include "alarm.h"
#include "linklayer.h"

static unsigned char command_possible[] = {SET, UA, RR_1, RR_0, REJ_1, REJ_0, DISC, I_1, I_0 };


int main(int argc, char **argv){

	strcpy(ll.port, argv[1]);
	int mode = atoi(argv[2]);
	if(argc != 3 ||( mode != SEND && mode != RECEIVE)) {
		printf("Usage: %s /dev/ttySx\n x = port num\n", argv[0]);
	}

	ll.timeOut = 10;
	ll.sequenceNumber = 0;
	ll.numTransmissions  = 3;

	int fd = llopen(0, mode);
	if(mode == SEND){

		char message[] = "mens agem";
		llwrite(fd, message, strlen(message));
		//llclose(fd);
	}

	else if (mode == RECEIVE){
	char *bufferino;
		llread(fd, bufferino);
		free(bufferino);
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
	puts("reallocing\n");
	*buf = (unsigned char*) realloc(*buf, length);
	puts("reallocced\n");
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
	char header[] = { FLAG, 0x03, I(ll.sequenceNumber), header[1]^header[2] };
	int n= byteStuffing(buffer,  length, &bufferStuffed	);
	char* message = (char*)  malloc(n+6);
	memcpy(message, header, 4);
	memcpy(message+4, bufferStuffed, n);
	message[n+4] = dataBCC;
	message[n+5] = FLAG;
	int wrote = 0;
	while(!wrote)
		wrote = write(fd, message, n+6);


	puts("message sent\n");

	free(message);
	free(bufferStuffed);

	Command command = receiveCommand(fd);
	//was asked for new frame
	if (command.command == RR(!ll.sequenceNumber)){
		ll.sequenceNumber = (!ll.sequenceNumber);
		return length;
	}
	//frame was rejected, resend
	if (command.command == REJ(ll.sequenceNumber)){
		puts("byte was rejected,resending\n");
		return llwrite(fd, buffer, length);
	}
	else
		puts("llwrite: received unexpected response\n");

	return length;
}

int llread(int fd, char *buffer){
	printf("preparing to read frame\n");
	Command command = receiveCommand(fd);
	puts("llread:received command");
	int repeated;
	if (command.command == I(ll.sequenceNumber))
		repeated = 0;
	else if (command.command == I(!ll.sequenceNumber))
		repeated = 1;
	else if (command.command == DISC){
			printf("llread: disconnecting\n");
			while(!sendByte(fd, DISC, 0x01)){}
			puts("llread: disc confirmation sent\n");
			command = receiveCommand(fd);
			if (command.command != UA){
							puts("llread: didnt receive UA after disc confirmation\n");
							return E_GENERIC;
			}
			else{
				puts("llread: connection successfully closed\n");
				return E_CLOSED;
			}
	}

	if (command.command == I(0) || command.command == I(1)){
		puts("llread: received a data frame\n");
		//if we never saw this frame before, consider it
		if(!repeated){
			puts("llread: new data frame\n");
			int length = byteDeStuffing(&command.data, command.size);
			puts("llread: destuffing succeeded\n");
			int bccOK = verifyBCC(command.data, length, command.data[length-1]);
			//Reject frames with wrong BCC
			if(!bccOK){
				puts("llread: frame was damaged, rejecting and rereading\n");
				while(!sendByte(fd, 0x03, REJ(ll.sequenceNumber) )){}
					return llread(fd, buffer);
			}

			//accept the frame and confirm it
			puts("llread: frame bcc ok, accepting\n");
			buffer = (char*) malloc(length);
			memcpy(buffer, command.data, length);
			while(!sendByte(fd,0x03, RR(!ll.sequenceNumber))){}
			puts("llread: receiver ready sent, message confirmed\n");
			ll.sequenceNumber = !ll.sequenceNumber;
			free(command.data);
			return length;

		}
		//this frame is repeated. Confirm it and ask for the new frame
		else{
				puts("llread: message repeated");
				while(!sendByte(fd, 0x03, RR(ll.sequenceNumber))){}
				return E_GENERIC;
		}

	}

	else{
		printf("received unexpected command 0x%02x\n",command.command);
	}


}

int verifyBCC(unsigned char* data, int datalength, char correctBCC){
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


int sendByte(int fd, char address, char command){
	unsigned char FRAME[5] = {FLAG, address, command, FRAME[1]^FRAME[2], FLAG};
	int wrote = write(fd, FRAME, 5);
	if(!wrote)
		return -1;
	puts("FRAME with command %02x sent\n");
	return 1;

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
		puts("going to sleep\n");
		while(!read(fd, &byte, 1))
			continue;

		printf("receiveCommand: received byte 0x%02x\n", byte);
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

					break;
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

	Command command = receiveCommand(fd);
	if(command.command != DISC){
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
		if(command.command == SET){
			puts("llopen_receive: received SET message\n");
			while(!sendByte(fd, UA, 0x03)){}
		}

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

	}
	return fd;
}
