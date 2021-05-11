#ifndef CONTROL_RESPONSE_H_
#define CONTROL_RESPONSE_H_

void init_response(int sock_index, char *cntrl_payload);

void router_update(int sock_index, char *cntrl_payload);

void routing_table_response(int sock_index);

void send_routing_table_to_all();

void send_routing_update_to_all(int index, int cost);


#endif
