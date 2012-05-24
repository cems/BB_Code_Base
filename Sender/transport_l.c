#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>
#include "session.h"
#include "transport_l.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>

/* SCRIPT call */
/* Check if the connection is up, If not configure the wireless interface */

void transport_intf_flap() {
        if (!system("./wireless_up.sh")) {
                /* wlan0 is not seen as a part of ifconfig */
                log_mesg("No wireless Device found\n");
        } else if (!system("./wireless_assoc.sh")) {
                /* Wireless association  is not seen as a part of iwconfig */
                log_mesg("No access point\n");
                system("iwconfig wlan0 essid TING");
                system("ifconfig wlan0 10.10.10.1/24");
                system("route add -net 10.20.1.0 netmask 255.255.255.0 gw 10.10.10.10");
        } else if (!system("./wireless_ip.sh")) {
                log_mesg("NO IP");
                /* IP is not seen as a part of ifconfig wlan0 */
                system("iwconfig wlan0 essid TING");
                system("ifconfig wlan0 10.10.10.1/24");
                system("route add -net 10.20.1.0 netmask 255.255.255.0 gw 10.10.10.10");
        }
}

/* Sends a packet by invoking the sendto() system  call */
void transport_send_packet(int socket_t, char *packet, int packet_len, int *error, struct sockaddr_in *address, int address_len) {

	int result;

	result = sendto(socket_t, packet, packet_len, 0, (struct sockaddr *)address, (socklen_t)address_len);
        
	if (result == -1) {
		log_mesg("Packet not sent size %d. \n", packet_len);
	}

}


/* Receives a packet by invoking the recvfrom() system  call */
int transport_recv_packet(int socket_t, char *packet, int *packet_len, int *error, struct sockaddr_in *address, int *address_len) {

        int result = 0;

        result = recvfrom(socket_t, packet, PKT_SIZE_MAX, 0, (struct sockaddr *)address, (socklen_t *)address_len);

        if (*packet_len == -1) {
                log_mesg("Dumb packet Received.\n");
        }

        return result;
}


/* Check layer 2 connection.
 *
 * Create a TCP socket. Establish the connection. 
 * Corresponding resilt is saven in errno. 
 * 101 = Network is unreachable
 * 110 = Connection timed out
 * 113 = No route to host 
 *
 *
 * Success
 * 111 = Connection refused
 * Based on the results we can check if the connection is up or not
 */
int check_wireless_connectivity(session_t *session) {

        log_mesg("Inside check_wireless_connectivity\n");

        int errno;
        int sockfd;

        struct sockaddr_in servaddr;
        sockfd = socket(AF_INET, SOCK_STREAM, 0);

        bzero(&servaddr,sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = inet_addr(inet_ntoa(session->address.sin_addr));
        servaddr.sin_port=htons(32000);

        connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
        if (errno == 113 || errno == 110 || errno == 101 ) { 
             log_mesg("Error number: %d No wireless connectivity\n ", errno);
             close(sockfd);
             return 1;
        }  
        
        log_mesg("error number : %d\n" , errno); 
        close(sockfd);

        return 0;
}
