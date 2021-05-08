//
// Created by Jain on 5/8/2021.
//

#ifndef JAINZACH_CONTROL_HANDLER_H
#define JAINZACH_CONTROL_HANDLER_H

int create_control_sock();
int new_control_conn(int sock_index);
bool isControl(int sock_index);
bool control_recv_hook(int sock_index);

#endif //JAINZACH_CONTROL_HANDLER_H
