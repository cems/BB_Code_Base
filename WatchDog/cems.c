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
      system("iwconfig wlan0 essid Ting");
      system("ifconfig wlan0 10.10.10.1/24");
     
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
	   if (errno == ECONNREFUSED) { 
	        return 0;
	   }
           continue;
       }
       break;
  }

  close(sockdes);

  if (info == NULL)
  {
      system("iwconfig wlan0 essid Ting");
      system("ifconfig wlan0 10.10.10.1/24");
      return 1;
  }
  else
      return 0;
}

void onLinkBreakage(FILE *fp, int *flag, int *no_ap)
{
  FILE *fp3;
  int temps;
  char buffer[100];
  int temp;
  if (*flag == 3) {//USB module has been reset once.
      sprintf(buffer, "\nUSB reset did not resolve connectivity, rebooting\n");
      temp = fwrite(buffer, sizeof(char), strlen(buffer), fp);
      *flag = 4;//To reboot board.
      return;
  }

  sprintf(buffer, "\nNot reachable, resetting USB module\n");
  temp = fwrite(buffer, sizeof(char), strlen(buffer), fp);
     
  system("modprobe -r zd1211rw");
  fp3 = popen("iwlist wlan0 scan | grep Ting", "r");
  if (fgets(buffer, sizeof(buffer)-1, fp3) != NULL) {
	    temp = fwrite(buffer, sizeof(char), strlen(buffer), fp);
	    *no_ap = 0;
  }
  else {
		(*no_ap) ++;
		if(*no_ap == 4) {
			sprintf(buffer, "\nAP is not reachable ... going to reboot\n");
	  		temp = fwrite(buffer, sizeof(char), strlen(buffer), fp);
		}
  }
	
  pclose(fp3);
 
  (*flag )++;
}

void log_data(FILE *fp1)
{
     FILE *fp2;
     time_t rawtime;//timestamp.
     struct tm * timeinfo;//timestamp.
     char buffer[100];//buffer to store command output.
     int temp;
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
     system("dmesg -c");
     fp2 = popen("lsmod", "r");
     while (fgets(buffer, sizeof(buffer)-1, fp2) != NULL)
            temp = fwrite(buffer, sizeof(char), strlen(buffer), fp1);
     pclose(fp2);
     return ;
}


// ./watchdog File.txt IP PORT
int main( int argc, char *argv[] )
{
  int no_ap = 0;//Count the number of times AP is not reachable.
  FILE *fp1;//fp1->used to write into backup, fp2->to read command output.
  int flag = 0;//flag used to hold reset state.
  int link_brk = 0;//Count the number of times link break is encountered continously.
  char buffer[30];
  if (argc != 4) {
      printf("\nWrong arguments entered");
      exit(0);
  }

  fp1 = fopen(argv[1], "a+");
  log_data(fp1);
  while(1) {
     
     if (detectLinkBreakage(argv[2], argv[3])) {
	//printf("Link BRK\n");
	link_brk ++;
	 onLinkBreakage(fp1, &flag, &no_ap);
	 if ((flag == 4)||(no_ap == 4))
	     break;
     } else {
//	printf("NO\n");
	sprintf(buffer, "\nEverthing OK .... Sleeping 30 secs\n");
  	fwrite(buffer, sizeof(char), strlen(buffer), fp1);

         flag = 0;
	 link_brk = 0;
     }
   sleep(30);
  }
  log_data(fp1);
  fclose(fp1);
  system("reboot");

}
