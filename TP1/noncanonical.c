/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define FLAG 0x7E
#define A    0x03
#define UA   0x02
#define SET  0x07

#define SUPERVISION 0


volatile int STOP=FALSE;

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    unsigned char buf[255];

    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
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

    tcgetattr(fd,&oldtio); /* save current port settings */

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */

    tcflush(fd, TCIFLUSH);
    tcsetattr(fd,TCSANOW,&newtio);

    printf("New termios structure set\n");

    unsigned char read_byte;

    int i = 0;
    int end = 0;
    int reading=0;
    while (i < 5) {       /* loop for input */
        int read_num = read(fd, &read_byte, 1);
        printf("%02x lidos:%d ", read_byte, read_num);
        buf[i] = read_byte;
        i++;             
    }

    if (buf[3] != (buf[2]^buf[1])){
        puts("erro");
        printf("FLAG:%2x, A:%2x, C:%2x, BCC:%02x, FLAG:%02x, XOR:%02x",buf[0], buf[1], buf[2], buf[3], buf[4], buf[2]^buf[1]);
        exit(1);
    }
    
    if (buf[1] == A && buf[2] == SET){
        char res[5];
        res[0] = FLAG;
        res[1] = A;
        res[2] = UA;
        res[3] = A^UA;
        res[4] = FLAG;
        write(fd, res, 5);
        printf("res: 0x%5x", res);
    }
        

	sleep(2);
    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
