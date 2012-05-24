#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>


int detectLinkBreakage(char ip_addr[], char port[])
{
  int sockdes, rv;//sockdes->socket descriptor, rv->return value.
  struct addrinfo help, *server_addrinfo, *info;//TCP connection variables.

  memset(&help, 0, sizeof help);
  help.ai_family = AF_UNSPEC;
  help.ai_socktype = SOCK_STREAM;

  if ((rv = getaddrinfo(ip_addr, port, &help, &server_addrinfo)) != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
      return 1;
  }

  // loop through all the results and connect to the first we can
  for (info = server_addrinfo; info != NULL; info = info->ai_next) {
       if ((sockdes = socket(info->ai_family, info->ai_socktype,info->ai_protocol)) == -1) {
           continue;
       }
       // connection to the server is initiated after which this program halts to listen 
       if (connect(sockdes, info->ai_addr, info->ai_addrlen) == -1) {
           close(sockdes);
	   if (errno == ECONNREFUSED)
	       return 0;
           continue;
       }
       break;
  }

  close(sockdes);

  if (info == NULL)
      return 1;
  else
      return 0;
}

void sendMail(FILE *fp, char *sub, char *mail_body)
{
  char buffer[50];
  int temp;

  sprintf(buffer, "\nSending mail\n");
  temp = fwrite(buffer, sizeof(char), strlen(buffer), fp);

  memset(buffer, 0, sizeof(buffer));
  sprintf(buffer, "mail -s %s mlsichit@ncsu.edu < %s", sub, mail_body); 
  system((char *)buffer);
}

int checkLogFile(char *log_file)
{
  FILE *fp1;
  int init, final;

  fp1 = fopen(log_file, "rb+");
  fseek(fp1, 0L, SEEK_END);
  final = ftell(fp1);
  while (1) {
         init = final;
  	 fseek(fp1, 0L, SEEK_END);
  	 final = ftell(fp1);
	 if (init == final) {
  	     fclose(fp1);
	     return 1;
	 }
  }
  
  fclose(fp1);

  return 0;
}

int main( int argc, char *argv[] )
{
  FILE *fp1, *fp2;//fp1->used to write into backup, fp2->to read command output.
  int temp;
  char buffer[100];//buffer to store command output.
  time_t rawtime, start, end;//timestamp.
  double dif;
  struct tm * timeinfo;//timestamp.
  int connected = 1;//status variable to record connection status.

  if (argc != 6) {
      printf("\nWrong arguments entered");
      exit;
  }

  fp1 = fopen(argv[1], "a+");
  while(1) {
     sleep(5);

     time ( &rawtime );
     timeinfo = localtime ( &rawtime );
     sprintf(buffer, "\nThe current date/time is: %s\n", asctime(timeinfo));
     temp = fwrite(buffer, sizeof(char), strlen(buffer), fp1);

     fp2 = popen("ifconfig", "r");
     while (fgets(buffer, sizeof(buffer)-1, fp2) != NULL)
            temp = fwrite(buffer, sizeof(char), strlen(buffer), fp1);
     pclose(fp2);
     fp2 = popen("iwconfig wlan0", "r");
     while (fgets(buffer, sizeof(buffer)-1, fp2) != NULL)
            temp = fwrite(buffer, sizeof(char), strlen(buffer), fp1);
     pclose(fp2);
     fp2 = popen("dmesg", "r");
     while (fgets(buffer, sizeof(buffer)-1, fp2) != NULL)
            temp = fwrite(buffer, sizeof(char), strlen(buffer), fp1);
     pclose(fp2);

     if (detectLinkBreakage(argv[2], argv[3])) {
	 if (connected == 0) {//was disconnected earlier.
	     time(&end);
	     dif = difftime (end,start);
	     if (dif > 43200)
	         sendMail(fp1, "12 hours ended!! No connectivity!!", argv[5]);
         } else {//disconnected first time.
	     connected = 0;
	     time(&start);
	     sendMail(fp1, "Link Broken!!", argv[5]);
	 }    
     } else {
	if (connected == 0) {//was disconnected earlier.
	    connected = 1;
	    if (checkLogFile(argv[4]) == 1) {//Checking if logfile is being written into.
	        sendMail(fp1, "Connectivity back, logfile written", argv[5]);
	    } else {
	        sendMail(fp1, "Connectivity back, logfile NOT written", argv[5]);
	    } 
	}
     }
  }
  fclose(fp1);

  return 0;
}
