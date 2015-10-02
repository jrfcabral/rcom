/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define FLAG 0x7E
#define A 0x03
#define UA 0x01
#define C_SET 0x07	

volatile int STOP=FALSE;

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[255];
    int i, sum = 0, speed = 0;
    
    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS4", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }


  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */


    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */



  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) próximo(s) caracter(es)
  */



    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");



    /*for (i = 0; i < 255; i++) {
      buf[i] = 'a';
    }
    
    /*testing
    buf[25] = '\n';*/


	

	int check = setupComms(fd);
	if(!check){
		printf("Handshake failed.\n");
		return -1;
	}
	

	printf("Type in message: ");
	gets(buf);
    res = write(fd,buf,strlen(buf)+1);   
    printf("%d bytes written\nWaiting for response...\n", res);
	char rdbuf[255], msg[255];
	int n = 0, cnt = 0;
	while(rdbuf[0] != '\0'){
		n = read(fd, rdbuf, 1);
		msg[cnt++] = rdbuf[0];
	}
	//n = read(fd, rdbuf, res);
	printf("got back: %s\n", msg);
 

  /* 
    O ciclo FOR e as instruções seguintes devem ser alterados de modo a respeitar 
    o indicado no guião 
  */


	sleep(2);
   
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }




    close(fd);
    return 0;
}

int setupComms(int fd){
	unsigned char SET[5];
	unsigned char RESP[5];
	SET[0] = FLAG;	
	SET[1] = A;
	SET[2] = C_SET;
	SET[3] = SET[1]^SET[2];
	SET[4] = FLAG;

	int ret = write(fd, SET, 5);
	printf("%d\n", ret);
	int i = 0;
	for(i = 0; i < 5; i++){
		int readRet = read(fd, &RESP[i], 1);	
		printf("read returned: %d\nRead: %x\n", readRet, RESP[i]);
	}
	//printf("%x\n", RESP);
	if(RESP[2] == UA && RESP[3] == RESP[2]^RESP[1]){
		return 1;
	}
	else{
		return -1;
	}		
}


