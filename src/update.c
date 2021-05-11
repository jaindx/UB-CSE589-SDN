
#include <string.h>
#include "../include/global.h"
#include "../include/control_header.h"
#include "../include/network_util.h"
#include "../include/router_info.h"
#include "../include/globals.h"

#include <arpa/inet.h>
void router_update(int sock_index,char *cntrl_payload)
{

   	memcpy(&updateID, cntrl_payload, sizeof(updateID));
    memcpy(&updateCost, cntrl_payload+2, sizeof(updateCost));


    for(int i =0; i< nRouters; i++)
    {
    	if(updateID == routerID[i])
    	{
    		printf("changing cost\n");
    		cost[i] = updateCost;
    	}
    }
        


	uint16_t payload_len;
	char *cntrl_response_header;
	payload_len = 0;
	cntrl_response_header = create_response_header(sock_index, 3, 0, payload_len);
	sendALL(sock_index, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
	free(cntrl_response_header);


}