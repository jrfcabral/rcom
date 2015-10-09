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

#define BAUDRATE B38400
#define MAX_RESEND 3
#define TIMEOUT 3
#define FLAG 0x7E
#define A 0x03
#define UA 0x03
#define C_SET 0x07

int resend = 0;

void alarmHandler(){
	write(STDOUT_FILENO, "SIGALRM received\n", 17);
	resend = 1;
}

int main(int argc, char **argv){
	char *arg = (char *) malloc(strlen(argv[1])*sizeof(char)+1);
	strcpy(arg, argv[1]);
	char *tok1, *tok2;
	tok1 = strtok(argv[1], "/");
	tok2 = strtok(NULL, "/");
	if(argc != 2 || (!strcmp(tok1, "dev") && !strncmp(tok2, "ttyS", 5))){
		printf("Usage: %s /dev/ttySx\n x = port num\n", argv[0]);
	}
	
	
	llopen(arg);
}

int llopen(int port, int flag){
	
	
	signal(SIGALRM, alarmHandler); //DEPRE-Frickin-CATED	

	int fd;
	
	fd = open(, O_RDWR|O_NOCTTY);
	if(fd <0){
		perror(port);
		exit(-1);	
	}
	
	struct termios oldTio, newTio;

	if(tcgetattr(fd, &oldTio) == -1){
		perror("tcgetattr error");
		exit(-1);	
	}
	
	memset(&newTio, 0, sizeof(newTio)); //possible error here. if error use bzero instead
	newTio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    	newTio.c_iflag = IGNPAR;
    	newTio.c_oflag = 0;

    	/* set input mode (non-canonical, no echo,...) */
    	newTio.c_lflag = 0;

    	newTio.c_cc[VTIME]    = 10;   /* inter-character timer unused */
    	newTio.c_cc[VMIN]     = 0;   /* blocking read until 5 chars received */
	
	tcflush(fd, TCIOFLUSH);

	if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      		perror("tcsetattr");
     		exit(-1);
    	}

	
}
