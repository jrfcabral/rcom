#include "linklayer.h"
#include "applicationlayer.h"

int visMode = 1;

//1- porta
//2- modo (SEND | RECEIVE)
//3- filepath
int main(int argc, char **argv){

	//int fdesc = llopen(argv[1], SEND);
	//exit(-1);
	setbuf(stdout, NULL);

	ll.timeOut = 3;
	ll.sequenceNumber = 0;
	ll.numTransmissions  = 3;
	int mode;

	if(argc == 2 && !strcmp("--help", argv[1])){
		printTutorial();
		exit(0);
	}

	if(argc >= 3){
		if(!strcmp(argv[2], "send"))
			mode = 0;
		else if(!strcmp(argv[2], "receive"))
			mode = 1;
		else{
			printUsage(argv[0]);
			exit(-1);
		}
	}
	else 
		exit(-1);
	if( (argc < 4 && mode == SEND) || (argc<3 && mode == RECEIVE) || strncmp(argv[1], "/dev/ttyS", strlen("dev/ttyS"))) {
		printUsage(argv[0]);
		exit(-1);
	}

	if((argc > 4 && mode == SEND) || (argc > 3 && mode == RECEIVE)){
		int i;
		for(i = 0; i < argc-(mode==SEND?4:3); i++){
			if(parseParams(argv[i+(mode==SEND?4:3)]) == -1)
				return -1;
		}
	}	
	

	int fd;
	if (mode == SEND && (fd = open(argv[3], O_RDONLY)) == ENOENT){
		perror("");
		exit(-1);
	}

	int serialPort = llopen(argv[1], mode);
	if (serialPort < 0){
		perror("");
		exit(-1);
	}

	int result;



	if (mode == SEND){

		result = sendFile(serialPort, fd, argv[3]);
	}
	else if (mode == RECEIVE){
		result = readFile(serialPort);
	}

	if(result){
		perror("could not transmit file");
		return result;
	}
	printStatistics(visMode);
	return 0;
}

int getSize(int fd){
	if (fd < 0)
		return -1;
	struct stat info;
	if (fstat(fd, &info))
		return -1;
	return info.st_size;

}

int sendFile(int port, int fd, char *filePath)
{
	int size = getSize(fd);
	unsigned char* buffer = (unsigned char*) mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
	if(buffer == MAP_FAILED){
		//printf("did not mmap\n");
		exit(-1);
	}
	unsigned char *bufferBckup = buffer;
	int length;

	unsigned char* packet = makeControlPacket(size , filePath, 1, &length);
	//printf("gonna send following packet of length %d: \n", length);
		/*int j;
		for(j = 0; j < length; j++){
			//printf("%d: 0x%02x - %c\n", j, packet[j], packet[j]);
		}*/
	if(llwrite(port, packet, length) < 0 )
		return -1;
	int i = 0;
	int acum = 0;
	float percentage;
	char *proBar = updateProgressBar(acum, size, &percentage);
	for(i = 0; i < (size/PACKET_SIZE); i++){
		/*//printf("gonna send following packet of length %d: \n", length);
		int j;
		for(j = 0; j < length; j++){
			//printf("%d: 0x%02x - %c\n", j, packet[j], packet[j]);
		}*/
		packet = makeDataPacket(PACKET_SIZE, buffer, &length);

		if(llwrite(port, packet, length) < 0 )
			return -1;
		buffer += PACKET_SIZE;

		acum += PACKET_SIZE;
		proBar = updateProgressBar(acum, size, &percentage);
		if(visMode != 0)
			printProgressBar(proBar, percentage);
		

	}

	if((size % PACKET_SIZE) != 0){
		packet = makeDataPacket((size % PACKET_SIZE), buffer, &length);
		/*//printf("gonna send following packet of length %d: \n", length);
		int j;
		for(j = 0; j < length; j++){
			//printf("%d: 0x%02x - %c\n", j, packet[j], packet[j]);
		}*/
		if(llwrite(port, packet, length) < 0 )
			return -1;
		acum += (size % PACKET_SIZE);
		
		proBar = updateProgressBar(acum, size, &percentage);
		if(visMode != 0)
			printProgressBar(proBar, percentage);
		
		
	}

	printf("\n");
	packet = makeControlPacket(size, filePath, 2, &length);
	llwrite(port, packet, length);


	llclose(port);

	if(munmap(bufferBckup, size) == -1)
		perror("failed to unmap the file");

		return 0;
}

