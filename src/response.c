//
// Created by Jain on 5/8/2021.
//

#include "../include/global.h"
#include "../include/control_header.h"
#include "../include/connection_manager.h"
#include "../include/network_util.h"

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <sys/queue.h>
#include <unistd.h>

extern fd_set master_list;
extern int head_fd;
extern int router_socket;
extern int data_control_socket;

#define AUTHOR_RESPONSE "I, jainzach, have read and understood the course academic integrity policy."

void author_response(int sock_index)
{
    uint16_t payload_len, response_len;
    char *cntrl_response_header, *cntrl_response_payload, *cntrl_response;

    payload_len = sizeof(AUTHOR_RESPONSE)-1;
    cntrl_response_payload = (char *) malloc(payload_len);
    memcpy(cntrl_response_payload, AUTHOR_RESPONSE, payload_len);

    cntrl_response_header = create_response_header(sock_index, 0, 0, payload_len);

    response_len = CNTRL_RESP_HEADER_SIZE+payload_len;
    cntrl_response = (char *) malloc(response_len);

    memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
    free(cntrl_response_header);

    memcpy(cntrl_response+CNTRL_RESP_HEADER_SIZE, cntrl_response_payload, payload_len);
    free(cntrl_response_payload);

    sendALL(sock_index, cntrl_response, response_len);

    free(cntrl_response);
}

void init_response (int sock_index,char *cntrl_payload)
{
    uint16_t  num_of_routers, interval;

    memcpy(&num_of_routers,cntrl_payload,2);
    memcpy(&interval,cntrl_payload+2,2);

    num_of_routers = ntohs(num_of_routers);
    interval = ntohs(interval);

    g_num_of_routers = (int) num_of_routers;
    g_interval =  (int) interval;

    t_timer.tv_sec = g_interval;
    t_timer.tv_usec = 0;


    char *with_offset = cntrl_payload + 4;
#if 0
    for (int i = 0; i <num_of_routers; i++)
	{
		memset(&g_peers[i],0,sizeof(struct peer_info));
		struct PEER_INFO *ptr = (struct PEER_INFO *) with_offset+(i*12);
		g_peers[i].router_id = ntohs(ptr->router_id);
		g_peers[i].port1 = ntohs(ptr->port1);
		g_peers[i].port2 = ntohs(ptr->port2);
		g_peers[i].cost  = ntohs(ptr->cost);
		g_peers[i].router_ip  = ntohl(ptr->router_ip);
		print_g_peer (&g_peers[i]);
	}
#endif
#if 1
    for (int i =0; i<num_of_routers; i++)
    {
        uint16_t router_id,port1,port2,cost;
        uint32_t router_ip;
        router_id = port1 = port2 = cost =0;
        router_ip = 0;

        memcpy(&router_id, with_offset+(i*12),2);
        memcpy(&port1, with_offset+2+(i*12),2);
        memcpy(&port2, with_offset+4+(i*12),2);
        memcpy(&cost, with_offset+6+(i*12),2);
        memcpy(&router_ip, with_offset+8+(i*12),4);
        memset(&g_peers[i],0,sizeof(struct peer_info));

        g_peers[i].router_id = ntohs(router_id);
        g_peers[i].port1 = ntohs(port1);
        g_peers[i].port2 = ntohs(port2);
        g_peers[i].cost = ntohs(cost);
        g_peers[i].router_ip = ntohl(router_ip);
        print_g_peer (&g_peers[i]);
        if (cost == 0)
        {
            my_router_id = ntohs(router_id);
            my_router_ip = ntohl(router_ip);
            my_router_port = ntohs(port1);
            my_data_port = ntohs(port2);
        }
    }
#endif

    for (int i =0; i <num_of_routers; i++)
    {
        memset(&g_routing_table[i],0,sizeof(struct rtable_entry));
    }

    for (int i=0; i <num_of_routers; i++)
    {
        g_routing_table[i].router_id = g_peers[i].router_id;
        g_routing_table[i].next_hop_id = g_peers[i].router_id;
        g_routing_table[i].cost = g_peers[i].cost;

        if (g_peers[i].cost == 65535)
        {
            g_routing_table[i].cost = 65535;
            g_routing_table[i].next_hop_id = 65535;
        }
        if (g_peers[i].cost == 0)
            is_near[i] = 1;
    }

    init_data_router_sockets (my_router_id);
    send_control_response (sock_index,1,0);
    init_active_peers();

    return;
}

void print_g_peer (struct PEER_INFO *ptr)
{
    print_ip(ptr->router_ip);
    return;
}

void send_control_response (int sock_index, int control_code, int response_code)
{
    uint16_t payload_len, response_len;
    char *cntrl_response_header, *cntrl_response;

    cntrl_response_header = create_response_header(sock_index, control_code, response_code, payload_len);

    response_len = CNTRL_RESP_HEADER_SIZE;
    cntrl_response = (char *) malloc(response_len);

    //Header
    memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
    free(cntrl_response_header);

    sendALL(sock_index, cntrl_response, response_len);

    free(cntrl_response);
    return;
}

