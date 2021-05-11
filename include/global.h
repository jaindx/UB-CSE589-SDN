//
// Created by Jain on 5/8/2021.
//

#ifndef JAINZACH_GLOBAL_H
#define JAINZACH_GLOBAL_H

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>


struct timeval t_timer;

typedef enum {FALSE, TRUE} bool;

#define ERROR(err_msg) {perror(err_msg); exit(EXIT_FAILURE);}

#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))

uint16_t CONTROL_PORT;
uint16_t ROUTER_PORT;
uint16_t DATA_PORT;

#endif //JAINZACH_GLOBAL_H
