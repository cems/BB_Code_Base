#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <strings.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/sockios.h>
#include <string.h>
#include "session.h"

/*  Invoke sender with the sender's hostname and receiver'IP as an argument */
int main(int argc, char *argv[])
{
	char ip[20] = {0};
	session_data s_data;
        char host_name[10] = {0};


        if (argc != 3 ) {
                printf("Usage : session <hostname> <remote-ipaddress>\n");
                return 1;
        }


        memcpy(host_name, argv[1], strlen(argv[1]));
        memcpy(ip, argv[2], strlen(argv[2]));

        pthread_t session;

        s_data.ip = ip;
        s_data.host_name = host_name;

        /* Invoke session transfer which builds all the session parameters and learns the new port to connect to the server */
        pthread_create(&session, NULL, session_transfer, (void *)&s_data);
        pthread_join(session, NULL);

	return 0;
}