void routing_table_response (int sock_index, char *cntrl_payload)
{
    uint16_t payload_len, response_len;
    char *cntrl_response_header, *cntrl_response_payload, *cntrl_response;

    payload_len = 5*sizeof(struct rtable_entry);
    cntrl_response_payload = (char *) malloc(payload_len);
    memset(cntrl_response_payload, 0 ,payload_len);

    cntrl_response_header = create_response_header(sock_index, 2, 0, payload_len);

    for (int i=0; i<g_num_of_routers; i++)
    {
        uint16_t router_id, next_hop_id, cost,pad;
        router_id = next_hop_id = cost= pad =0;

        router_id =  htons(g_routing_table[i].router_id);
        cost = htons(g_routing_table[i].cost);
        next_hop_id = htons(g_routing_table[i].next_hop_id);

        memcpy(cntrl_response_payload+(i*8), &router_id, 2);
        memcpy(cntrl_response_payload+(i*8)+4, &next_hop_id, 2);
        memcpy(cntrl_response_payload+(i*8)+6, &cost, 2);
    }

    response_len = CNTRL_RESP_HEADER_SIZE+payload_len;
    cntrl_response = (char *) malloc(response_len);
    memset (cntrl_response, 0, response_len);

    //Header
    memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
    free(cntrl_response_header);

    //Payload
    memcpy(cntrl_response+CNTRL_RESP_HEADER_SIZE, cntrl_response_payload, payload_len);
    free(cntrl_response_payload);

    free(cntrl_response);
    return;
}

void routing_cost_update (int sock_index, char *cntrl_payload)
{
    uint16_t router_id = 0;
    uint16_t cost = 0;

    memcpy (&router_id,cntrl_payload,2);
    memcpy (&cost,cntrl_payload+2,2);

    router_id = ntohs(router_id);
    cost = ntohs(cost);

    for (int i=0; i<g_num_of_routers; i++)
    {
        if (g_routing_table[i].router_id == router_id)
        {
            if (g_routing_table[i].next_hop_id == router_id)
            {
                g_routing_table[i].cost = cost;

                if (cost == 65535)
                    g_routing_table[i].next_hop_id = cost;
            }
        }
        else if (g_routing_table[i].next_hop_id == router_id)
        {
            if (cost == 65535)
            {
                g_routing_table[i].next_hop_id = 65535;
                g_routing_table[i].cost = 65535;

            }
        }

        if (g_peers[i].router_id == router_id)
        {
            g_peers[i].cost = cost;
        }
    }
    send_control_response (sock_index, 3, 0);
    return;
}

void init_data_router_sockets (int router_id)
{
    for (int i=0;i< g_num_of_routers; i++)
    {
        if (g_peers[i].router_id == router_id)
        {
            init_udp_socket(g_peers[i].port1);
            init_tcp_socket(g_peers[i].port2);
        }
        else
        {
            continue;
        }
    }
    return;
}

