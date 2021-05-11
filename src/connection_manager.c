//
// Created by Jain on 5/8/2021.
//

#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/queue.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include "../include/connection_manager.h"
#include "../include/global.h"
#include "../include/control_handler.h"
#include "../include/globals.h"
#include "../include/router_info.h"
#include "../include/control_responses.h"

void main_loop()
{
    isRouterInitialized = FALSE;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    int selret, sock_index, fdaccept;
    while(TRUE){
        watch_list = master_list;
        selret = select(head_fd+1, &watch_list, NULL, NULL, &timeout);
        if(selret < 0)
            ERROR("select failed.\n");
        printf("%lu\n",timeout.tv_sec);
        if(selret == 0 && isRouterInitialized)
        {
            timer();
           /* send_routing_table_to_all();
            timeout.tv_sec = updateTime;
*/

   /*         int activeIndex;  // put all this code in functions
            for(int i =0;i<nRouters;i++)
            {
                if(activeTimer[i] == 1)
                {
                    activeIndex = i;
                    break;
                }
            }
            printf("$$$Time out for router id %d$$$\n",ntohs(routerID[activeIndex]));
            printf("Timer value %lu \n",timeout.tv_sec);
            for(int i =0;i<nRouters;i++)
                printf("router id %d  time-out:%d\n",ntohs(routerID[i]),timeoutTime[i]);

            if(activeIndex = currRouterIndex)
                send_routing_table_to_all(); // this will only be sent when main timer expires


            if(missCount[activeIndex] !=0)
            {
                if(missCount[activeIndex] == -1)
                    missCount[activeIndex] = 1;
                else
                    missCount[activeIndex]++;   
            }
            else
                missCount[activeIndex]=-1;

            if(missCount[activeIndex]==3)
            {
                cost[activeIndex] = 65535;
                nextHop[activeIndex] = 65535;
                printf("This router be going down %d\n",ntohs(routerID[activeIndex]));
            }
            // start checking from this line onwards
            int minTimer = 65535;
            int nextTimerIndex;
            //get smallest timeouttime
            for(int i =0;i<nRouters;i++)
            {
                if(timeoutTime[i] < minTimer && i!=activeIndex)
                    {
                        minTimer = timeoutTime[i];
                        nextTimerIndex = i;
                    }
            }

            if(minTimer == 65535)
            {
                timeout.tv_sec= updateTime;
                timeout.tv_usec = 0;
            }
            else
            {
                for(int i=0;i<nRouters;i++)  //setActive timer
                {
                    if(i==nextTimerIndex)
                        activeTimer[i]=1;
                    else
                        activeTimer[i]=0;
                }
                timeout.tv_sec = minTimer;
                timeout.tv_usec = 0;

                for(int i =0;i<nRouters;i++)
                {
                    if(i == nextTimerIndex)
                        timeoutTime[i] =updateTime;
                    else
                    {
                        if(timeoutTime[i] !=  65535)
                        {
                            timeoutTime[i] = timeoutTime[i] - minTimer;
                        }
                    }


                }

            } */
        }
        /* Loop through file descriptors to check which ones are ready */
        for(sock_index=0; sock_index<=head_fd; sock_index+=1){

            if(FD_ISSET(sock_index, &watch_list))
            {

                /* control_socket */
                if(sock_index == control_socket)
                {
                    printf("Control socket\n");
                    fdaccept = new_control_conn(sock_index); // looks unnecessary?sending sock_index

                    /* Add to watched socket list */
                    FD_SET(fdaccept, &master_list);
                    if(fdaccept > head_fd) head_fd = fdaccept;

                }

                /* router_socket */
                else if(sock_index == router_socket)
                {
                    //call handler that will call recvfrom() .....
                    int routerIndex = router_recv_hook(sock_index);
                    if(timerInitialized[routerIndex] == 0)
                    {
                        timeoutTime[routerIndex] = updateTime-timeout.tv_sec;
                        timerInitialized[routerIndex] = 1;
                        printf("[Timer value will be :%d - %lu]\n",updateTime,timeout.tv_sec);

                    }
                    missCount[routerIndex] =0;   
                }
                /* data_socket */
                else if(sock_index == data_socket)
                {
                    fdaccept = new_data_conn(sock_index); // looks unnecessary?sending sock_index
                    for(int i =0;i<10;i++)
                    printf("reached data\n");
                    /* Add to watched socket list */
                    FD_SET(fdaccept, &master_list);
                    if(fdaccept > head_fd) head_fd = fdaccept;
                }

                /* Existing connection */
                else
                {
                    printf("CCsock : %d  control:%d router:%d \n",sock_index,control_socket,router_socket);
                    if(isControl(sock_index))  //VI checks if fd exists in the list or not
                    {
                        if(!control_recv_hook(sock_index)) 
                            FD_CLR(sock_index, &master_list);
                    }
                    else if (isData(sock_index))
                    {
                        if(!data_recv_hook(sock_index)) 
                            FD_CLR(sock_index, &master_list);
                    }
                    else ERROR("Unknown socket index\n");
                }
            }
        }
    }
}

void init()
{
    control_socket = create_control_sock();
    //router_socket and data_socket will be initialized after INIT from controller

    FD_ZERO(&master_list);
    FD_ZERO(&watch_list);

    /* Register the control socket */
    FD_SET(control_socket, &master_list);
    head_fd = control_socket;

    main_loop();
}