int readFile(int port)
{
	ControlPacket packet;
	while(!getControlPacket(port, &packet)){}
	//puts("\nGot beginning packet");
	if(packet.end != CONTROL_PACKET_BEGIN){
		printf("Error: didn't receive expected start control package\n");
		return -1;
	}

	int file = open(packet.filename+1, O_CREAT|O_TRUNC|O_WRONLY, 0666);
	free(packet.filename);
	DataPacket dataPacket;
	unsigned char expectedSequenceNumber = 0;
	float percentage;
	int acum = 0;
	char * proBar = updateProgressBar(0, packet.size, &percentage);
	while(getDataPacket(port, &dataPacket) != E_NOTDATA){

		if (expectedSequenceNumber != dataPacket.sequenceNumber){
			printf("Error in packet sequence: expected packet no %u and got packet no %u\n", expectedSequenceNumber, dataPacket.sequenceNumber);
		exit(-1);
		}
		expectedSequenceNumber++;
		expectedSequenceNumber %= 255;
		
		//printf("Received data packet with size %d\n", dataPacket.size);
		write(file, dataPacket.data, dataPacket.size);
		acum += dataPacket.size;
		proBar = updateProgressBar(acum, packet.size, &percentage);
		if(visMode != 0)
			printProgressBar(proBar, percentage);

	}
	printf("\n");
	unsigned char* dump;
	while(llread(port, &dump) != E_CLOSED){}	
	//puts("discei");
	return 0;
}

int getDataPacket(int port, DataPacket* packet){
	unsigned char* buffer;
	int length = llread(port, &buffer);
	if (length < 1)
		return E_GENERIC;



	if(buffer[0] != DATA_PACKET && buffer[0] == CONTROL_PACKET_END){
		//printf("lalalalala %02x lalala\n", buffer[0]);
		return E_NOTDATA;

	}
	packet->sequenceNumber = buffer[1];
	packet->size = 0x00;
	packet->size+= buffer[3];
	packet->size+= buffer[2]*256;
	//printf("%d packet size\n", packet->size);	
	packet->data = (char*) malloc(packet->size);
	memcpy(packet->data, buffer+4, packet->size);
	free(buffer);
	return length;
}

int getControlPacket(int port, ControlPacket* packet){
	unsigned char* buffer;
	int length = llread(port, &buffer);



	if(length < 0)
		return E_GENERIC;
	if (buffer[0] != 1 && buffer[0] != 2)
		return E_NOTCONTROL;

	packet-> end = buffer[0];
	int k;
	for(	k=0;k<length;k++){
		//printf("0x%02x, %x\n",k,buffer[k]);
	}

	int size;
	memcpy(&size, buffer+3, 4);
	packet->size = size;
	int filenameLength = buffer[8];
	packet->filename = malloc(filenameLength+1);
	int i;
	for(i=0;i<filenameLength+1;i++){
		packet->filename[i] = buffer[8+i];
		//printf("%c", buffer[8+i]);
	}
	//puts("");
	return length;
}




unsigned char* makeControlPacket(unsigned int size, char* name, int end, int* length){
	int i, stopper;
	for(i = strlen(name); i > 0; i--){
		if(name[i] == '/'){
			stopper = i+1	;
		}
	}
	char *actualName = (char *)malloc(strlen(name) - stopper + 1);

	for(i = 0; i < (strlen(name) - stopper); i++){
		actualName[i] = name[stopper+i];
	}
	actualName[(strlen(name) - stopper)] = '\0';
	unsigned char* packet = malloc(1+2+sizeof(unsigned int)+2+strlen(name));
	packet[0] = end;
	packet[1] = TYPE_FILE_SIZE;
	packet[2] = sizeof(unsigned int);
	memcpy(&packet[3], &size, sizeof(unsigned int));
	packet[3+sizeof(unsigned int)] = TYPE_FILE_NAME;
	packet[4+sizeof(unsigned int)] = strlen(actualName)+1;
	memcpy(&packet[5+sizeof(unsigned int)], actualName, strlen(actualName)+1);
	*length = 1+2+sizeof(unsigned int)+2+strlen(actualName)+1;
	return packet;
}

