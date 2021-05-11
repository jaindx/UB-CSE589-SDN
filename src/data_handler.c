
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/queue.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include "../include/global.h"
#include "../include/network_util.h"
#include "../include/control_header.h"
#include "../include/author.h"
#include "../include/globals.h"
#include "../include/control_responses.h"
#include "../include/router_info.h"
#include "../include/connection_manager.h"
#include <unistd.h>
void create_new_socket(int index, char* data_packet);
int temp_sock =-1;
int found =0;
struct sockaddr_in saddrr;
/* Linked List for active control connections */
struct DataConn
{
    int sockfd;
    LIST_ENTRY(DataConn) next;
}*connection_data, *conn_temp;
LIST_HEAD(ControlConnsHead, DataConn) data_conn_list;

int create_data_sock()
{
    int sock;
    struct sockaddr_in data_addr;
    socklen_t addrlen = sizeof(data_addr);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
        ERROR("socket() failed");

    /* Make socket re-usable */
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int)) < 0)
        ERROR("setsockopt() failed");

    bzero(&data_addr, sizeof(data_addr));

    data_addr.sin_family = AF_INET;
    data_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    data_addr.sin_port = DATA_PORT;

    if(bind(sock, (struct sockaddr *)&data_addr, sizeof(data_addr)) < 0)
        ERROR("bind() failed");

    if(listen(sock, 10500) < 0)
        ERROR("listen() failed");

    LIST_INIT(&data_conn_list);

    return sock;
}

int new_data_conn(int sock_index)
{
    int fdaccept, caddr_len;
    struct sockaddr_in remote_data_addr;

    caddr_len = sizeof(remote_data_addr);
    fdaccept = accept(sock_index, (struct sockaddr *)&remote_data_addr, &caddr_len);
    if(fdaccept < 0)
        ERROR("accept() failed");

    /* Insert into list of active control connections */
    connection_data = malloc(sizeof(struct DataConn));
    connection_data->sockfd = fdaccept;
    LIST_INSERT_HEAD(&data_conn_list, connection_data, next);

    return fdaccept;
}

void remove_Data_conn(int sock_index)
{
    LIST_FOREACH(connection_data, &data_conn_list, next) {
        if(connection_data->sockfd == sock_index) LIST_REMOVE(connection_data, next); // this may be unsafe?
        free(connection_data);
    }

    close(sock_index);
}

bool isData(int sock_index)
{
    LIST_FOREACH(connection_data, &data_conn_list, next)
        if(connection_data->sockfd == sock_index) return TRUE;

    return FALSE;
}

bool data_recv_hook(int sock_index)
{
    char *data_packet;
    uint32_t destip;
    uint8_t transferid;
    uint8_t ttl;
    uint16_t seq_no;
    uint32_t padding;
    char *fileReceived;
    fp = fopen("file-1", "wb");
    char filename1[7];


    while(1)
    {
    /* Get control header */
    data_packet = (char *) malloc(sizeof(char)*1036);
    bzero(data_packet, 1036);

    fileReceived = (char *) malloc(sizeof(char)*1024);
    memset(fileReceived,'\0', 1024);
    int bytesread;
    bytesread = recvALL(sock_index, data_packet, 1036);
    if( bytesread< 0){
        remove_Data_conn(sock_index);
        free(data_packet);
        return FALSE;
    }
    printf("** DATA Header : **\n");
    printf("bytesread: %d\n",bytesread);
    memcpy(&destip, data_packet, sizeof(destip));
    memcpy(&transferid, data_packet+4, sizeof(transferid));
    memcpy(&ttl, data_packet+5, sizeof(ttl));
    memcpy(&seq_no, data_packet+6, sizeof(seq_no));
    memcpy(&padding, data_packet+8,sizeof(padding));
    memcpy(fileReceived, data_packet+12, 1024);
    printf("%s\n",fileReceived);
    printf("IP : %d(network format)  transferid: %d  ttl : %d  seq_no:%d\n",destip,transferid,ttl,seq_no);
    printf("Ntohs ttl:%d transferid %d seqno:%d padding:%d\n",ntohs(transferid),ntohs(ttl),ntohs(seq_no),ntohs(padding));
    printf("Padding : %d\n",padding);
    ttl--;
   
    if(destip == destIp[currRouterIndex])
    {
        printf("found\n");
        sprintf(filename1,"file-%d",transferid);
        fwrite(fileReceived,1, 1024,fp);    
       if(found == 0)
	{	
		char *cntrl_response_header;
            cntrl_response_header = create_response_header(sock_index, 5, 0, 0);
            sendALL(sock_index, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
            free(cntrl_response_header);
		found =1;
	}	
	 if(padding!=0)
        {
            fclose(fp);
            int ret = rename("file-1", filename1);
    
            if(ret == 0) {
                 printf("File renamed successfully");
            } else {
                 printf("Error: unable to rename the file");
            }

   
            break;
        }
    }
    else if(ttl ==0)
    {
        printf("ttl over\n");
        break;
    }
    else
    {

        printf("will send again\n");
        memcpy(data_packet+5,&ttl,1);
        printf("new ttl has been set\n");
        for(int i =0;i<nRouters;i++)
        {
            if(destip = destIp[i])
            { 
                for(int j =0;j<nRouters;j++)
                {
                    if(nextHop[i] == routerID[j])
                        create_new_socket(j,data_packet);
                }   
            }   
        }
    }

}
}

void create_new_socket(int index, char* data_packet)
{
    {
        socklen_t addrlen = sizeof(saddrr);
        temp_sock = socket(AF_INET, SOCK_STREAM, 0);
        if(temp_sock < 0)
            ERROR("socket() failed");
        if(setsockopt(temp_sock, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int)) < 0)
                    ERROR("setsockopt() failed");
        //  bzero(&serveraddr[i], sizeof(serveraddr));
        saddrr.sin_family = AF_INET;
        saddrr.sin_port = dataPort[index];              
        saddrr.sin_addr.s_addr = destIp[index];
    }
    connect(temp_sock, (struct sockaddr *)&saddrr, sizeof(saddrr));
    printf("Number of bytes sent: %lu\n",write(temp_sock, data_packet, 1036));
        
}

void create_client_data_sockets()
{
    for(int i =0; i < nRouters; i++)
    {
        
            int port = ntohs(dataPort[i]);
            dSocket[i];
            socklen_t addrlen = sizeof(dataServeraddr[i]);
            dSocket[i] = socket(AF_INET, SOCK_STREAM, 0);
            if(dSocket[i] < 0)
                ERROR("socket() failed");
            if(setsockopt(dSocket[i], SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int)) < 0)
                ERROR("setsockopt() failed");
            dataServeraddr[i].sin_family = AF_INET;
            dataServeraddr[i].sin_port = dataPort[i];              
            dataServeraddr[i].sin_addr.s_addr = destIp[i];
        
    }
}
