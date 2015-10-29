#include "linklayer.h"
#include "applicationlayer.h"

//1- porta
//2- modo (SEND | RECEIVE)
//3- filepath
int main(int argc, char **argv){

	//int fdesc = llopen(argv[1], SEND);
	//exit(-1);
	

	ll.timeOut = 10;
	ll.sequenceNumber = 0; 
	ll.numTransmissions  = 3;

	int mode = atoi(argv[2]);
	if(argc != 4 ||( mode != SEND && mode != RECEIVE) || strncmp(argv[1], "/dev/ttyS", strlen("dev/ttyS"))) {
		printf("Usage: %s /dev/ttySx\n x = port num\n", argv[0]);
		exit(-1);
	}
	int fd;
	if (mode == SEND && (fd = open(argv[3], O_RDONLY)) == ENOENT){
		perror("");
		exit(-1);
	}
	else if (mode == RECEIVE && (fd = open(argv[3], (O_RDWR | O_CREAT | O_TRUNC))) < 0){
		perror("");
		exit(-1);
	}	
	
	
	/*char test[] = "/bin/src/testerino.xd";
	char *coise = malloc(strlen(test));
	coise = basename(test);
	puts(coise);*/

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
		result = readFile(serialPort, fd);
	}

	if(result){
		perror("could not transmit file");
		return result;
	}
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
		printf("did not mmap\n");
		exit(-1);
	}
	int length;

	unsigned char* packet = makeControlPacket(size , filePath, 1, &length);
	printf("gonna send following packet of length %d: \n", length);
		/*int j;
		for(j = 0; j < length; j++){
			printf("%d: 0x%02x - %c\n", j, packet[j], packet[j]);
		}*/
	if(llwrite(port, packet, length) < 0 )
		return -1;
		
	int i = 0;
	for(i = 0; i < (size/PACKET_SIZE); i++){
		/*printf("gonna send following packet of length %d: \n", length);
		int j;
		for(j = 0; j < length; j++){
			printf("%d: 0x%02x - %c\n", j, packet[j], packet[j]);
		}*/
		packet = makeDataPacket(PACKET_SIZE, buffer, &length);
		
		if(llwrite(port, packet, length) < 0 )
			return -1;
		buffer += PACKET_SIZE;
	}
	
	if((size % PACKET_SIZE) != 0){
		/*packet = makeDataPacket((size % PACKET_SIZE), buffer, &length);
		printf("gonna send following packet of length %d: \n", length);
		int j;
		for(j = 0; j < length; j++){
			printf("%d: 0x%02x - %c\n", j, packet[j], packet[j]);
		}*/
		if(llwrite(port, packet, length) < 0 )
			return -1;
	}
	
	packet = makeControlPacket(size, filePath, 2, &length);
	llwrite(port, packet, length);	


	llclose(port);
	
	if(munmap(buffer, size) == -1)
		perror("failed to unmap the file");
}

int readFile(int port, int fd)
{
	ControlPacket packet;
	while(!getControlPacket(port, &packet)){}
	puts("recebi pacote de inicio");
	if(packet.end != CONTROL_PACKET_BEGIN){
		printf("readFile error: didn't receive expected start control package\n");
		return -1;
	}
	
	int file = open(packet.filename, O_CREAT|O_TRUNC|O_WRONLY);
	free(packet.filename);
	DataPacket dataPacket;
	int i;
	int expectedSequenceNumber = 0;
	while(getDataPacket(port, &dataPacket) != E_NOTDATA){
		
		expectedSequenceNumber++;
		expectedSequenceNumber %= 255;
		printf("data packet with size %d\n", dataPacket.size);
		write(file, dataPacket.data, dataPacket.size);
	}	
	puts("recebi pacote final");
	return 0;
}

int getDataPacket(int port, DataPacket* packet){
	char* buffer;
	int length = llread(port, &buffer);
	if (length < 1)
		return E_GENERIC;

	

	if(buffer[0] != DATA_PACKET && buffer[0] == CONTROL_PACKET_END){
		printf("lalalalala %02x lalala\n", buffer[0]);
		return E_NOTDATA;

	}
	packet->sequenceNumber = buffer[1];
	packet->size = 0x00;
	packet->size+= buffer[3];
	packet->size+= buffer[2]*16;
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
	for(	k=0;k<length;k++)
		printf("0x%02x, %x\n",k,buffer[k]);

	int size;
	memcpy(&size, buffer+3, 4);
	packet->size = size;
	int filenameLength = buffer[8];
	packet->filename = malloc(filenameLength+1);
	int i;
	for(i=0;i<filenameLength+1;i++){
		packet->filename[i] = buffer[8+i];
		printf("%c", buffer[8+i]);
	}
	puts("");
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
	printf("packetSize %x, packet[2] %x packet[3] %x\n", packetSize, packet[2], packet[3]);
	memcpy((packet+4), buffer, packetSize);
	
	*length = 4+packetSize;
	return packet;
}

