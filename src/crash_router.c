#include <string.h>
#include "../include/global.h"
#include "../include/control_header.h"
#include "../include/network_util.h"
#include "../include/router_info.h"
#include "../include/globals.h"

#include <arpa/inet.h>

void crashRouter(int sock_index)
{

	uint16_t payload_len;
	char *cntrl_response_header;
	payload_len = 0;
	cntrl_response_header = create_response_header(sock_index, 4, 0, payload_len);
	sendALL(sock_index, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
	free(cntrl_response_header);

	router_crashed =1;

	exit(1);


}