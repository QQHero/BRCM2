#include "mp_server.h"
#include "mp_udpclient.h"
#include "mp_util.h"
#include <stdio.h>
#include<sys/socket.h>
#include<netinet/in.h>

#include "router_server.h"

#define PORT_NUM_BASE 20000
#define MOD_BASE 100


int udp_sock_create(char* ip_addr, int port, struct sockaddr_in *addr)
{
        int fd = 0;
        struct sockaddr_in cli_addr;
        socklen_t addrlen = sizeof(struct sockaddr_in);


        printf("udp_sock_create， ip_addr:%s, port:%d\n",ip_addr, port);
       
        if ( (fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        {
                debug_print("socket create error!\n");
				return -1;
        }

        memset((char *) addr, 0, sizeof(addr));
        addr->sin_family = AF_INET;
        addr->sin_port = htons(port);

        static int counter = 0;
        
        cli_addr.sin_family = AF_INET;
        cli_addr.sin_port = htons( PORT_NUM_BASE + (counter % MOD_BASE));  //assign a port number which is not used
        cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        counter++;

        //inet_pton(AF_INET, ip_addr, &cli_addr.sin_addr);
        

        int err;
        err = bind(fd,(struct sockaddr*)&cli_addr,addrlen);
        if (err<0)
        {
                perror("socket bind error");
                return -1;
        }

        if (inet_aton(ip_addr , &(addr->sin_addr)) == 0) 
        {
                debug_print("inet_aton() failed\n");
                return -1;
        }

        //debug_print("UDP socket FD created:%d\n",fd);

	return fd;

    
}

int udp_send(int fd, struct sockaddr_in *addr, char* message, int len)
{
	if (sendto(fd, message, len , 0 , addr, sizeof(struct sockaddr_in))==-1)
	{
		debug_print("UDP send error\n");
		return -1;
	}  
	return 0;
}


