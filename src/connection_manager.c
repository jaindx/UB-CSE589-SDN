//
// Created by Jain on 5/8/2021.
//

#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/queue.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include "../include/connection_manager.h"
#include "../include/global.h"
#include "../include/control_handler.h"
#include "../include/control_header.h"


fd_set master_list, watch_list;
int head_fd;
int sock_present = -1;
extern struct timeval tv;
FILE *file_fp;
void main_loop()
{
    last_pkt = (char *)malloc(1024);
    one_last_pkt = (char *)malloc(1024);

    my_router_id = -1;
    int selret, sock_index, fdaccept;
    file_fp =NULL;
    while(TRUE)
    {

        struct timeval p_timer;
        p_timer.tv_sec = t_timer.tv_sec;
        p_timer.tv_usec = t_timer.tv_usec;

        if (router_socket > 0)
        {
            FD_SET(router_socket, &master_list);
            if (router_socket > head_fd)
            {
                head_fd = router_socket;
            }
        }

        if (data_control_socket >0)
        {
            FD_SET(data_control_socket, &master_list);
            if (data_control_socket > head_fd)
            {
                head_fd = data_control_socket;
            }
        }

        if (data_socket > 0)
        {
            FD_SET(data_socket,  &master_list);
            if (data_socket > head_fd)
            {
                head_fd = data_socket;
            }
        }

        watch_list = master_list;
        selret = select(head_fd+1, &watch_list, NULL, NULL, &p_timer);


        if(selret < 0)
        {
            ERROR("select failed.");
        }
        else if (selret == 0)
        {
            t_timer.tv_sec=g_interval;
            t_timer.tv_usec=0;

            update_inactive_peers ();

            if (router_socket>0)
            {
                send_routing_updates ();
            }
            continue;
        }

        t_timer.tv_sec = p_timer.tv_sec;
        t_timer.tv_usec = p_timer.tv_usec;

        for(sock_index=0; sock_index<=head_fd; sock_index+=1){

            if(FD_ISSET(sock_index, &watch_list)){

                if(sock_index == control_socket)
                {
                    fdaccept = new_control_conn(sock_index);
                    FD_SET(fdaccept, &master_list);
                    if(fdaccept > head_fd) head_fd = fdaccept;
                }

                else if (sock_index == data_control_socket)
                {
                    struct sockaddr_in r2_addr;
                    int caddr_len = 0;
                    memset(&r2_addr, 0, sizeof(r2_addr));
                    data_socket = accept(sock_index, (struct sockaddr *)&r2_addr, &caddr_len);
                }

                else if(sock_index == router_socket)
                {
                    process_routing_updates (sock_index);
                    print_g_routing_table();
                }

                else if(sock_index == data_socket)
                {
                    route_packets (sock_index);
                }

                else
                {
                    if(isControl(sock_index))
                    {
                        if(!control_recv_hook(sock_index))
                            FD_CLR(sock_index, &master_list);
                    }
                        //else if isData(sock_index);
                    else ERROR("Unknown socket index");
                }
            }
        }
    }
}

void init()
{
    control_socket = create_control_sock();

    //router_socket and data_socket will be initialized after INIT from controller

    FD_ZERO(&master_list);
    FD_ZERO(&watch_list);

    /* Register the control socket */
    FD_SET(control_socket, &master_list);
    head_fd = control_socket;

    main_loop();
}

void
send_routing_updates (void)
{
    if (router_socket<=0)
    {
        return;
    }
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *payload=NULL;
    uint16_t port, cost;
    uint32_t ip_addr;
    int serverlen = 0;

    payload =(char *)malloc(8+(g_num_of_routers*12));
    memset (payload, 0, 8+(g_num_of_routers*12));
    uint16_t num_of_routers = htons(g_num_of_routers);
    uint16_t rport = htons(my_router_port);
    uint32_t rip = htonl(my_router_ip);

    memcpy(payload, &num_of_routers, 2);
    memcpy(payload+2, &rport, 2);
    memcpy(payload+4, &rip, 4);

    char *offset = payload+8;
    uint32_t router_ip;
    uint16_t port1,router_id,cost1;
    for (int i=0; i < g_num_of_routers; i++)
    {

        if ((g_peers[i].cost == 65535) ||
            (g_peers[i].cost == 0))
        {
            continue;
        }


        for (int j=0; j < g_num_of_routers; j++)
        {
            router_ip = htonl(g_peers[j].router_ip);
            port1 = htons(g_peers[j].port1);
            router_id = htons(g_peers[j].router_id);

            memcpy (offset+(j*12), &router_ip,4);
            memcpy (offset+4+(j*12), &port1, 2);
            memcpy (offset+8+(j*12), &router_id, 2);
            cost = get_cost(g_peers[j].router_id);
            cost1 = htons(cost);
            memcpy (offset+10+(j*12), &cost1, 2);

        }

        uint32_t router_ip = htonl(g_peers[i].router_ip);
        uint16_t router_port = htons(g_peers[i].port1);

        bzero((char *) &serveraddr, sizeof(serveraddr));
        serveraddr.sin_family = AF_INET;
        serveraddr.sin_port = router_port;
        memcpy(&serveraddr.sin_addr.s_addr,&router_ip,4);

        int sent_byte = 0 ;
        serverlen = sizeof(serveraddr);

        sent_byte = sendto(router_socket, payload, 68, 0, &serveraddr, serverlen);
    }

    if (payload)
        free(payload);
    return;
}

