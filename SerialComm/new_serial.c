/* ---- Include Files ---------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <getopt.h>
#include <termios.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <strings.h>
#include <signal.h>		/* for signal */

/* ---- Public Variables ------------------------------------------------- */

FILE *fp,*fp1;	//File pointers for the 2 files
FILE *debug;
int         opt;
char        devName[ 40 ];	//port name is stored here
const char *baudStr = NULL;	//Baud rate is stored here
const char *portStr;		//Read port from command line
speed_t     baudRate;		//Set baudrate
struct termios stdin_tio;
struct termios stdin_tio_org;
struct termios attr;

int    alreadyUploading = 0;   // flag set to 1 while uploading
                               // prevents reentry in the onTimer handler 
                               // if current upload is still ongoing

#define INTERVAL 10000		/* number of milliseconds to go off */
#define CLOCK_INTERVAL 50000		/* number of milliseconds interval to set the ABLE clocK*/

int gVerbose = 0;
int gDebug = 0;

int gPortFd = -1;		//File descriptor for the porta
int any_one_here = 0;
int clock_interval = 0; /* Variable to calculate the elapsed time for the clock set routine */

struct option gLongOption[] =
{
    // option       A  Flag   V  (has_arg, flag, val)
    // -----------  -  ----  ---
    { "baud",       1, NULL, 'b' },
    { "debug",      0, NULL, 'd' },
    { "help",       0, NULL, 'h' },
    { "port",       1, NULL, 'p' },
    { "verbose",    0, NULL, 'v' },
    { 0 },

};

struct
{
    speed_t     speed;
    unsigned    baudRate;
} gBaudTable[] =
{
    { B50,          50 },
    { B75,          75 },
    { B110,        110 },
    { B134,        134 },
    { B150,        150 },
    { B200,        200 },
    { B300,        300 },
    { B600,        600 },
    { B1200,      1200 },
    { B1800,      1800 },
    { B2400,      2400 },
    { B4800,      4800 },
    { B9600,      9600 },
    { B19200,    19200 },
    { B38400,    38400 },
    { B57600,    57600 },
    { B115200,  115200 },
    { B230400,  230400 }
};

#define ARRAY_LEN(x)    ( sizeof( x ) / sizeof( x[ 0 ]))



/* ---- Private Function Prototypes -------------------------------------- */
void onTimer();	//Function that is triggered every INTERVAL seconds
void Init_Serial();//Initialize the serial port
int upload_data();//Get the data from the ABLE board

char *StrMaxCpy( char *dst, const char *src, size_t maxLen );
char *StrMaxCat( char *dst, const char *src, size_t maxLen );
void  Usage( void );

/* ---- Functions -------------------------------------------------------- */

#if defined(__CYGWIN__)
// cfmakeraw defines the serial port parameters. The structure of termios is as shown
//	struct termios {
//  tcflag_t c_iflag;    /* input specific flags (bitmask) */
//  tcflag_t c_oflag;    /* output specific flags (bitmask) */
//  tcflag_t c_cflag;    /* control flags (bitmask) */
//  tcflag_t c_lflag;    /* local flags (bitmask) */
//  cc_t     c_cc[NCCS]; /* special characters */
//};
/*Input flags - Turn off input processing
// convert break to null byte, no CR to NL translation,
// no NL to CR translation, don't mark parity errors or breaks
// no input parity check, don't strip high bit off,
// no XON/XOFF software flow control
//
config.c_iflag &= ~(IGNBRK | BRKINT | ICRNL |
                    INLCR | PARMRK | INPCK | ISTRIP | IXON);
//
// Output flags - Turn off output processing
// no CR to NL translation, no NL to CR-NL translation,
// no NL to CR translation, no column 0 CR suppression,
// no Ctrl-D suppression, no fill characters, no case mapping,
// no local output processing
//
// config.c_oflag &= ~(OCRNL | ONLCR | ONLRET |
//                     ONOCR | ONOEOT| OFILL | OLCUC | OPOST);
config.c_oflag = ~OPOST;
//
// No line processing:
// echo off, echo newline off, canonical mode off, 
// extended input processing off, signal chars off
//
config.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);
//
// Turn off character processing
// clear current char size mask, no parity checking,
// no output processing, force 8 bit input
//
config.c_cflag &= ~(CSIZE | PARENB);
config.c_cflag |= CS8;
*/

