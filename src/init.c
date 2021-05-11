#include <string.h>
#include "../include/global.h"
#include "../include/control_header.h"
#include "../include/network_util.h"
#include "../include/router_info.h"
#include <arpa/inet.h>
#include "../include/connection_manager.h"
#include "../include/globals.h"
#include "../include/control_handler.h"


void init_response(int sock_index, char *cntrl_payload)
{
    memcpy(&nRouters, cntrl_payload, sizeof(nRouters));
    memcpy(&updateTime, cntrl_payload+2, sizeof(updateTime));
    nRouters = ntohs(nRouters);
    updateTime = ntohs(updateTime);
    timeout.tv_sec = updateTime;

    memset(routerID, 0,sizeof(uint16_t)*nRouters);
    memset(routerPort, 0,sizeof(uint16_t)*nRouters);
    memset(dataPort, 0,sizeof(uint16_t)*nRouters);
    memset(cost, 0,sizeof(uint16_t)*nRouters);
    memset(nextHop, 0,sizeof(uint16_t)*nRouters);
    memset(destIp, 0,sizeof(uint32_t)*nRouters);

    memset(rSocket, 0, 5 * sizeof(int));
    memset(timeoutTime, 0, 5 * sizeof(int));
    memset(activeTimer, 0, 5 * sizeof(int));
    memset(missCount, 0, 5 * sizeof(int));
    memset(timerInitialized, 0, 5 * sizeof(int));
    memset(isNeighbor, 0, 5 * sizeof(int));

    for(int i =0;i<5;i++)
    {
       rSocket[i] = -1;   
       dSocket[i] = -1;                //int
       timeoutTime[i] = 65535;  
       activeTimer[i] = 0;
       missCount[i] =-1;
       timerInitialized[i] =0;
       datafd[i] =-1;
    }

    int MEMORY_OFFSET =4;
    for(int i =0;i<nRouters;i++)
    {
        memcpy(&routerID[i], cntrl_payload + MEMORY_OFFSET, sizeof(routerID[i]));
        memcpy(&nextHop[i], cntrl_payload + MEMORY_OFFSET, sizeof(routerID[i]));
        MEMORY_OFFSET+=2;
        memcpy(&routerPort[i], cntrl_payload + MEMORY_OFFSET, sizeof(routerPort[i]));
        MEMORY_OFFSET+=2;
        memcpy(&dataPort[i], cntrl_payload + MEMORY_OFFSET, sizeof(dataPort[i]));
        MEMORY_OFFSET+=2;
        memcpy(&cost[i], cntrl_payload + MEMORY_OFFSET, sizeof(cost[i]));
        MEMORY_OFFSET+=2;
        memcpy(&destIp[i], cntrl_payload + MEMORY_OFFSET, sizeof(destIp));
        MEMORY_OFFSET+=4;
        


        if(cost[i]==0)
        {
            currRouterIndex = i;
            timeoutTime[i] = updateTime;
            isNeighbor[i] = 0;
            activeTimer[i] =1;
        }
        else if(cost[i] == 65535)
            isNeighbor[i] = 0;
        else
            isNeighbor[i] = 1;

    }

    DATA_PORT = dataPort[currRouterIndex];
    data_socket = create_data_sock();
    printf("DATA SOCKET :%d   CONTROL SOCKET : %d\n",data_socket,control_socket);
    FD_SET(data_socket, &master_list);
    if(head_fd < data_socket)
       head_fd = data_socket;

    create_client_data_sockets();
    create_router_sockets();
    create_client_router_sockets();  // creates error maybe?



    for(int i =0; i<nRouters; i++)
    {
        for(int j =0; j< nRouters; j++)
        {
            if(i == currRouterIndex)
                rtable[currRouterIndex][j] = cost[j];
            else
                rtable[i][j] = 65535; 
        }
        
    }
    
    printTable();


    printf("INFO for all the routers \n");
    printf("Number of routers : %d updateTime : %d \n",nRouters,updateTime);

    for(int i=0;i <nRouters;i++)
    // printf("routerID:%d routerPort:%d dataPort:%d cost:%d  \n",ntohs(routerID[i]),ntohs(routerPort[i]),ntohs(dataPort[i]),ntohs(cost[i]));
     	printf("routerID:%d routerPort:%d dataPort:%d cost:%d  \n",routerID[i],ntohs(routerPort[i]),ntohs(dataPort[i]),cost[i]);

        // sending blank response
	uint16_t payload_len;
	char *cntrl_response_header;
	payload_len = 0;
	cntrl_response_header = create_response_header(sock_index, 1, 0, payload_len);
	sendALL(sock_index, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
	free(cntrl_response_header);
	
}
void printTable()
{
    
    for(int i =0; i<nRouters; i++)
    {
    
        for(int j =0; j< nRouters; j++)
        {
            printf("%5d|",ntohs(rtable[i][j]));
        }
    printf("\n");    
    }
    
}