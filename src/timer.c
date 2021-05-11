#include <sys/select.h>

#include "../include/connection_manager.h"
#include "../include/global.h"
#include "../include/control_handler.h"
#include "../include/globals.h"
#include "../include/router_info.h"
#include "../include/control_responses.h"


void timer()
{

	int activeIndex = -1;  // put all this code in functions
    for(int i =0;i<nRouters;i++)
    {
        if(activeTimer[i] == 1 && i ==currRouterIndex)
        {
                activeIndex = i;
        }
    }

    

    for(int i=0;i<nRouters;i++)
    {
        if(activeTimer[i]==1)
            timeoutTime[i] = updateTime;
        if(timeoutTime[i] == 0)
            timeoutTime[i] = updateTime;
    }

     /*for(int i =0;i<nRouters;i++)
         printf("[router id %d  time-out:%d]\n",ntohs(routerID[i]),timeoutTime[i]);
*/

    if(activeIndex == currRouterIndex)
    {
        send_routing_table_to_all(); // this will only be sent when main timer expires
    }

    //this part of code is to check if router has not received rtables for 3 timeouts
 //   printf("active index %d",activeIndex);
 //   printf("Miss count of active index : %d\n",missCount[activeIndex]);
    
    for(int i =0;i<nRouters;i++)
    {

        if(i != currRouterIndex && activeTimer[i] == 1)
        {
            if(missCount[i] ==0)
            {
                missCount[i] = -1;
            }
            else
            {
                if(missCount[i] == -1)
                    missCount[i] = 1;
                else
                    missCount[i]++;
            }
        }
    

        if(missCount[i]==3)
        {
            cost[i] = 65535;
            nextHop[i] = 65535;
            rtable[currRouterIndex][i] = 65535;
            isNeighbor[i]=-1;
/*            send_routing_update_to_all(i, 65535);
*/            printf("This router has gone down %d\n",ntohs(routerID[activeIndex]));

            for(int i =0; i<nRouters; i++)
            {
                for(int j =0; j < nRouters; j++ )
                {
                    if(rtable[currRouterIndex][i] > rtable[currRouterIndex][j] + rtable[j][i])
                    {
                        rtable[currRouterIndex][i] = rtable[currRouterIndex][j] + rtable[j][i];
                        nextHop[i] = routerID[j];
                    }
                }

            }   
    
        }

    

    }


    for(int i =0;i<nRouters;i++)
    {
/*        printf("routerId:%d misscount:%d\n",ntohs(routerID[i]),missCount[i]);
*/    }
    //	getting index of router with the smallest timeout value 
    int minTimer = 65535;
    int nextTimerIndex;
    for(int i =0;i<nRouters;i++)
    {
        if(timeoutTime[i] < minTimer)
        {
            minTimer = timeoutTime[i];
            //nextTimerIndex = i;
        }
    }
/*    printf("minimum timer is :%d\n",minTimer);
*/    // updating relative timeouts with respect to the new timer
    /*if(minTimer == updateTime)
    {
        timeout.tv_sec= updateTime;
        timeout.tv_usec = 0;
    }
    else*/
    timeout.tv_sec = minTimer;
    timeout.tv_usec = 0;
    
        for(int i=0;i<nRouters;i++)  //marking new timer as the active timer
        {
            /*if(i==nextTimerIndex)
                activeTimer[i]=1;
            else
                activeTimer[i]=0;*/
            if(timeoutTime[i] == minTimer)
                activeTimer[i]=1;
            else
                activeTimer[i]=0;
        }
        

        //updating relative timeouts
        for(int i =0;i<nRouters;i++)
        {
            /*if(i != nextTimerIndex && timeoutTime[i]!= 65535)*/
            if(timeoutTime[i]!= 65535)
                 timeoutTime[i] = timeoutTime[i] - minTimer;
        }

    
/*    printf("****After selecting new timer****\n");
    for(int i =0;i<nRouters;i++)
        printf("[router id %d  time-out:%d]\n",ntohs(routerID[i]),timeoutTime[i]);
*/

}