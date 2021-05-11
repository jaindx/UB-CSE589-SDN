#ifndef ROUTER_INFO_H_
#define ROUTER_INFO_H_
#include <netinet/in.h>
bool isRouterInitialized;
struct timeval timeout;



/*	
*	Init will initialize all the routers.
*	nRouters will contain the number of routers 
*	updateTime will contain the time before the response should arrive
*	currRouterIndex - tells the index of the current router	
*	
*	For each router i we store the following information
*	routerID[i] 	: id of the ith router in Network Format
*	routerPort[i]   : router port of the ith router in Network Format
*	dataPort[i]	: data port of the ith router in Network Format
*	cost[i]  	: cost to reach the ith router in Network Format
*	nextHop[i]	: to reach the ith router we should go to this router first in Network Format
*	destIp[i]	: the ip address of the ith router in Network Format
*	isNeighbor[i]   : 1 means the ith router is a neighbor of our router, 0 means its not
*	  rSocket[i]	: Contains router socket for all neighbor routers, is -1 for routers which are not neighbors
	  serveraddr[i] : Contains serveraddr(ip+port) for all the neighbors
*/


uint16_t *routerID,*routerPort, *dataPort, *cost, *nextHop, nRouters, updateTime, updateID, updateCost;
uint32_t *destIp;

int rSocket[5],isNeighbor[5];

int currRouterIndex;
struct sockaddr_in serveraddr[5];


/*	Timer implementaion
*	timeoutTime[i] 	:will contain the relative time for the ith router  (have not consider the reset case on interrupt)
*	activeTimer 	:will be set to 1 for the router for which we are running the timer and 0 for the rest
*	missCount[i] 	:will be 0 if router has received the periodic updates,1 if one miss,2 if 2 miss and 3 if 3 miss (-1 default)
*	setActiveTimer(): will set the activeTimer to 1 for the index passed and rest all to zero
*/	

int timeoutTime[5];
int activeTimer[5];
int missCount[5];
void setActiveTimer(int index);



void create_router_sockets();
void create_client_router_sockets();


/*	router_recv_hook() : will receive the routing updates from the neighboring routers
*	The format of the data it receieves is as follows
*	%d [%d %d %d %d]->repeats.
*	The first %d is the index of the router sendings its routing table
*	The next 4 which repeat(depending on the number of routers it connects to is as follows(All the info is in host format):
*	%d - router id of the router it connects to  
*	%d - index of the router it connects to
*	%d - next hop for the router
*	%d - cost of the router to reach its connecting router	
*/

int router_recv_hook(int sock_index);    //returns the index of the router which had sent the table

void timer();
int timerInitialized[5];

uint16_t rtable[5][5];
void printTable();

#endif	
