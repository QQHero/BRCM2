/* @author: Tencent slimehsiao */
#ifndef _MPUDPCLIENT_H_
#define _MPUDPCLIENT_H_


int udp_sock_create(char* ip_addr, int port, struct sockaddr_in* addr);
int udp_send(int fd, struct sockaddr_in *addr, char* message, int len);

#endif
