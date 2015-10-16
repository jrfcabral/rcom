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
#define A_SEND 0x03
#define A_RECEIVE 0x01
#define C_UA 0x03
#define C_SET 0x07
#define SEND 0
#define RECEIVE 1

typedef enum {
	WAIT_FLAG,
	WAIT_A,
	WAIT_C,
	WAIT_BCC,
    BCC_OK,
    EXIT
} state;


int resend = 0;

void alarmHandler(){
	write(STDOUT_FILENO, "SIGALRM received\n", 17);
	resend = 1;
}

state verifyByte(unsigned char expected, unsigned char read, int ifSucc, int ifFail){
	state toGo;	
	if(expected == read){
		toGo = (state) ifSucc;
	}
	else if(read == FLAG){
		toGo = WAIT_A;
	}
	else{
		toGo = (state) ifFail;
	}
	return toGo;
}


int main(int argc, char **argv){
	/*char *arg = (char *) malloc(strlen(argv[1])*sizeof(char)+1);
	strcpy(arg, argv[1]);
	char *tok1, *tok2;
	tok1 = strtok(argv[1], "/");
	tok2 = strtok(NULL, "/");*/
	if(argc != 2 /*|| (!strcmp(tok1, "dev") && !strncmp(tok2, "ttyS", 5))*/){
		printf("Usage: %s /dev/ttySx\n x = port num\n", argv[0]);
	}
	
	
	llopen(4, RECEIVE);
}

int llopen(int port, int mode){
	
	
	signal(SIGALRM, alarmHandler); //DEPRE-Frickin-CATED	

	int fd;
	
	char fileName[15];
    sprintf(fileName, "/dev/ttyS%d", port);
	
	fd = open(fileName, O_RDWR|O_NOCTTY);
	if(fd <0){
		perror(fileName);
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
        state currentState = WAIT_FLAG;
		printf("%d\n", currentState);
        char buf[255];
        int n = 0;
        while(currentState != EXIT){
           unsigned char in;
            int l = read(fd, &in, 1);
            char correctBcc;
            if (!l)
                continue;
            if(n >= 2)
                 correctBcc = buf[1]^buf[2];

            printf("read: %x\n", in);			

            switch(currentState){
                case WAIT_FLAG:
					currentState = verifyByte(FLAG, in, 1, 0);                  
                break;
                case WAIT_A:
					currentState = verifyByte(A_SEND, in, 2, 0);                 
                break;
                case WAIT_C:
					currentState = verifyByte(C_SET, in, 3, 0);                   
					break;
                case WAIT_BCC: 
	               currentState = verifyByte(0x04, in, 5, 0);                 
	               break;
                   
                case BCC_OK:
					currentState = verifyByte(FLAG, in, 6, 0);                 
                    break;
                default:
                    perror("Something very strange happened\n");
                    exit(-3);
                    break;                        
                
            }
            
        }
		printf("Received SET frame\n");
        unsigned char UA[5] = {FLAG, A_RECEIVE, C_UA, UA[1]^UA[2], FLAG};
        write(fd, UA, 5);
       
		
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

			unsigned char SET[5] = {FLAG, A_SEND, C_SET, SET[1]^SET[2], FLAG};
	    	write(fd, SET, 5);
			state currentState = WAIT_FLAG;
			while(currentState != EXIT){

					unsigned char in;
					if(!read(fd, &in, 1))
						continue;
			
				  switch(currentState){
		            case WAIT_FLAG:
						currentState = verifyByte(FLAG, in, 1, 0);                  
		            break;
		            case WAIT_A:
						currentState = verifyByte(A_SEND, in, 2, 0);                 
		            break;
		            case WAIT_C:
						currentState = verifyByte(C_UA, in, 3, 0);                   
						break;
		            case WAIT_BCC: 
			           currentState = verifyByte(A_SEND^C_UA, in, 5, 0);                 
			           break;                   
		            case BCC_OK:
						currentState = verifyByte(FLAG, in, 6, 0);                 
		                break;
		            default:
		                perror("Something very strange happened\n");
		                exit(-3);
		                break;                        
		        }
			}
	}

	return fd;	
}

