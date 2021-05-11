//
// Created by Jain on 5/8/2021.
//

#ifndef JAINZACH_CONTROL_HEADER_H
#define JAINZACH_CONTROL_HEADER_H

#define CNTRL_HEADER_SIZE 8
#define DATA_HEADER_SIZE 8

#define CNTRL_RESP_HEADER_SIZE 8

#define PACKET_USING_STRUCT

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


struct __attribute__((__packed__)) PEER_INFO
{
    uint16_t router_id;
    uint16_t port1;
    uint16_t port2;
    uint16_t cost;
    uint32_t router_ip;
};
#endif

struct peer_info
{
    uint16_t router_id;
    uint16_t port1;
    uint16_t port2;
    uint16_t cost;
    uint32_t router_ip;
};


struct rtable_entry
{
    uint16_t router_id;
    uint16_t padding;
    uint16_t next_hop_id;
    uint16_t cost;
};

int is_near[5];

int inactive_peer_count [5];

struct peer_info g_peers[5];
int g_num_of_routers;
int g_interval;
int send_time;
int my_router_id;
uint32_t my_router_ip;
uint32_t my_router_port;
uint32_t my_data_port;
struct rtable_entry g_routing_table[5];
struct timeval tv;
char* create_response_header(int sock_index, uint8_t control_code, uint8_t response_code, uint16_t payload_len);

#endif //JAINZACH_CONTROL_HEADER_H
