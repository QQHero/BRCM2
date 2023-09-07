//
// Created by penciljiang on 2022/1/10.
//
#include "router_server.h"
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//#include <zconf.h>
#include <string.h>
#include "router_log.h"
#include "router_protocol.h"
#include "mp_core.h"
//#include "mp_aeb.h"


#define TENCENT_MEVENT_UDP_PORT 44000


const int kInvalidSocket = -1;

const int MAX_SOCKETS = 20;
const int EMPTY = 0;
const int LISTEN = 1;
const int RECEIVE = 2;
const int IDLE = 3;
const int SEND = 4;
const int LISTEN_NUM = 20;

//const int SELECT_TIMEOUT = 10000;
const int SELECT_TIMEOUT = 200000;

const int BUF_SIZE = 2000;
const int MAX_BUFF_SIZE = 8000;



// sockets equal to MAX_SOCKETS

socketsCount = 0;

// method
void closeSocketFd(int fd);
void acceptConnection(int index);
void sendMessage(int index);
void receiveMessage(int index);
void tcpEchoRunnable();

void parseRecvData(int index);

int handle_json(int index, char *objReceived);


/********* handle recv data *********/
int handleRecvData(const int index, const char *recvData, const int len);
// json data
int handleSendData(const int index, const char *sendData, const int len);

// event handler
void mevent_event_handler(int);
int init_event_socket();
int mevent_socket;

void closeSocketFd(int fd) 
{
    DEBUG_LOG_D("socket[%d] close, current fds:%d", fd, socketsCount);
    close(fd);
}

int addSocket(int id, int what, char* ip, int port) 
{
    int i = 0;

    for (; i < MAX_SOCKETS; i++) {
        if (sockets[i].recv == EMPTY) {
            sockets[i].id = id;
            sockets[i].recv = what;
            sockets[i].send = IDLE;
            sockets[i].recvLen = 0;
            sockets[i].sendLen = 0;
            strcpy(sockets[i].clientIp, ip);
            sockets[i].clientPort = port;
            socketsCount++;
            return (1);
        }
    }
    return (0);
}


void removeSocket(int index) {
  sockets[index].id = kInvalidSocket;
  sockets[index].recv = EMPTY;
  sockets[index].send = EMPTY;
  socketsCount--;
}

static inline int make_socket_nonblocking(int fd) 
{
    int flags;
  
    if ((flags = fcntl(fd, F_GETFL, NULL)) < 0) {
        return -1;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        return -1;
    }
    return 0;
}

//create event socket

/* get_ifname_unit() index is << 4 */
#define EAPD_WKSP_PORT_INDEX_SHIFT	4
#define EAPD_WKSP_SPORT_OFFSET		(1 << 5)
#define EAPD_WKSP_MPORT_OFFSET		(1 << 6)
#define EAPD_WKSP_VX_PORT_OFFSET	(1 << 7)

#define EAPD_WKSP_MEVENT_UDP_PORT	44000
#define EAPD_WKSP_MEVENT_UDP_RPORT 	EAPD_WKSP_MEVENT_UDP_PORT
#define EAPD_WKSP_MEVENT_UDP_SPORT 	EAPD_WKSP_MEVENT_UDP_PORT + EAPD_WKSP_SPORT_OFFSET