static void cfmakeraw(struct termios *termios_p)
{
    termios_p->c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON);
    termios_p->c_oflag &= ~OPOST;
    termios_p->c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN); //selecting raw mode for input.
    termios_p->c_cflag &= ~(CSIZE|PARENB);
    termios_p->c_cflag |= CS8;
}
#endif /* defined(__CYGWIN__) */

unsigned int calc_crc(unsigned int length, char UPLOAD_PACKET[])

/* 
 * TGW 01 Feb 10  Routine to calculate 2-byte crc and append it to UPLOAD_PACKET string.
 *    Passed "length" is final length of packet including two crc bytes.
 *    TGW 25 Feb 10  Added inversion of final crc value by XOR with 0xFFFF, which yields 
 *       values called for by STX2 datasheet.
 *       TGW 19 Mar 10  Routine calculates correct CRC, at least for sample string.  Cleaned up reporting printout.   
 *
 *       */

{
  unsigned int crc = 0xFFFF; // initialize crc to FFFF
  unsigned int i, j; // loop counters

  crc = 0xFFFF; // initialize cyclic redundancy check for modem packet
  for(i=0; i<(length); i++)
    {
      crc ^= (unsigned int)(UPLOAD_PACKET[i]);
      for(j=0; j<8; j++)
	{
	  if(crc & 0x0001) crc = (crc>>1) ^ 0x8408; // if crc is odd, shift R 1 bit and EOR with 0x8408
	  else crc >>= 1; // if even, just shift right 1 bit
	}
    }
  crc ^= 0xFFFF; // final value must be bit-inverted (unclear in STX2 datasheet)
  return crc;
}

void Init_Serial()
{
	if (( gPortFd = open( devName, O_RDWR)) < 0 ) {
//	if (( gPortFd = open( devName, O_RDWR | O_NOCTTY | O_NDELAY)) < 0 ) {
        fprintf( stderr, "Unable to open serial port '%s': %s\n", devName, strerror( errno ));
        exit( 2 );
    }
    fprintf(debug,"Serial Opened\n");
    printf("Serial Opened\n");

    if ( tcgetattr( gPortFd, &attr ) < 0 ) {
        fprintf( stderr, "Call to tcgetattr failed: %s\n", strerror( errno ));
        exit( 3 );
    }

    cfmakeraw( &attr );

    // CLOCAL - Disable modem control lines
    // CREAD  - Enable Receiver

    attr.c_cflag |= ( CLOCAL | CREAD );
    attr.c_cc[VMIN] = 0;
    attr.c_cc[VTIME] = 200;
    cfsetispeed( &attr, baudRate );
    cfsetospeed( &attr, baudRate );

    if (tcsetattr(gPortFd, TCSAFLUSH, &attr) < 0 ) {
        fprintf( stderr, "Call to tcsetattr failed: %s\n", strerror( errno ));
        exit(4);
    }

    // Put stdin & stdout in unbuffered mode.

    // setbuf( stdin, NULL );
    setbuf(stdout, NULL);

    // Put stdin in raw mode (i.e. turn off canonical mode). This is to read all the characters in the buffer via read.

    if (tcgetattr(fileno(stdin), &stdin_tio_org) < 0 ) {
        fprintf(stderr, "Unable to retrieve terminal settings: %s\n", strerror( errno ));
        exit(5);
    }

    stdin_tio = stdin_tio_org;
    stdin_tio.c_lflag &= ~ICANON ;

    stdin_tio.c_cc[VTIME] = 200;
    stdin_tio.c_cc[VMIN] = 0;

    if (tcsetattr( fileno( stdin ), TCSANOW, &stdin_tio ) < 0 ) {
        fprintf( stderr, "Unable to update terminal settings: %s\n", strerror( errno ));
        exit( 6 );
    }
}