void init_udp_socket (int udp_port)
{
    int udp = socket (AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in serverAddr;

    bzero((char *) &serverAddr, sizeof(serverAddr));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(udp_port);

    bind(udp, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
    router_socket = udp;

    FD_SET(udp, &master_list);
    if (udp > head_fd)
    {
        head_fd = udp;
    }

    return;
}

void init_tcp_socket (int tcp_port)
{
    data_control_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in sin;
    memset(&sin,0,sizeof(sin));

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(tcp_port);

    bind(data_control_socket, (struct sockaddr *) &sin, sizeof(sin));
    listen(data_control_socket ,2);
    return;
}

int get_router_info (uint16_t router_id, uint16_t *port, uint32_t  *ip)
{
    for (int i =0 ; i < g_num_of_routers; i++)
    {
        if (g_peers[i].router_id  = router_id)
        {
            *port = g_peers[i].port1;
            *ip = g_peers[i].router_ip;
            return 1;
        }
    }
    return 0;
}

int get_router_id (uint32_t ip, uint16_t port, uint16_t *router_id)
{
    for (int i=0; i < g_num_of_routers; i++)
    {
        if ((g_peers[i].router_ip == ip) &&
            (g_peers[i].port1 == port))
        {
            *router_id = g_peers[i].router_id;
            return 1;
        }
    }
    return 0;
}

uint16_t get_cost (uint16_t id)
{
    for (int i=0; i < g_num_of_routers; i++)
    {
        if (g_routing_table[i].router_id == id)
        {
            return g_routing_table[i].cost;
        }
    }
}

void send_file (int sock_index,char *cntrl_payload, int payload_len)
{
    uint32_t router_ip = 0;
    uint8_t ttl, tid;
    uint16_t seq_num, h_seq_num;
    seq_num = h_seq_num  = 0;
    int filename_len = payload_len - 8;
    char *filename = (char *) malloc(filename_len+1);
    memset(filename,0,filename_len+1);

    ttl = tid  = 0;

    memcpy (&router_ip, cntrl_payload, 4);
    router_ip = ntohl(router_ip);

    memcpy (&ttl, cntrl_payload+4, 1);
    memcpy (&tid, cntrl_payload+5, 1);
    memcpy (&seq_num, cntrl_payload+6, 2);
    memcpy (filename, cntrl_payload+8, filename_len);

    h_seq_num = ntohs(seq_num);

    int index1 = find_index_with_ip(router_ip);
    int next_hop1 = g_routing_table[index1].next_hop_id;

    long int filesize = 0;
#if 1
    if (next_hop1 == 65535)
        return;
#endif
    index1 =  -1;
    for (int i=0 ; i<g_num_of_routers; i++)
    {
        if (g_peers[i].router_id == next_hop1)
        {
            index1 = i;
            break;
        }
    }
    int sock_to_send = connect_to_peer (g_peers[index1].router_ip);
#if 1
    if (sock_to_send < 0)
    {
        return;
    }
#endif
    int fin_sent = 0;
    int fd  = 0;
    char *data = NULL;
    FILE *fp;
    fp  = fopen (filename, "rw+");

    if (fp == 	NULL)
    {
        return;
    }

    if(fp != NULL)
    {
        fseek(fp, 0 , SEEK_END);
        filesize = ftell(fp);
        fseek(fp, 0 , SEEK_SET);

        data = (char *) malloc(filesize+1);
        memset (data,0,filesize+1);
        int rd = fread(data,1,filesize,fp);
        fclose(fp);
    }

    char buffer[1024];

    char *header_payload = (char *) malloc(12);
    memset(header_payload, 0, 12);

    router_ip = htonl(router_ip);
    memcpy (header_payload, &router_ip, 4);
    memcpy (header_payload+4, &tid, 1);
    memcpy (header_payload+5, &ttl, 1);

    int bytes_read = 1024;
    uint32_t padding = 1;
    padding = padding<<31;
    int total = 0;
    long int total_read = 0;
    char *payload = (char *) malloc(12+1024);
    while (1)
    {
        if (total == filesize)
            break;

        memset(payload, 0, 12+1024);
        memset(buffer, 0, 1024);
        memcpy(buffer, data+total, 1024);
        total += bytes_read;

        if (total == filesize)
        {
            fin_sent = 1;
            memcpy (header_payload+8, &padding, 4);
        }

        h_seq_num = htons(seq_num);
        memcpy (header_payload+6, &h_seq_num, 2);
        memcpy (payload, header_payload, 12);
        memcpy (payload+12, buffer, 1024);

        if (bytes_read >0)
        {
            memcpy (one_last_pkt, last_pkt, 1024);
            memcpy (last_pkt, payload+12, 1024);
        }
        seq_num++;
    }
    free (data);
    free(header_payload);
    free(payload);

    send_control_response (sock_index,5,0);
    return;
}

void  last_packet (int sock_index)
{
    uint16_t payload_len, response_len;
    char *cntrl_response_header, *cntrl_response_payload, *cntrl_response;

    payload_len = 1024;
    cntrl_response_payload = (char *) malloc(payload_len);
    memcpy(cntrl_response_payload, last_pkt, 1024);

    cntrl_response_header = create_response_header(sock_index, 7, 0, payload_len);

    response_len = CNTRL_RESP_HEADER_SIZE+payload_len;
    cntrl_response = (char *) malloc(response_len);

    //Header
    memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
    free(cntrl_response_header);

    //Payload
    memcpy(cntrl_response+CNTRL_RESP_HEADER_SIZE, cntrl_response_payload, payload_len);
    free(cntrl_response_payload);

    sendALL(sock_index, cntrl_response, response_len);

    free(cntrl_response);
    return;
}

void  one_last_packet (int sock_index)
{
    uint16_t payload_len, response_len;
    char *cntrl_response_header, *cntrl_response_payload, *cntrl_response;

    payload_len = 1024;
    cntrl_response_payload = (char *) malloc(payload_len);
    memcpy(cntrl_response_payload, one_last_pkt, 1024);

    cntrl_response_header = create_response_header(sock_index, 8, 0, payload_len);

    response_len = CNTRL_RESP_HEADER_SIZE+payload_len;
    cntrl_response = (char *) malloc(response_len);

    //Header
    memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
    free(cntrl_response_header);

    //Payload
    memcpy(cntrl_response+CNTRL_RESP_HEADER_SIZE, cntrl_response_payload, payload_len);
    free(cntrl_response_payload);

    sendALL(sock_index, cntrl_response, response_len);

    free(cntrl_response);
    return;
}

int connect_to_peer (uint32_t router_ip)
{
    int ft_fd = socket(AF_INET, SOCK_STREAM, 0);
    int index = find_index_with_ip (router_ip);
    int server_port = g_peers[index].port2;

    print_ip(router_ip);

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));

    bzero((char *) &sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(server_port);
    uint32_t t_router_ip = htonl(router_ip);
    memcpy(&sin.sin_addr.s_addr,&t_router_ip,4);

    if (connect(ft_fd, (struct sockaddr *)&sin, sizeof (struct sockaddr)))
    {
        close(ft_fd);
        return -1;
    }
    return ft_fd;
}
