#include <string.h>
#include "../include/global.h"
#include "../include/control_header.h"
#include "../include/network_util.h"
#include "../include/router_info.h"
#include <arpa/inet.h>

//for header check control code and response code to send
// TO DO:Response code, can add a bool in init, if initialized then good to go, else if not initialized and asked to retrieve set error code
void routing_table_response(int sock_index) //remove tcp later
{
	uint16_t payload_len, response_len;
	char *cntrl_response_header, *cntrl_response_payload, *cntrl_response;

	payload_len = nRouters * 8; 
	cntrl_response_payload = (char *) malloc(payload_len);
	uint16_t *infinity = malloc(sizeof(uint16_t));
	memset(infinity,'f',sizeof(infinity));

	printf("payload_length");
	int RESPONSE_OFFSET =0;
	for(int i=0; i < nRouters; i++)
	{
		memcpy(cntrl_response_payload + RESPONSE_OFFSET,&routerID[i],sizeof(routerID[i]));
		RESPONSE_OFFSET+=2;
		memset(cntrl_response_payload + RESPONSE_OFFSET,0,sizeof(routerID[i]));
		RESPONSE_OFFSET+=2;
		if(cost[i]==65535)
			memcpy(cntrl_response_payload + RESPONSE_OFFSET, &cost[i],sizeof(nextHop[i]));
		else
			memcpy(cntrl_response_payload + RESPONSE_OFFSET, &nextHop[i],sizeof(nextHop[i]));
		RESPONSE_OFFSET+=2;
		memcpy(cntrl_response_payload + RESPONSE_OFFSET, &cost[i], sizeof(cost[i]));
		RESPONSE_OFFSET+=2;
	}

	printf("##ROUTING TABLE##\n");
	for(int i =0;i< nRouters;i++)
	{
	//	printf("N routerid :%d  nexthop:%d  cost:%d\n",routerID[i], nextHop[i],cost[i]);
		printf("[ %d -> %d -> %d = %d]\n",ntohs(routerID[currRouterIndex]),ntohs(nextHop[i]),ntohs(routerID[i]),ntohs(cost[i]));
		//printf("H routerid :%d  nexthop:%d  cost:%d\n",ntohs(routerID[i]), ntohs(nextHop[i]),ntohs(cost[i]));
		
	}



	cntrl_response_header = create_response_header(sock_index, 2, 0, payload_len);

	response_len = CNTRL_RESP_HEADER_SIZE+payload_len;
	cntrl_response = (char *) malloc(response_len);
	
	memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
	free(cntrl_response_header);
	
	memcpy(cntrl_response+CNTRL_RESP_HEADER_SIZE, cntrl_response_payload, payload_len);
	free(cntrl_response_payload);

	sendALL(sock_index, cntrl_response, response_len);
	
	free(cntrl_response);

}


void send_routing_table_to_all()
{
	char buffer[1000];
	memset(buffer,'\0',1000*sizeof(char));
	
	sprintf(buffer,"%d ",currRouterIndex);
	printf("[Sending routing tables to neighbors. Router id:%d]\n",ntohs(routerID[currRouterIndex]));
	// very first is the router for which the following details are present
	int OFFSET = strlen(buffer);
	
	for(int i =0; i<nRouters; i++)
	{
		// first isrouter id, second is index, second is nexthop, third is nexthop 4th is cost
			sprintf(buffer + OFFSET, "%d %d %d %d ", ntohs(routerID[i]),i,ntohs(nextHop[i]),ntohs(cost[i]));
			OFFSET = strlen(buffer); 
		
	}
	buffer[strlen(buffer)-1]='\0';
//	printf("[Data in buffer: %s]\n",buffer);

	for(int i =0;i<nRouters;i++)
	{
		//printf("routerid %d cost:%d]\n",ntohs(routerID[i]),ntohs(cost[i]));
		if(isNeighbor[i] == 1)
		{
		//	printf("[Sending Data to its neightbor router with id:%d]\n",ntohs(routerID[i]));
			struct sockaddr_in addr = serveraddr[i];
			addr.sin_port = routerPort[i];
			//printf("[router:%d][port:%d] [ip:%s] [cost:%d]\n ",ntohs(routerID[i]),ntohs(addr.sin_port),inet_ntoa(addr.sin_addr),ntohs(cost[i]));
			if (sendto(rSocket[i], buffer, strlen(buffer), 0, (struct sockaddr *)&addr, sizeof(addr)) < 0 )
            {
               	ERROR("send failed");
            }
            
         //   	printf("send success\n");
		}
	}
}

void send_routing_update_to_all(int index, int cost)
{
	char buffer[1000];
	memset(buffer,'\0',1000*sizeof(char));
	
	sprintf(buffer,"c ");
	printf("[Sending routing update to neighbors.]\n");
	int OFFSET = strlen(buffer);
	
	for(int i =0; i<nRouters; i++)
	{
			sprintf(buffer + OFFSET, "%d %d ", index,cost);
			OFFSET = strlen(buffer); 
	}
	buffer[strlen(buffer)-1]='\0';
	printf("[Data in buffer: %s]\n",buffer);

	for(int i =0;i<nRouters;i++)
	{
		if(isNeighbor[i] == 1)
		{
			struct sockaddr_in addr = serveraddr[i];
			addr.sin_port = routerPort[i];
			sendto(rSocket[i], buffer, strlen(buffer), 0, (struct sockaddr *)&addr, sizeof(addr));
		}
	}
}



		/*if(ntohs(cost[i]) != 65535 && ntohs(cost[i]) != 0)
		{
			printf("ACCESSIBLE  [router:%d] [cost:%d]\n",ntohs(routerID[i]),ntohs(cost[i]));
			printf("[IP:%d][Port:%d]\n",ntohs(destIp[i]), ntohs(routerPort[i]));
			
			unsigned long ipp = destIp[i];
			char ip[INET_ADDRSTRLEN];
       		inet_ntop(AF_INET,(struct in_addr *)&ipp,ip,INET_ADDRSTRLEN);
       		printf("IP:%s   ipp:%d \n",ip,destIp[i]);
		}
		else
		{
			printf("NOT ACCESSIBLE [router:%d] [cost:%d]\n",ntohs(routerID[i]),ntohs(cost[i]));
		}*/
