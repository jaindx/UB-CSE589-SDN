//
// Created by Jain on 5/8/2021.
//

#ifndef JAINZACH_CONTROL_HEADER_H
#define JAINZACH_CONTROL_HEADER_H

#define CNTRL_HEADER_SIZE 8
#define DATA_HEADER_SIZE 8

#define CNTRL_RESP_HEADER_SIZE 8

#define PACKET_USING_STRUCT // Comment this out to use alternate packet crafting technique

#ifdef PACKET_USING_STRUCT
    struct __attribute__((__packed__)) CONTROL_HEADER
    {
        uint32_t dest_ip_addr;
        uint8_t control_code;
        uint8_t response_time;
        uint16_t payload_len;
    };

    struct __attribute__((__packed__)) CONTROL_RESPONSE_HEADER
    {
        uint32_t controller_ip_addr;
        uint8_t control_code;
        uint8_t response_code;
        uint16_t payload_len;
    };
#endif

char* create_response_header(int sock_index, uint8_t control_code, uint8_t response_code, uint16_t payload_len);

#endif //JAINZACH_CONTROL_HEADER_H