static int mevent_eapd_socket_init(void)
{

	int reuse = 1;
	struct sockaddr_in sockaddr;
	mevent_socket = -1;

	/* open loopback socket to communicate with EAPD */
	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	sockaddr.sin_port = htons(EAPD_WKSP_MEVENT_UDP_SPORT);

	if ((mevent_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		DEBUG_LOG_D("Unable to create loopback socket\n");
		return -1;
	}

	if (setsockopt(mevent_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse)) < 0) {
		DEBUG_LOG_D("Unable to setsockopt to loopback socket %d.\n", mevent_socket);
		goto exit1;
	}

	if (bind(mevent_socket, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0) {
		DEBUG_LOG_D("Unable to bind to loopback socket %d\n", mevent_socket);
		goto exit1;
	}

	DEBUG_LOG_I("opened loopback socket %d\n", mevent_socket);
	return mevent_socket;

	/* error handling */
exit1:
	close(mevent_socket);
	return -1;
}



int startRouterListen(int listenPort) 
{
    int listenSocket;
    int ret;
    struct sockaddr_in serverService;
    
    
    DEBUG_LOG_D("startRouterListen");

    mevent_socket = init_event_socket();

    // clear sockets..
    memset(sockets, 0, sizeof(sockets));
    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == kInvalidSocket) {
        return -1;
    }
    
    serverService.sin_family = AF_INET;
    serverService.sin_addr.s_addr = htonl(INADDR_ANY);
    serverService.sin_port = htons(listenPort);
    ret = bind(listenSocket, (const struct sockaddr *) &serverService, sizeof(serverService));
    if (ret < 0) {
        closeSocketFd(listenSocket);
        DEBUG_LOG_D("Fail to bind socket");
        return -1;
    }
    ret = listen(listenSocket, LISTEN_NUM);
    if (ret < 0) {
        closeSocketFd(listenSocket);
        DEBUG_LOG_D("Fail to bind socket");
        return -1;
    }
    addSocket(listenSocket, LISTEN, "255.255.255.255", listenPort);
    DEBUG_LOG_D("Succ to listen fd:%d.", listenSocket);
    //mevent_eapd_socket_init();
    

    tcpEchoRunnable();
    return 0;
}




void tcpEchoRunnable() 
{
    DEBUG_LOG_D("Start tcpEchoRunnable");
    // change to thread
    int i = 0;
    int maxFd = 0;

	struct timeval to; //timeout for select

    struct timeval te; //timestamp for get ap information
    gettimeofday(&te, NULL); // get current time
    static long long time_stamp_previous_ms;
    static long long time_stamp_current_ms;
    
    time_stamp_previous_ms = te.tv_sec*1000LL + te.tv_usec/1000; // calculate milliseconds
     
	while (1) {
        fd_set waitRecv;
        FD_ZERO(&waitRecv);
        for (i = 0; i < MAX_SOCKETS; i++) {
            if ((sockets[i].recv == LISTEN) || (sockets[i].recv == RECEIVE)) {
                FD_SET(sockets[i].id, &waitRecv);
                if (sockets[i].id > maxFd) {
                    maxFd = sockets[i].id;
                }
            }
        }

        /*
            put udp socket into slect fd_set
        */
        for (i=0;i<REPORT_LIST_SIZE;i++) {
            if (report_list[i].sock_udp_fd > 0) {
                FD_SET(report_list[i].sock_udp_fd, &waitRecv);
                if (report_list[i].sock_udp_fd > maxFd) {
                    maxFd = report_list[i].sock_udp_fd;
                }
            }
        }
    
        fd_set waitSend;
        FD_ZERO(&waitSend);
        for (i = 0; i < MAX_SOCKETS; i++) {
            if (sockets[i].send == SEND) {
                FD_SET(sockets[i].id, &waitSend);
                if (sockets[i].id > maxFd) {
                    maxFd = sockets[i].id;
                }
            }
        }

        //fd_set event_fd_set;
        //FD_ZERO(&event_fd_set);
        if (mevent_socket > 0) {
            FD_SET(mevent_socket, &waitRecv);
        }
        if (mevent_socket > maxFd) {
            maxFd = mevent_socket;
        } 


        gettimeofday(&to, NULL);
        time_stamp_current_ms = to.tv_sec*1000LL + to.tv_usec/1000;
        int wait_time_us=0;
        wait_time_us = SELECT_TIMEOUT - ((time_stamp_current_ms - time_stamp_previous_ms)*1000);
        if (wait_time_us < 0 ) {
            wait_time_us = 0;
        } else if (wait_time_us > 200000) {
            wait_time_us = 200000;
        }

   
        to.tv_sec = 0;
        to.tv_usec = wait_time_us;
        int nfd = 0;
        //DEBUG_LOG_D("Try select: maxFd(%d)", maxFd);
        // linux should select max fd + 1
        //DEBUG_LOG_I("select fd");
        nfd = select(maxFd + 1, &waitRecv, &waitSend, NULL, &to);

        /* Internal Timer timeout*/
        if (nfd == 0) {
            gettimeofday(&te, NULL); // get current time
            time_stamp_current_ms = te.tv_sec*1000LL + te.tv_usec/1000;
            if (time_stamp_current_ms - time_stamp_previous_ms >= 200) {
                gather_ap_info();
                time_stamp_previous_ms = time_stamp_current_ms;
            }
            continue;
        }

        if (nfd < 0) {
            DEBUG_LOG_D("select nfd < 0, error!");
            /*TODO: handle error*/
        }

        /*check if it's from udp RTT packet*/

        for (i=0;i<REPORT_LIST_SIZE && nfd > 0;i++) {
            if (report_list[i].sock_udp_fd > 0) {
                if (FD_ISSET(report_list[i].sock_udp_fd, &waitRecv)) {
                    handle_udp_echo(i);
                }
            }
        }

        /*check if it's event */
        if (FD_ISSET(mevent_socket, &waitRecv)) {
            DEBUG_LOG_I("Select EVENT socket");
            mevent_event_handler(mevent_socket);
        }

        for (i = 0; i < MAX_SOCKETS && nfd > 0; i++) {
            if (FD_ISSET(sockets[i].id, &waitRecv)) {
                nfd--;
                if (sockets[i].recv == LISTEN) {
                    DEBUG_LOG_D("Accept: i:(%d)", i);
                    acceptConnection(i);
                }
                if (sockets[i].recv == RECEIVE) {
                    DEBUG_LOG_D("Recv socket(%d): i:(%d)", sockets[i].id, i);
                    // do with receive Message, just parse header..
                    receiveMessage(i);
                }
            }
        }

        for (i = 0; i < MAX_SOCKETS && nfd > 0; i++) {
            if (FD_ISSET(sockets[i].id, &waitSend)) {
                nfd--;
                if (sockets[i].send == SEND) {
                    DEBUG_LOG_D("Send socket(%d): i:(%d)", sockets[i].id, i);
                    // cache send message and send..
                    sendMessage(i);
                }
            }
        }    
    }
    DEBUG_LOG_D("Stop tcpEchoRunnable");
    return;
}