void process_routing_updates (int sockfd)
{
    int recv_length = 0;
    int buf_size = 8 + (g_num_of_routers * 12);

    char *buf= (char*) malloc(buf_size);

    struct sockaddr clientaddr;
    int clientlen =sizeof(clientaddr);

    memset(buf,0,buf_size);

    memset(&clientaddr, 0,sizeof(clientaddr));

    recv_length = recvfrom(sockfd, buf, buf_size, 0, (struct sockaddr *) &clientaddr, &clientlen);

    uint16_t num_of_update, src_router_port, peer_router_id;
    uint32_t src_ip_addr;

    num_of_update = src_router_port = peer_router_id = 0;
    src_ip_addr = 0;

    memcpy (&num_of_update, buf, 2);
    memcpy (&src_router_port, buf+2, 2);
    memcpy (&src_ip_addr, buf+4, 4);

    num_of_update = ntohs(num_of_update);
    src_router_port = ntohs(src_router_port);
    src_ip_addr = ntohl(src_ip_addr);

    get_router_id(src_ip_addr, src_router_port, &peer_router_id);
    update_active_peers (peer_router_id);

    char *off = buf+8;
    struct rtable_entry PeerUpdate[5];
    memset(&PeerUpdate, 0, sizeof(PeerUpdate));

    for (int i=0; i < g_num_of_routers; i++)
    {
        uint32_t ip = 0;
        uint16_t port, cost, router_id;
        port = cost = router_id = 0;
        memcpy (&ip, off + i*12, 4);
        memcpy (&port, off+4+(i*12), 2);
        memcpy (&router_id, off+8+(i*12), 2);
        memcpy (&cost, off+10+(i*12), 2);

        PeerUpdate[i].router_id = ntohs(router_id);
        PeerUpdate[i].next_hop_id = peer_router_id;
        PeerUpdate[i].cost = htons(cost);
    }
    Distance_Vector_Update(PeerUpdate);
    return;
}

int Cost_Reach (uint16_t router_id, uint16_t *cost, uint16_t *peer)
{
    int status=0;

    for (int i = 0; i<g_num_of_routers; i++)
    {
        if (g_peers[i].router_id == router_id)
        {
            *cost = g_peers[i].cost;
            status++;
        }
    }

    for (int i = 0; i<g_num_of_routers; i++)
    {
        if (g_routing_table[i].router_id == router_id)
        {
            *peer = g_routing_table[i].cost;
            status++;
        }
    }

    if(status==2)
        return 1;
    else
        return 0;
}


int Cost_to_Reach (uint16_t router_id, uint16_t *cost, uint16_t *peer)
{
    for (int i = 0; i<g_num_of_routers; i++)
    {
        if (g_routing_table[i].router_id == router_id)
        {
#if 0
            if (is_near[i])
            {
                *cost = g_peers[i].cost;
                *peer = router_id;
            }
            else
#endif
            {
                *cost = g_routing_table[i].cost;
                *peer = g_routing_table[i].next_hop_id;
            }
            return 1;
        }
    }
    return 0;
}






int find_index_in_rtable (uint16_t router_id)
{
    for (int i = 0 ;i <g_num_of_routers; i++)
    {
        if (g_routing_table[i].router_id == router_id)
        {
            return i;
        }
    }
    return -1;
}


int find_index_with_ip (uint32_t router_ip)
{
    for (int i = 0 ;  i<g_num_of_routers; i++)
    {
        if (g_peers[i].router_ip == router_ip)
        {
            return i;
        }
    }

    return -1;
}

bool is_inactive_peer (uint16_t router_id)
{
    int index = -1;
    index = find_index_in_rtable (router_id);
    if (index<0)
    {
        return FALSE;
    }
    if (g_peers[index].cost != 0)
        return FALSE;

    if (inactive_peer_count[index]<=0)
        return TRUE;

    return FALSE;
}

