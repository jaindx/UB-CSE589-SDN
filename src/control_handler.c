//
// Created by Jain on 5/8/2021.
//
#include "../include/global.h"
#include "../include/network_util.h"
#include "../include/control_header.h"
#include "../include/author.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/queue.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include "../include/globals.h"
#include "../include/control_responses.h"
#include "../include/router_info.h"
#include "../include/connection_manager.h"

#ifndef PACKET_USING_STRUCT
#define CNTRL_CONTROL_CODE_OFFSET 0x04
#define CNTRL_PAYLOAD_LEN_OFFSET 0x06
#endif

struct ControlConn
{
    int sockfd;
    LIST_ENTRY(ControlConn) next;
}*connection, *conn_temp;
LIST_HEAD(ControlConnsHead, ControlConn) control_conn_list;

int create_control_sock()
{
    int sock;
    struct sockaddr_in control_addr;
    socklen_t addrlen = sizeof(control_addr);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
    ERROR("socket() failed");

    /* Make socket re-usable */
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int)) < 0)
    ERROR("setsockopt() failed");

    bzero(&control_addr, sizeof(control_addr));

    control_addr.sin_family = AF_INET;
    control_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    control_addr.sin_port = htons(CONTROL_PORT);

    if(bind(sock, (struct sockaddr *)&control_addr, sizeof(control_addr)) < 0)
    ERROR("bind() failed");

    if(listen(sock, 5) < 0)
    ERROR("listen() failed");

    LIST_INIT(&control_conn_list);

    return sock;
}

int new_control_conn(int sock_index)
{
    int fdaccept, caddr_len;
    struct sockaddr_in remote_controller_addr;

    caddr_len = sizeof(remote_controller_addr);
    fdaccept = accept(sock_index, (struct sockaddr *)&remote_controller_addr, &caddr_len);
    if(fdaccept < 0)
    ERROR("accept() failed");

    /* Insert into list of active control connections */
    connection = malloc(sizeof(struct ControlConn));
    connection->sockfd = fdaccept;
    LIST_INSERT_HEAD(&control_conn_list, connection, next);

    return fdaccept;
}

void remove_control_conn(int sock_index)
{
    LIST_FOREACH(connection, &control_conn_list, next) {
        if(connection->sockfd == sock_index) LIST_REMOVE(connection, next); // this may be unsafe?
        free(connection);
    }

    close(sock_index);
}

bool isControl(int sock_index)
{
    LIST_FOREACH(connection, &control_conn_list, next)
    if(connection->sockfd == sock_index) return TRUE;

    return FALSE;
}

bool control_recv_hook(int sock_index)
{
    char *cntrl_header, *cntrl_payload;
    uint8_t control_code;
    uint16_t payload_len;

    cntrl_header = (char *) malloc(sizeof(char)*CNTRL_HEADER_SIZE);
    bzero(cntrl_header, CNTRL_HEADER_SIZE);

    if(recvALL(sock_index, cntrl_header, CNTRL_HEADER_SIZE) < 0){
        remove_control_conn(sock_index);
        free(cntrl_header);
        return FALSE;
    }
    
    printf("** Control Header : which it does not read because of strange format %s \n",cntrl_header);

#ifdef PACKET_USING_STRUCT
        BUILD_BUG_ON(sizeof(struct CONTROL_HEADER) != CNTRL_HEADER_SIZE);

        struct CONTROL_HEADER *header = (struct CONTROL_HEADER *) cntrl_header;
        control_code = header->control_code;
        payload_len = ntohs(header->payload_len);
        uint16_t response_time = header->response_time;
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET,(struct in_addr *)&header->dest_ip_addr,ip,INET_ADDRSTRLEN);
        printf("Fields derived from the control header\n");
        printf("control code: %d payload_len : %d \n",control_code,payload_len);
        printf("response time : %d dest_ip : %s \n",response_time,ip);
#endif
#ifndef PACKET_USING_STRUCT
    memcpy(&control_code, cntrl_header+CNTRL_CONTROL_CODE_OFFSET, sizeof(control_code));
    memcpy(&payload_len, cntrl_header+CNTRL_PAYLOAD_LEN_OFFSET, sizeof(payload_len));
    payload_len = ntohs(payload_len);
#endif

    free(cntrl_header);

    if(payload_len != 0){
        cntrl_payload = (char *) malloc(sizeof(char)*payload_len);
        bzero(cntrl_payload, payload_len);

        if(recvALL(sock_index, cntrl_payload, payload_len) < 0){
            remove_control_conn(sock_index);
            free(cntrl_payload);
            printf("payload length is zero \n");
            return FALSE;
        }
    }

    switch(control_code){
        case 0: author_response(sock_index);
            break;

        case 1: printf("\nready for init initialization :P \n");
                init_response(sock_index, cntrl_payload);
                printf("router has been initialized\n");
                isRouterInitialized = TRUE;
                break;

        case 2: printf("Routing table\n");
                routing_table_response(sock_index);
                break;
                
        case 3: printf("Updating router costs\n");
                router_update(sock_index,cntrl_payload);
                break; 

        case 4: printf("Crash Router\n");
                crashRouter(sock_index);
                break;

        case 5: printf("\nFiles to send\n");
                sendFile(sock_index,cntrl_payload,payload_len);
                break;
    }

    if(payload_len != 0) free(cntrl_payload);
    return TRUE;
}