void receiveMessage(int index) 
{
    int msgSocket = sockets[index].id;
    int len = sockets[index].recvLen;
    int spareLen = sizeof(sockets[index].recvBuffer) - len;
    int bytesRecv;

    if (spareLen <= 0) {
        DEBUG_LOG_D("socket(%d) has not enough space to read", msgSocket);
        return;
    }
    bytesRecv =
    recv(msgSocket, &sockets[index].recvBuffer[len], sizeof(sockets[index].recvBuffer) - len, 0);
    if (bytesRecv <= 0) {
        closeSocketFd(msgSocket);
        removeSocket(index);
        return;
    }

  DEBUG_LOG_D("socket(%d) recv bytes len:%d", msgSocket, bytesRecv);
  sockets[index].recvLen += bytesRecv;

  parseRecvData(index);
  // TODO(penciljiang) we should support send can be called in inner layer
//  sockets[index].send = SEND;
  //sendMessage(index);
	return;
}


void acceptConnection(int index) 
{
    int id = sockets[index].id;
    struct sockaddr_in from;        // Address of sending partner
    int fromLen = sizeof(from);
    int msgSocket;
    char clientIp[16] = {0};
    int port = 0;
    int ret;

    msgSocket = accept(id, (struct sockaddr *) &from, (socklen_t *)(&fromLen));
    DEBUG_LOG_D("accept socket(%d), current cnts:%d",  msgSocket, socketsCount);
    if (msgSocket < 0) {
        DEBUG_LOG_D("Fail to accept socket:%d",  msgSocket);
        return;
    }
    // sockeadd_in from
    strcpy(clientIp, inet_ntoa(from.sin_addr));
    port = ntohs(from.sin_port);
    DEBUG_LOG_I("accept socket client ip:%s client port:%d len:%d", clientIp, port, strlen(clientIp));

    ret = make_socket_nonblocking(msgSocket);
    if (ret < 0) {
        closeSocketFd(id);
        DEBUG_LOG_D("make accept socket fd nonblocking fail",  msgSocket);
        return;
    }
    if (addSocket(msgSocket, RECEIVE, clientIp, port) == 0) {
        closeSocketFd(id);
    }
    return;
}