void Distance_Vector_Update (struct rtable_entry PeerUpdate[5])
{
    for (int i = 0; i < g_num_of_routers; i++)
    {
        if (PeerUpdate[i].router_id ==  my_router_id)
            continue;
#if 0
		if (PeerUpdate[i].next_hop_id == 4)
		if (PeerUpdate[i].router_id == 5)
		if (my_router_id == 1)
			continue;
#endif

        if (is_inactive_peer(PeerUpdate[i].router_id))
            continue;

        int index = find_index_in_rtable (PeerUpdate[i].router_id);
        if (index == -1)
            continue;

        if ((65535 == PeerUpdate[i].cost))
        {
            if ((PeerUpdate[i].next_hop_id != g_routing_table[index].next_hop_id))
                continue;
            else
            {
                g_routing_table[index].cost = 65535;
                g_routing_table[index].next_hop_id = 65535;
                continue;
            }
        }

        uint16_t curr_cost1,curr_cost2;
        uint16_t curr_peer;
        if (g_routing_table[index].next_hop_id == PeerUpdate[i].next_hop_id)
        {
            curr_cost2 = 0;
            Cost_to_Reach (PeerUpdate[i].next_hop_id, &curr_cost2, &curr_peer);
            g_routing_table[index].cost = curr_cost2 + PeerUpdate[i].cost;
            curr_cost2 = 0;
            continue;
        }

        curr_cost1 = curr_cost2 = curr_peer = 0;
        Cost_to_Reach (PeerUpdate[i].router_id, &curr_cost1, &curr_peer);

        if (PeerUpdate[i].cost >= curr_cost1)
            continue;

        curr_peer = 0;
        Cost_Reach(PeerUpdate[i].next_hop_id, &curr_cost2, &curr_peer);

        if (PeerUpdate[i].cost + curr_cost2 >= curr_cost1)
            continue;

        g_routing_table[index].cost = PeerUpdate[i].cost+curr_cost2;
        g_routing_table[index].next_hop_id = PeerUpdate[i].next_hop_id;
    }
    return;
}

void update_rtable_inactive(uint16_t router_id)
{
    for (int i =0; i<g_num_of_routers; i++)
    {
        if (g_routing_table[i].cost == 0 )
            continue;


        if ((g_routing_table[i].router_id == router_id )
            || (g_routing_table[i].next_hop_id == router_id))
        {
            g_routing_table[i].cost = 65535;
            g_routing_table[i].next_hop_id = 65535;
        }
    }
}

void update_inactive_peers ()
{
    for (int i =0; i<g_num_of_routers; i++)
    {
        if (g_interval == 0)
            continue;

        if (g_peers[i].cost == 0)
            continue;

        if (g_peers[i].cost == 65535)
            continue;

        inactive_peer_count[i]  -= g_interval;
        if (inactive_peer_count[i] <= 0)
        {
            uint16_t router_id = g_peers[i].router_id;
            update_rtable_inactive (router_id);
            inactive_peer_count[i] = 0;
        }
    }
}
void update_active_peers (uint16_t router_id)
{
    int index = -1;
    index = find_index_in_rtable (router_id);
    if ((index < 0) || (g_interval == 0))
    {
        return;
    }
    inactive_peer_count[index] = 3 * g_interval;
    return;
}

void init_active_peers ()
{
    for (int i=0; i<g_num_of_routers; i++)
    {
        inactive_peer_count[i] = 3 * g_interval;
    }
    return;
}

int route_packets(int sock_index)
{
    char *payload;
    int payload_len = 12+1024;

    uint32_t fin = 0;
    /* Get control payload */
    payload = (char *) malloc(payload_len+1);
    bzero(payload, payload_len+1);

    if(recvALL(sock_index, payload, payload_len) < 0)
    {
        free(payload);
        return FALSE;
    }

    memcpy (one_last_pkt, last_pkt, 1024);
    memcpy (last_pkt, payload+12, 1024);

    uint32_t router_ip;
    uint8_t tid, ttl;
    tid = ttl = 0;
    char filename[30];

    memcpy (&fin, payload+8,4);
    memset (filename, 0,30);
    memcpy (&router_ip, payload, 4);
    router_ip = ntohl(router_ip);

    if (my_router_ip == router_ip)
    {
        memcpy (&ttl, payload+4, 1);
        sprintf (filename,"file-%d",ttl);
        if (NULL == file_fp)
        {
            file_fp = fopen(filename, "a+");
        }
    }
    else
    {
        memcpy (&ttl, payload+5, 1);
        uint16_t seq =0;
        memcpy (&seq, payload+6, 2);
        ttl = ttl -1;
        if (ttl == 0)
        {
            goto free;
        }
        memcpy (payload+5, &ttl, 1);
        Send_to_Next_Hop(router_ip, payload);
        goto free;
    }

    free:
    if (fin != 0 )
    {
        if (data_socket >0)
        {
            FD_CLR(data_socket,  &master_list);
            close(data_socket);
            data_socket = -1;
        }

        if (sock_present > 0)
        {
            FD_CLR(sock_present,  &master_list);
            close(sock_present);
            sock_present = -1;
        }

        if (file_fp)
            fclose(file_fp);

        file_fp = NULL;
    }
    if (payload)
        free(payload);
    return;
}

void Send_to_Next_Hop (uint32_t router_ip, char *payload)
{

    int index = find_index_with_ip(router_ip);

    int next_hop = g_routing_table[index].next_hop_id;
    int sock_to_send = -1;
    if (sock_present < 0)
    {
        sock_to_send = connect_to_peer (g_peers[next_hop-1].router_ip);
        sock_present = sock_to_send;
    }
    else
        sock_to_send  = sock_present;

    sendALL(sock_to_send, payload, 12+1024);

    return;
}