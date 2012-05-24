#include "session.h"

void transport_send_packet(int socket_t, char *packet, int packet_len, int *error, struct sockaddr_in *address, int address_len);
int transport_recv_packet(int socket_t, char *packet, int *packet_len, int *error, struct sockaddr_in *address, int *address_len);
void transport_intf_flap();
int check_wireless_connectivity(session_t *session);
