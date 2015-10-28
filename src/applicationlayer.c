#include "linklayer.h"
#include "applicationlayer.h"

//1- porta
//2- modo (SEND | RECEIVE)
//3- filepath
int main(int argc, char **argv){

	ll.timeOut = 10;
	ll.sequenceNumber = 0; 
	ll.numTransmissions  = 3;
	int length;
	unsigned char* packet = makeControlPacket(15, "umficheirodebelonome",0, &length);
	int i;
	for(i = 0; i < length; i++){
		printf("%d, 0x%02x\n", i, packet[i]);
	}
	exit(0);
	
	int mode = atoi(argv[2]);
	if(argc != 4 ||( mode != SEND && mode != RECEIVE) || strncmp(argv[1], "/dev/ttyS", strlen("dev/ttyS"))) {
		printf("Usage: %s /dev/ttySx\n x = port num\n", argv[0]);
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
	
	int serialPort = llopen(argv[1], mode);
	if (serialPort < 0){
		perror("");
		exit(-1);
	}
	
	int result;	
	
	if (mode == SEND){
		result = sendFile(serialPort, fd);
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

int sendFile(int port, int fd)
{
	int size = getSize(fd);
	unsigned char* buffer = (unsigned char*) mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
	
	int i = 0;
	for(i = 0; i < (size/PACKET_SIZE); i++){
		
	}
	
	
	
	if(munmap(buffer, size))
		perror("failed to unmap the file");
}

int readFile(int port, int fd)
{
	ControlPacket packet;
	while(!getControlPacket(port, &packet)){}

	if(packet.end != CONTROL_PACKET_BEGIN){
		printf("readFile error: didn't receive expected start control package\n");
		return -1;
	}
	
	int file = open(packet.filename, O_CREAT|O_TRUNC|O_WRONLY);
	free(packet.filename);
	DataPacket dataPacket;
	int i;
	int expectedSequenceNumber = 0;
	for(i=0; i < packet.size;i++){
		getDataPacket(port, &dataPacket);
		if (expectedSequenceNumber != dataPacket.sequenceNumber){
			printf("ReadFile: received packet with wrong sequence number. aborting.\n");
			exit(-1);
		}
		expectedSequenceNumber++;
		expectedSequenceNumber %= 255;
		write(fd, dataPacket.data, dataPacket.size);
	}
		
		while(!getControlPacket(port, &packet)){}
	if(packet.end != CONTROL_PACKET_END){
		printf("readFile error: didn't receive expected start control package\n");
		return -1;
	}
		
}

int getDataPacket(int port, DataPacket* packet){
	char* buffer = NULL;
	int length = llread(port, buffer);
	if (length < 1)
		return E_GENERIC;

	if(buffer[0] != DATA_PACKET)
		return E_NOTDATA;
	packet->sequenceNumber = buffer[1];
	packet->size = (buffer[2] << 8 || buffer[3]);
	packet->data = (char*) malloc(packet->size);
	memcpy(packet->data, buffer+4, packet->size);
	free(buffer);
	return length;
}

int getControlPacket(int port, ControlPacket* packet){
	char* buffer = NULL;
	int length = llread(port, buffer);
	if(length < 0)
		return E_GENERIC;
	if (buffer[0] != 1 && buffer[0] != 2)
		return E_NOTCONTROL;
	
	packet-> end = buffer[0];
	
	int i = 3	;
	int argSize = buffer[i];
	memcpy(&packet->size, &buffer[i], argSize);
	i+=argSize+1;
	
	argSize = buffer[i-1];
	packet->filename = (char*) malloc(argSize);
	memcpy(packet->filename, &buffer[i], argSize);
	free(buffer);
	return length;
}




unsigned char* makeControlPacket(unsigned int size, char* name, int end, int* length){
	unsigned char* packet = malloc(1+2+sizeof(unsigned int)+2+strlen(name));
	packet[0] = end;
	packet[1] = TYPE_FILE_SIZE;
	packet[2] = sizeof(unsigned int);
	memcpy(&packet[3], &size, sizeof(unsigned int));
	packet[3+sizeof(unsigned int)] = TYPE_FILE_NAME;
	packet[4+sizeof(unsigned int)] = strlen(name);
	memcpy(&packet[5+sizeof(unsigned int)], name, strlen(name));
	*length = 1+2+sizeof(unsigned int)+2+strlen(name);
	return packet;		
}

