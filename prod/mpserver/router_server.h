//
// Created by penciljiang on 2022/1/10.
//

#ifndef START_ROUTER_ACCGW_ROUTER_SERVER_H
#define START_ROUTER_ACCGW_ROUTER_SERVER_H

/*
 * 启动路由器server监听，针对tcp端口
 */
int startRouterListen(int listenPort);

// we should use this struct to cache session state
struct SocketState {
  int id;            // Socket handle
  int recv;            // Receiving
  int send;            // Sending
  char clientIp[16]; // now only support ipv4 ip
  int clientPort;  // client port
  int recvLen;
  char recvBuffer[8000];
  int sendLen;
  char sendBuffer[8000];
};
struct SocketState sockets[20];
int socketsCount;



int handle_json(int index, char *objReceived);

void removeSocket(int index);
int addSocket(int id, int what, char* ip, int port);

#endif  // START_ROUTER_ACCGW_ROUTER_SERVER_H
