//
// Created by Jain on 5/8/2021.
//

#ifndef JAINZACH_NETWORK_UTIL_H
#define JAINZACH_NETWORK_UTIL_H

ssize_t recvALL(int sock_index, char *buffer, ssize_t nbytes);
ssize_t sendALL(int sock_index, char *buffer, ssize_t nbytes);

#endif //JAINZACH_NETWORK_UTIL_H
