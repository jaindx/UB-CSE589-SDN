//
// Created by Jain on 5/8/2021.
//

#include <string.h>
#include <netinet/in.h>

#include "../include/global.h"
#include "../include/control_header.h"

#ifndef PACKET_USING_STRUCT
#define CNTRL_RESP_CONTROL_CODE_OFFSET 0x04
#define CNTRL_RESP_RESPONSE_CODE_OFFSET 0x05
#define CNTRL_RESP_PAYLOAD_LEN_OFFSET 0x06
#endif

char* create_response_header(int sock_index, uint8_t control_code, uint8_t response_code, uint16_t payload_len)
{
    char *buffer;
#ifdef PACKET_USING_STRUCT
        BUILD_BUG_ON(sizeof(struct CONTROL_RESPONSE_HEADER) != CNTRL_RESP_HEADER_SIZE);

        struct CONTROL_RESPONSE_HEADER *cntrl_resp_header;
#endif
#ifndef PACKET_USING_STRUCT
    char *cntrl_resp_header;
#endif

    struct sockaddr_in addr;
    socklen_t addr_size;

    buffer = (char *) malloc(sizeof(char)*CNTRL_RESP_HEADER_SIZE);
#ifdef PACKET_USING_STRUCT
    cntrl_resp_header = (struct CONTROL_RESPONSE_HEADER *) buffer;
#endif
#ifndef PACKET_USING_STRUCT
    cntrl_resp_header = buffer;
#endif

    addr_size = sizeof(struct sockaddr_in);
    getpeername(sock_index, (struct sockaddr *)&addr, &addr_size);

#ifdef PACKET_USING_STRUCT
        memcpy(&(cntrl_resp_header->controller_ip_addr), &(addr.sin_addr), sizeof(struct in_addr));
        cntrl_resp_header->control_code = control_code;
        cntrl_resp_header->response_code = response_code;
        cntrl_resp_header->payload_len = htons(payload_len);
#endif

#ifndef PACKET_USING_STRUCT
    memcpy(cntrl_resp_header, &(addr.sin_addr), sizeof(struct in_addr));
    memcpy(cntrl_resp_header+CNTRL_RESP_CONTROL_CODE_OFFSET, &control_code, sizeof(control_code));
    memcpy(cntrl_resp_header+CNTRL_RESP_RESPONSE_CODE_OFFSET, &response_code, sizeof(response_code));
    payload_len = htons(payload_len);
    memcpy(cntrl_resp_header+CNTRL_RESP_PAYLOAD_LEN_OFFSET, &payload_len, sizeof(payload_len));
#endif

    return buffer;
}