int main(int argc, char **argv ) {

    struct itimerval it_val;	/* for setting itimer */
    debug = fopen("Serial_Logs.txt","a+");
   // system("echo powersave > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
    fprintf(debug,"Put BB to powersave\n");
    while (( opt = getopt_long( argc, argv, "b:dhp:v", gLongOption, NULL )) > 0 ) {
        switch ( opt ) {
            case 'b': 
                baudStr = optarg;
                break;
            case 'd':
                gDebug = 1;
                break;
            case 'p':
                portStr = optarg;
                break;
            case 'v':
                gVerbose = 1;
                break;
            case '?':
            case 'h':
                Usage();
                return 1;
        }
    }

    devName[0 ] = '\0';
    if ( portStr[ 0 ] != '/' ) {
        StrMaxCpy( devName, "/dev/", sizeof( devName ));
    }

    StrMaxCat( devName, portStr, sizeof( devName ));

    if (portStr == NULL) {
    	strcpy(devName, "/dev/ttyUSB0");
    }

    baudRate = B115200;
    fprintf(debug,"Set port and Baud rate\n");
   
    Init_Serial();
    fprintf(debug,"Initialized serial port\n");

    /* Upon SIGALRM, call onTimer().*/
    if (signal(SIGALRM, (void (*)(int))onTimer ) == SIG_ERR) {
        perror("Unable to catch SIGALRM");
        exit(1);
    }
    it_val.it_value.tv_sec =     INTERVAL/1000;
    it_val.it_value.tv_usec =    (INTERVAL*1000) % 1000000;	
    it_val.it_interval = it_val.it_value;
    if (setitimer(ITIMER_REAL, &it_val, NULL) == -1) {
        perror("error calling setitimer()");
        exit(1);
    }

    while(1) {
//        printf("Calling Pause\n");
        pause();	//Pause puts CPU in IDLE.
    }
}

/*Function to print the system time upto microseconds resolution*/
void timerr(){
	 struct timeval tim;
         gettimeofday(&tim, NULL);
         double t1=tim.tv_sec;
         //printf("%.6lf seconds elapsed\n",t1);
}

/*Function to sleep for nanoseconds*/
int __nsleep(const struct timespec *req, struct timespec *rem)
{
    struct timespec temp_rem;
    if(nanosleep(req,rem)==-1)
        __nsleep(rem,&temp_rem);
    else
        return 1;
}
 /*Function to sleep for milliseconds*/
int msleep(unsigned long milisec)
{
    struct timespec req={0},rem={0};
    time_t sec=(int)(milisec/1000);
    milisec=milisec-(sec*1000);
    req.tv_sec=sec;
    req.tv_nsec=milisec*1000000L;
    __nsleep(&req,&rem);
    return 1;
}
 

void onTimer(){

  char file[100];	//Storage for file name
  time_t t = time(NULL);	//Get current system time
  struct tm tm = *localtime(&t);	//convert to struct

  if (alreadyUploading)
    return;

  alreadyUploading = 1;   // set it to 1 to prevent future re-entry if current upload is ongoing


    
  //printf("now: %d-%d-%d %d:%d:%d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  sprintf(file, "host01/%d%d%d.txt",tm.tm_year+1900,tm.tm_mon+1,tm.tm_mday);	//Store the current time as YYYYMMDD in file
    
  fp1 = fopen(file,"a+");	//Open file with name as YYYYMMDD
	
  fp = fopen("FILE.txt","a+");//Open a global file
	
  //	system("echo performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");

	
  if (upload_data()==0) {	//Get the data from ABLE
    printf("Upload failed \n");
  } else {
    //        printf("Upload Sucess \n");
  }

	
  fclose(fp);//Close the global file
    
  fclose(fp1);//Close the file with name as YYYYMMDD

  alreadyUploading = 0;   // done uploading, so we release it

  //	system("echo powersave > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");//Put CPU in powersave mode
}

