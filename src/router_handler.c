#include "../include/global.h"
#include "../include/network_util.h"
#include "../include/control_header.h"
#include "../include/author.h"
#include "../include/globals.h"
#include "../include/control_responses.h"
#include "../include/router_info.h"
#include "../include/connection_manager.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/queue.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

void create_router_sockets()
{
    int i =currRouterIndex;

    ROUTER_PORT = routerPort[i];      
    struct sockaddr_in router_addr;
    socklen_t addrlen = sizeof(router_addr);

    router_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(router_socket < 0)
        ERROR("socket() failed");

    if(setsockopt(router_socket, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int)) < 0)
        ERROR("setsockopt() failed");

    bzero(&router_addr, sizeof(router_addr));

    router_addr.sin_family = AF_INET;
   // router_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // check if someother can be the ip..mostly overthinking
    router_addr.sin_addr.s_addr = destIp[i]; //still added :P
    router_addr.sin_port = ROUTER_PORT;

    if(bind(router_socket, (struct sockaddr *)&router_addr, sizeof(router_addr)) < 0)
        ERROR("bind() failed");

    FD_SET(router_socket, &master_list);
    if(router_socket > head_fd)
        head_fd = router_socket;
}

void create_client_router_sockets()
{
    for(int i =0; i < nRouters; i++)
    {
        //printf("isNeighbor[i]=%d\n",isNeighbor[i]);
        if(isNeighbor[i] == 1)
        {
            int port = ntohs(routerPort[i]);
            rSocket[i];
            socklen_t addrlen = sizeof(serveraddr[i]);
            rSocket[i] = socket(AF_INET, SOCK_DGRAM, 0);
            if(rSocket[i] < 0)
                ERROR("socket() failed");
            if(setsockopt(rSocket[i], SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int)) < 0)
                ERROR("setsockopt() failed");
          //  bzero(&serveraddr[i], sizeof(serveraddr));
            serveraddr[i].sin_family = AF_INET;
            serveraddr[i].sin_port = routerPort[i];              
            serveraddr[i].sin_addr.s_addr = destIp[i];
        }
    }
}


int router_recv_hook(int sock_index) // returns the router's Index which sent the table
{
    char buffer[1000];
    memset(buffer,'\0',1000*sizeof(char));
    int length = recvfrom( sock_index, buffer, sizeof(buffer) , 0, NULL, 0 );
    if ( length < 0 ) 
        ERROR( "recvfrom failed" );
    buffer[length] = '\0';
//    printf( "%d bytes: '%s'\n", length, buffer );

    char *tok,*cmd;
    int rs_routerid, senderRouterIndex;
    int r_routerid,r_nextHop,r_cost,index,updateCost;
    
    tok = strtok(buffer," ");
    if(tok =="c")
    {
        tok = strtok(NULL," ");
        index = atoi(tok);
        
        tok = strtok(NULL," ");
        updateCost = atoi(tok);
        
    }
    else
    {
        senderRouterIndex = atoi(tok);
        rs_routerid = ntohs(routerID[senderRouterIndex]);

        printf("[Received routing tables from router with routerid:%d]\n",ntohs(routerID[senderRouterIndex]));
        while(tok!=NULL)
        {
            tok = strtok(NULL," ");
            if(tok == NULL)
                break;
            r_routerid = atoi(tok);

            tok = strtok(NULL," ");
            index = atoi(tok);
        
            tok = strtok(NULL," ");
            r_nextHop = atoi(tok);
        
            tok = strtok(NULL," ");
            r_cost = atoi(tok);

            //     printf("[from:%d to %d cost is :%d]\n",ntohs(routerID[senderRouterIndex]),r_routerid,r_cost);
            // applying distance vector algo :P
            // these are connected and now applying dv

            if(ntohs(cost[index]) > ntohs(cost[senderRouterIndex]) + r_cost)
            {
            //      printf("[Found a short path]\n");
            //    printf("[For:%d to:%d via %d]\n", ntohs(routerID[currRouterIndex]), r_routerid,rs_routerid);
            //     printf("[old cost:%d  new cost: %d + %d]\n",ntohs(cost[index]),ntohs(cost[rs_routeridIndex]),r_cost);
                // printf("cost to reach %d > cost to reach %d + cost to reach %d\n", ntohs(cost[index]), ntohs(cost[rs_routeridIndex]),r_cost);
                cost[index] = cost[senderRouterIndex] + ntohs(r_cost);
                nextHop[index] = routerID[senderRouterIndex];

            }
            else if(ntohs(cost[index]) == ntohs(cost[senderRouterIndex]) + r_cost)
            {
                int nextHopIndex_now =-1;
                for(int i=0;i<nRouters;i++)
                {
                    if(nextHop[index] == routerID[i])
                        nextHopIndex_now = i;
                }

                if(nextHopIndex_now != -1 && cost[nextHopIndex_now] > cost[senderRouterIndex])
                    nextHop[index] = routerID[senderRouterIndex];
            }

        // printf("to router id:%d nexthop:%d cost:%d \n", r_routerid,rs_routerid,r_cost);
        }
    }
    return senderRouterIndex;
}


