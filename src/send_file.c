#include <string.h>

#include "../include/global.h"
#include "../include/globals.h"
#include "../include/router_info.h"

#include "../include/control_header.h"
#include "../include/network_util.h"
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <stdio.h>



void sendFile(int sock_index,char *cntrl_payload, int total_payload_length)
{

    uint32_t destIP;
    uint8_t ttl, transferID;
    uint16_t seq_no;
    printf("payload :%d  \n",total_payload_length);
    int length = (total_payload_length - 8);
    printf("file length %d\n",length);
    char *filename = (char*)malloc(sizeof(char)*length);
    memset(filename,'\0',length);

    memcpy(&destIP, cntrl_payload, sizeof(destIP));
    memcpy(&ttl, cntrl_payload+4, sizeof(ttl));
    memcpy(&transferID, cntrl_payload+5, sizeof(transferID));
    memcpy(&seq_no, cntrl_payload+6, sizeof(seq_no));
    memcpy(filename,cntrl_payload+8,length);

    printf("IP : %d(network format)  ttl : %d  transferID : %d  seq_no:%d\n",destIP,ttl,transferID,seq_no);
    printf("name of the file is %s@@\n",filename);
    filename[length] = '\0';
    FILE *fp = fopen(filename,"rb");
    if(fp==NULL)
    {
        ERROR("File open error");
        
    }

    fseek(fp, 0L, SEEK_END);
	int	sz = ftell(fp);
	printf("size: %d\n",sz);
	rewind(fp);
    int forwardToIndex;
	int numberofpackets = sz/1024;
	printf("Number of packets : %d\n",numberofpackets);
   	
   	// this select data socket to send to
    for(int i =0;i<nRouters;i++)
    {
    	 
        printf("IP matching destIP(sendto) :%d  destIp(stored):%d\n",destIP,destIp[i]);
        if(destIP == destIp[i])
        { 
            for(int j =0;j<nRouters;j++)
            {
                if(nextHop[i] == routerID[j])
                {
                    printf("Connecting.. %d %d %d\n ",dSocket[j],dataServeraddr[j].sin_port,dataServeraddr[j].sin_addr.s_addr);
                    if(connect(dSocket[j], (struct sockaddr *)&dataServeraddr[j], sizeof(dataServeraddr[j]))<0)
                        ERROR("connect failed\n");
                }
            } 
    	}	
    }

    for(int i =0;i<numberofpackets;i++)
    {
    	char buff[1024];
    	memset(buff,'\0',1024);
        int nread = fread(buff,1,1024,fp);

        if(nread > 0)
        {
            printf("XXXXXXX%dXXXXXXXX",seq_no);
           	if(i ==numberofpackets-1)
           	{
        		create_send_packet(destIP,ttl,transferID,seq_no,buff,1);
        	}
        	else
        	{
        		create_send_packet(destIP,ttl,transferID,seq_no,buff,0);
        	}

        }
       /* if (nread < 1024)
        {
            if (feof(fp))
			{
                printf("End of file\n");
			}
        	if (ferror(fp))
            	printf("Error reading\n");
            break;
        }*/
        seq_no++;	
    }  
    fclose(fp);
    char *cntrl_response_header;
    cntrl_response_header = create_response_header(sock_index, 5, 0, 0);
    sendALL(sock_index, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
    free(cntrl_response_header);
}

void create_send_packet(uint32_t destIP, uint8_t ttl, uint8_t transferID,uint16_t seq_no, char *buff,int lastmsg)
{
	char *sendfile_header;
	sendfile_header = (char*)malloc(1036); // 1024(data) + 12(header)
	memset(sendfile_header,'\0',1036);
	uint32_t payload =0;
// int x = 1<<((sizeof(int)*8)-1);
	memcpy(sendfile_header,&destIP,sizeof(destIP));
	memcpy(sendfile_header + 4,&transferID,sizeof(transferID));
	memcpy(sendfile_header+5,&ttl,sizeof(ttl));
	memcpy(sendfile_header+6,&seq_no,sizeof(seq_no));
	//memcpy(sendfile_header+8,0,4);   //change later  code for padding as of now
	
	if(lastmsg ==0)
		memset(sendfile_header + 8,payload,4);
	else
	{
		memset(sendfile_header+8,1,4);
		payload = 0 | (1UL << 31);
	}
	memcpy(sendfile_header+12,buff, 1024);

	
	for(int i =0;i<nRouters;i++)
    {
    	if(destIP == destIp[i])
    	{ 
            for(int j =0;j<nRouters;j++)
            {
                if(nextHop[i] == routerID[j])
                    printf("Number of bytes sent: %lu\n",sendALL(dSocket[j], sendfile_header, 1036));

            } 
    		
    	}	
    }
	free(sendfile_header);

}