unsigned char *makeDataPacket(int packetSize, unsigned char *buffer, int *length){
	static int seqNum = 0;

	unsigned char *packet = (unsigned char *) malloc(4+packetSize);
	packet[0] = 0;
	packet[1] = seqNum++;
	if(seqNum == 255)
		seqNum = 0;
	packet[2] = packetSize >> 8;
	packet[3] = packetSize;
	//printf("packetSize %x, packet[2] %x packet[3] %x\n", packetSize, packet[2], packet[3]);
	memcpy((packet+4), buffer, packetSize);

	*length = 4+packetSize;
	return packet;
}


char *updateProgressBar(int completion, int totalSize, float *percentage){
	float num = (((float)completion/(float)totalSize)*100)/2.0;
	*percentage = num*2.0;
	char *progressBar = (char *)malloc(53);
	progressBar[0] = '[';
	progressBar[51] = ']';
	int i;
	for(i = 1; i < 51; i++){
		if(num-- > 0){
			progressBar[i] = '#';
		}
		else
			progressBar[i] = '-';
	}
	progressBar[52] = 0;

	return progressBar;
}

int printProgressBar(char *progressBar, float perc){
	printf("%s%.2f%%", progressBar, perc);
	int i;

	for(i = 0; i <= 60; i++){
		printf("\b");
	}
	free(progressBar);
	return 0;
}

int printTutorial(){
	printf("Available options: \n");
	printf("-q:\tMakes the program run quietly (i.e no output is shown)\n");	
	printf("-b=[num]:\tChanges the baudrate to num. Default is %d\nWARNING: If the receiver's baudrate is significantly lower than the sender's, it might cause the program to fail.\n", BAUDRATE);
	printf("-m=[num]:\tSets the number of times the program will try to transmit the same frame before exiting. Default is 3\n");
	printf("-t=[num]:\tSets the time (in seconds) the sender will wait for a response before resending the current frame. Default is 3.\n");
	printf("-p=[num]:\tSets the number of data bytes sent per package. Default is 100.\n");
	return 0;	
}

int parseParams(char *param){
	
		if(!strncmp("-b", param, 2)){
			char *temp = malloc(3);
			memcpy(temp, (param+3), strlen(param)-3);
			ll.baudRate = atoi(temp);
			if(ll.baudRate < 1){
				printf("\nError: Baudrate cannot be negative or 0.\n");
				return -1;
			}
			//printf("\nBaudrate changed\n");
		}
		else if(!strncmp("-m", param, 2)){
			char *temp = malloc(3);
			memcpy(temp, (param+3), strlen(param)-3);
			ll.numTransmissions = atoi(temp);
			if(ll.numTransmissions < 1){
				printf("\nError: Number of retries cannot be negative or 0.\n");
				return -1;
			}
			//printf("\nnumTransmissions changed\n");
			
		}

		else if(!strncmp("-t", param, 2)){
			char *temp = malloc(3);
			memcpy(temp, (param+3), strlen(param)-3);
			ll.timeOut = atoi(temp);
			if(ll.timeOut < 1){
				printf("\nError: Timeout cannot be negative or 0.\n");
				return -1;
			}
			//printf("\ntimeOut changed to %d\n", ll.timeOut);
			
		}

		else if(!strncmp("-p", param, 2)){
			char *temp = malloc(3);
			memcpy(temp, (param+3), strlen(param)-3);
			PACKET_SIZE = atoi(temp);
			if(PACKET_SIZE < 1){
				printf("\nError: Packet size cannot be negative or 0.\n");
				return -1;
			}
			//printf("\nPACKET_SIZE changed to %d\n", PACKET_SIZE);
			
		}

		else if(!strncmp("-q", param, 2)){
			visMode = 0;
		}

		else{
			printf("Error: Unknown option %s\n", param);
			return -1;
		}
	
	return 0;

}

int printUsage(char *arg){
	printf("Usage: %s /dev/ttySx or /dev/pts/x,  send or receive, path(if executed as sender)\n x = port num\nFor more info use %s --help\n", arg, arg);
	return 0;
}