set_ABLE_clock() {

  printf("Setting ABLE clock\n");
  char init = 'Q';
  char ch = NULL;
    
  if (write(gPortFd, &init,1) != 1) {
    fprintf(debug,"Write to serial port to get \"?\" failed\n");
    fprintf( stderr, "write to serial port failed: %s\n", strerror( errno ));
  }
	 
  int num = 0; /*Track the number of character we are receving */
  int done = 0; /*To check if the clock setting is done */

    
  while(!done) {

    ch = (char *)NULL;	//Reset the character

    if (read(gPortFd, &ch, 1) < 0) {
      fprintf(debug,"Read from serial port failed %s\n", strerror(errno));
      fprintf( stderr, "Serial port read failed: %s\n", strerror( errno ));
      return(0);
    }

    num++;
    if(ch == NULL) {
      fprintf(debug,"Received no character from ABLE\n");
      continue;
    }

    init = 'C';
    if (ch == '?') {
      if (write(gPortFd, &init,1) != 1) {
	fprintf(debug,"Write to serial port to get \"?\" failed\n");
	fprintf( stderr, "write to serial port failed: %s\n", strerror( errno ));
      }

      while(!done) {
	/*Read until we receive a '^' */
	if (read(gPortFd, &ch, 1) < 0) {
	  fprintf(debug,"Read from serial port failed %s\n", strerror(errno));
	  fprintf( stderr, "Serial port read failed: %s\n", strerror( errno ));
	  return(0);
	}

	if (ch == '^') {
	  /* We are ready to set the clock. Six variables. Y^M^D^H^M^S */
	  time_t t = time(NULL);  //Get current system time
	  struct tm tm = *localtime(&t);  //convert to struct

	  char year = (char)12;
	  char month = (char) tm.tm_mon + 1;
	  char day = (char) tm.tm_mday;
	  char hour = (char) tm.tm_hour;
	  char min = (char) tm.tm_min;
	  char sec = (char) tm.tm_sec;
	  int tracker = 1; /* to track the which data we are sending */

	  while(1) {
				
	    char data = NULL;
	    switch (tracker) {
	    case 1:
	      data = year;
	      break;
	    case 2:
	      data = month;
	      break;
	    case 3:
	      data = day;
	      break;
	    case 4:
	      data = hour;
	      break;
	    case 5:
	      data = min;
	      break;
	    case 6:
	      data = sec;
	      break;
	    }

	    tracker += 1;
	    if (write(gPortFd, &data, 1) != 1) {
	      fprintf(debug,"Write to serial port to get \"?\" failed\n");
	      fprintf( stderr, "write to serial port failed: %s\n", strerror( errno ));
	    }

	    char ack;
	    if (read(gPortFd, &ack, 1) < 0) {
	      fprintf(debug,"Read from serial port failed %s\n", strerror(errno));
	      fprintf( stderr, "Serial port read failed: %s\n", strerror( errno ));
	      return(0);
	    }

	    /* Send ACK */
						
	    ack = '*';
	    if (write(gPortFd, &ack, 1) != 1) {
	      fprintf(debug,"Write to serial port to get \"?\" failed\n");
	      fprintf( stderr, "write to serial port failed: %s\n", strerror( errno ));
	    }
				
	    char delimit = NULL;	
	    /* Read the next '^' or 'OK' */	
	    if (read(gPortFd, &delimit, 1) < 0) {
	      fprintf(debug,"Read from serial port failed %s\n", strerror(errno));
	      fprintf( stderr, "Serial port read failed: %s\n", strerror( errno ));
	      return(0);
	    }

	    if (delimit == '^') {
	      continue;
	    }

	    if (delimit == 'O') {
	      /* Read the 'K' and bail out */	
	      if (read(gPortFd, &delimit, 1) < 0) {
		fprintf(debug,"Read from serial port failed %s\n", strerror(errno));
		fprintf( stderr, "Serial port read failed: %s\n", strerror( errno ));
		return(0);
	      }
	      printf("Clock setting Done\n");
	      done = 1;
	      break;
	    }
	  }
	}
      }
    }
  }

  clock_interval = 0;
}

