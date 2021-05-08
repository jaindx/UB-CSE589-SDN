//
// Created by Jain on 5/8/2021.
//

#ifndef JAINZACH_CONNECTION_MANAGER_H
#define JAINZACH_CONNECTION_MANAGER_H

char *last_pkt;
char *one_last_pkt;

int control_socket, router_socket, data_socket;
int data_control_socket;
void init();

#endif //JAINZACH_CONNECTION_MANAGER_H
