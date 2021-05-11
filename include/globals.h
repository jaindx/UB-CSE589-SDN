#ifndef GLOBAL_H_
#define GLOBAL_H_
#include <netinet/in.h>

bool isRouterInitialized;
struct timeval timeout;
fd_set master_list, watch_list;
int head_fd;


int router_crashed;


uint32_t data_ip;
uint16_t data_length, data_code, data_time;

int dSocket[5];
int datafd[5];
struct sockaddr_in dataServeraddr[5];


void create_client_router_sockets();

void create_send_packet(uint32_t destIP, uint8_t ttl, uint8_t transferID,uint16_t seq_no, char *buff,int lastmsg);

FILE *fp;


#endif