int upload_data() {// returns zero on failure, one otherwise 

  char chw = 'Q';		//Character to send to ABLE
  char ch = NULL;		//Store received character in this variable
  int x = 0;		//counter for a 'for' loop
  char temp_buffer[1000];	//Buffer to store temporary data as it is received for CRC computation
  char low,high;		//Store low and high bytes of CRC here
  int old_length = 0;	//Cumulative length of a record till 'END OF RECORD' is received.
  int palint = 0;		//Check for indication to throw all packets received till now for a record.
  int packet_received = 0;//Flag to denote all data in packet is received and any character coming now is not DATA.
  int chksum_flag = 0;	//Flag to denote next byte is CHECK SUM.
  int sof_data = 0;	//Check if SOF character is part of data or new packet.
  int a = 0;		//Counter to count the number of bytes of checksum received
  char chk_sum[6] = "\0";	//Array to store the checksum
  int check_chksum = 0;	//Indication to start computation of checksum as all data in packet is received
  int length_flag = 0;	//Indicates next byte is for length
  int start_receive = 0;	//Start receiving and storing data.
  int num = 0;		//Counter to for number of data bytes received
  int length = 0;		//Stores the length of data expected
  unsigned int comp_crc = 0;	//get the CRC result from the function
  char final_char[1000] = "\0";	//Array to store final data of the record and print it and store it to a file
  int print_char = 0; 	//Indication to print the data to the file.a
  int end_packet = 0;
  int question_received = 0; // vairable to track if "?" is received.
  int bail_out = 0;// IF we did not receive "?" after 25 characters, bail out.
    
  if (any_one_here == 0) {
    any_one_here = 1;
  } else {
    return 1;
  }

  clock_interval += INTERVAL;

  tcflush(gPortFd, TCIFLUSH);

  if (clock_interval > CLOCK_INTERVAL) {
    set_ABLE_clock();
  }
    
  // Send 'Q' Initially
  if (write(gPortFd, &chw,1) != 1) {
    printf("Sending Q failed\n");
    fprintf(debug,"Write to serial port to get \"?\" failed\n");
    fprintf( stderr, "write to serial port failed: %s\n", strerror( errno ));
  }
	 
  ch = NULL;
  num = 0;
    
  while(1) {

    ch = (char *)NULL;	//Reset the character

    if (read(gPortFd, &ch, 1) < 0) {
      fprintf(debug,"Read from serial port failed %s\n", strerror(errno));
      fprintf( stderr, "Serial port read failed: %s\n", strerror( errno ));
      any_one_here = 0;
      return(0);
    }

    if (!question_received) {
      bail_out++;

      if (bail_out > 25) {
	printf("No Question Mark, Quit :D");
	any_one_here = 0;
	return(0);
      }
    }
   
    if(ch == NULL && chksum_flag == 0) {
      fprintf(debug,"Received no character from ABLE\n");
      continue;
    }

    if (ch == '#') {
      palint ++;
      if(palint > 3) {
	//Indiaction to flush out the packets collected so far for the record .
	fprintf(debug,"Received 4 # ... flushing out data\n");
	old_length = 0;
	final_char[0] = NULL;
	printf("thrown");
	any_one_here = 0;
	return 1;
      }
    } else {
      palint = 0;
    }

    if((ch == '?' || ch == "!") && (chksum_flag == 0) && (length_flag==0)) {
      //    if((ch == '?' || ch == "!") && (chksum_flag == 0)) {
      chw = 'U';

      num = 0;
      fprintf(debug,"Received %c ... sending U\n",ch);
      if (write( gPortFd, &chw,1 ) != 1) {
	fprintf(debug,"Write \"U\"to serial port failed\n");
	fprintf( stderr, "write to serial port failed: %s\n", strerror( errno ));
	any_one_here = 0;
	return(0);
      }
      question_received = 1;
      continue;
    }

    /* SOF */
    if (ch == 0x24 && chksum_flag == 0 && length_flag == 0) {
      packet_received = 0;
      sof_data ++;	//Used to prevent confusion if AA is received as part of data or checksum.
      if (sof_data == 1) {
	length_flag = 1;
      }
      temp_buffer[0] = ch;
      continue;
    }
		
    if (length_flag == 1) {
      fprintf(debug,"Received length = %d\n",ch);
      length_flag = 0;
      length = ch;	//Store the length
      temp_buffer[1] = ch;
      start_receive = 1;//Indicate to start receiving the data.
      //printf("Received length %d",ch);
      num = 0;
      continue;
    }
        
    /* Check if this is an END packet */
    if (ch == 'E' && chksum_flag == 0) {
      fprintf(debug,"Received End Packet\n");
      //printf("<-- E");
      end_packet = 1;
    }
        
    if(ch == '\n' || ch == '\r') {
      //printf("Received newline\n");
      print_char = 1;
    }
    //Receive "length - 2" bytes as length includes the start-of-frame also and when done, set the flag to store the checksum.
    if (start_receive == 1) {
      temp_buffer[num+2]=ch;	//Store the data in temporary buffer to compute the checksum on.
      num ++;
      if(num == length - 2) {
	packet_received = 1;
	start_receive = 0;
	chksum_flag = 1;
	num = 0;
      }
      continue;
    }
        

    if(chksum_flag == 1) {
      fprintf(debug,"Received Checksum %d\n",ch);
      chk_sum[a]=ch;
      a++;
      if(a == 2){	// If 2 bytes have been received --> Checksum is received fully. Set the flag to compute checksum on received data.
	check_chksum = 1;
	chksum_flag = 0;
	a = 0;
	//continue;
      } else {
	continue;
      }
    }
	
    /* Check for Start of Frame indicator and set the flag for 
     * indicating that the next byte will correspond to length.
     */
    /* If 2 # are received, then throw away all the data for the 
     * record as ABLE will resend the entire record and not just the packet.
     */
        
    /* Newline indicates that all the data for that record is 
     * received and we can print it to the file.
     */


    /* Compute checksum, if OK, send ACK, write data to 
     * temporary buffer and the FILE. If not, send a NACK
     */
    if (check_chksum == 1) {
      check_chksum = 0;//Reinitialize counters to 0 for the next packet
      chksum_flag = 0;
      sof_data = 0;
      comp_crc = calc_crc(length,temp_buffer);	//Compute checksum
      low =(char) (comp_crc & 0x00FF);	//Break checksum into high and low for comparision
      high =(char) (comp_crc >> 8);
      fprintf(debug,"Checksum computed .... calc is %d--%d ...... received is %d--%d\n",low,high,chk_sum[0],chk_sum[1]);
      int q = 0;	
               		   
      time_t t = time(NULL);	//Get current system time
      struct tm tm = *localtime(&t);	//convert to struct
      fprintf(debug,"%d %d:%d:%d\n", tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
      fprintf(debug,"DATA : %s", temp_buffer);


      if(low == chk_sum[0] && high == chk_sum[1]) {
	fprintf(debug,"Checksum matched\n");
	char chq='*';	//'*' is the ACK

	if (write( gPortFd, &(chq), 1 )!= 1)	{
	  fprintf(debug,"Sent ACK\n");
	  printf("couldnt write");
	}

	palint = 0;
                
	for (q=0;q < length-2;q++)	{
	  final_char[q+(old_length)] = temp_buffer[q+2];
	  temp_buffer[q+2] = NULL;
	}

	for(q=0;q<1000;q++) {
	  temp_buffer[q] = NULL;	//Put all characters to NULL.
	}

	old_length += length-2;	//Increment total length by length-2.
	if (end_packet == 0) {
	  if(print_char == 1)	{
	    print_char = 0;
	    for (x = 0; x<old_length; x++) {	//Before printing, check the character to prevent garbage.
	      if(((final_char[x]>44)&&(final_char[x]<58))||
		 ((final_char[x]>64)&&(final_char[x]<91))||
		 ((final_char[x]>96)&&(final_char[x]<126))||
		 (final_char[x] == 9)||(final_char[x] == '\n')||
		 (final_char[x] == 10)||(final_char[x] == 46)) {
		printf("%c",final_char[x]);	//Printf for Sender to read from
		fprintf (fp,"%c",final_char[x]);//Printing to 2 files.
		fprintf (fp1,"%c",final_char[x]);
	      }
	    }
	    old_length = 0;
	  } else {
	    fprintf(debug,"Not Printing \n");
	  }
		
	} else {
	  any_one_here = 0;
	  break;
	}
        
      } else   {	//If Not matched, send NACK '&'.
	fprintf(debug,"Checksum mismatch !!!!!!!!!!!!!!\n");        
	char chet='&';
                
	if(write(gPortFd, &(chet), 1 )!= 1) {
	  printf("couldnt write");
	}
                
	for(q=0;q<300;q++) {
	  temp_buffer[q] = NULL;	//Put all characters to NULL.
	}
      }
      ch = 0;
    }
  }
  //    system("echo powersave > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");

  any_one_here = 0;
  fprintf(debug,"Done with one cycle ... returning to onTimer\n");
  return(1);
}

/***************************************************************************/
/**
*  Concatenates source to the destination, but makes sure that the 
*  destination string (including terminating null), doesn't exceed maxLen.
*
*  @param   dst      (mod) String to concatnate onto.
*  @param   src      (in)  String to being added to the end of @a dst.
*  @param   maxLen   (in)  Maximum length that @a dst is allowed to be.
*
*  @return  A pointer to the destination string.
*/

char *StrMaxCat( char *dst, const char *src, size_t maxLen )
{
size_t	dstLen = strlen( dst );

if ( dstLen < maxLen )
{
    StrMaxCpy( &dst[ dstLen ], src, maxLen - dstLen );
}

return dst;

} /* StrMaxCat */

/***************************************************************************/
/**
*   Copies the source to the destination, but makes sure that the 
*   destination string (including terminating null), doesn't exceed 
*   maxLen.
*
*   @param  dst     (out) Place to store the string copy.
*   @param  src     (in)  String to copy.
*   @param  maxLen  (in)  Maximum number of characters to copy into @a dst.
*
*   @return A pointer to the destination string.
*/

char *StrMaxCpy( char *dst, const char *src, size_t maxLen )
{
if ( maxLen < 1 )
{
    /*
     * There's no room in the buffer?
     */

    return "";
}

if ( maxLen == 1 )
{
    /*
     * There's only room for the terminating null character
     */

    dst[ 0 ] = '\0';
    return dst;
}

/*
 * The Visual C++ version of strncpy writes to every single character
 * of the destination buffer, so we use a length one character smaller
 * and write in our own null (if required).
 *
 * This allows the caller to store a sentinel in the last byte of the
 * buffer to detect overflows (if desired).
 */

strncpy( dst, src, maxLen - 1 );
if (( strlen( src ) + 1 ) >= maxLen )
{
    /*
     * The string exactly fits, or probably overflows the buffer.
     * Write in the terminating null character since strncpy doesn't in
     * this particular case.
     *
     * We don't do this arbitrarily so that the caller can use a sentinel 
     * in the very end of the buffer to detect buffer overflows.
     */

    dst[ maxLen - 1 ] = '\0';
}

return dst;

} /* StrMaxCpy */

/***************************************************************************
*
*  Usage
*
****************************************************************************/

void Usage()
{
fprintf( stderr, "Usage: sertest [option(s)]\n" );
fprintf( stderr, "  Download a program via serial/i2c\n" );
fprintf( stderr, "\n" );
fprintf( stderr, "  -b, --baud=baud   Set the baudrate used\n" );
fprintf( stderr, "  -d, --debug       Turn on debug output\n" );
fprintf( stderr, "  -h, --help        Display this message\n" );
fprintf( stderr, "  -p, --port=port   Set the I/O port\n" );
fprintf( stderr, "  -v, --verbose     Turn on verbose messages\n" );

} // Usage