void sendMessage(int index) 
{
    int bytesSent = 0;
    int sendLen = sockets[index].sendLen;
    int msgSocket;
    int spareLen;

    DEBUG_LOG_I("sendMessage");

    if (sendLen <= 0) {
        return;
    }
    msgSocket = sockets[index].id;
    bytesSent = send(msgSocket, sockets[index].sendBuffer, sendLen, 0);
    if (bytesSent <= 0) {
        DEBUG_LOG_E("socket(%d) fail to send sendLen:%d", msgSocket, sendLen);
        return;
    }

    if (bytesSent < sendLen) {
        DEBUG_LOG_W("socket(%d) send buffer is not enough", msgSocket);
        // re cache data
        spareLen = sendLen - bytesSent;
        memmove(sockets[index].sendBuffer, sockets[index].sendBuffer + bytesSent, spareLen);
        sockets[index].sendLen = spareLen;
        sockets[index].send = SEND;
        return;
    }

    if (bytesSent == sendLen) {
        sockets[index].sendLen = 0;
        sockets[index].send = IDLE;
    }

    DEBUG_LOG_D("socket(%d) send bytes len:%d", msgSocket, bytesSent);
    return;
}

void parseRecvData(int index) 
{
    int ret = 0;
    int headLen = sizeof(RouterProtocolHead);
    int spareLen;
    int payloadLen;

    do {
        // ret means payload data
        ret = parseRouterProtocol(sockets[index].recvBuffer, sockets[index].recvLen);
        // if ret > 0 , means data is valid
        if (ret < 0) {
            DEBUG_LOG_E("parseRouterProtocol error, just close socket:%d", sockets[index].id);
            closeSocketFd(sockets[index].id);
            removeSocket(index);
            break;
        }
        if (ret == 0){
            break;
        }
        payloadLen = ret;

        // TODO we should parse json recv data.
        handleRecvData(index, sockets[index].recvBuffer + headLen, payloadLen);


        spareLen = sockets[index].recvLen - payloadLen - headLen;
        if (spareLen > 0) {
            memmove(sockets[index].recvBuffer, sockets[index].recvBuffer + headLen + payloadLen, spareLen);
        }
        sockets[index].recvLen = sockets[index].recvLen - headLen - payloadLen;
    } while (ret > 0 && sockets[index].recvLen > 0);
    return;
}

int handleRecvData(const int index, const char *recvData, const int len) 
{
    char data[len + 1];
    memcpy(data, recvData, len);
    data[len] = '\0';
    DEBUG_LOG_I("TODO handleRecvData recvData:%s, len:%d", data, len);
    handle_json(index,data);
    //handleSendData(index, data, len);
    return 0;
}

int handleSendData(const int index, const char *sendData, const int len) 
{
    char data[len + 1];
    uint8_t* req = NULL;
    size_t reqlen ;
    int spareLen;

    DEBUG_LOG_I("handleSendData");
        
    memcpy(data, sendData, len);
    data[len] = '\0';
    DEBUG_LOG_I("TODO handleSendData sendData:%s, len:%d", data, len);

    reqlen = formRouterProtocol(&req, sendData, len);
    if (reqlen <= 0) {
        DEBUG_LOG_E("Fail to formRouterProtocol, as reqlen is:%d", reqlen);
        return -1;
    }
    // copy to send buff, and mark send flag to send
    spareLen = MAX_BUFF_SIZE - reqlen - sockets[index].sendLen;
    if (spareLen < 0) {
        DEBUG_LOG_E("Fail to send data, as send-buff is full, spareLen:%d", spareLen);
        if (req) {
            free(req);
            req = NULL;
        }
        return -1;
    }

    memcpy(sockets[index].sendBuffer, req, reqlen);
    sockets[index].sendLen += reqlen;
    sockets[index].send = SEND;

    if (req) {
        free(req);
        req = NULL;
    }
    return 0;
}
