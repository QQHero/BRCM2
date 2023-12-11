//
// Created by penciljiang on 2022/1/10.
//

#ifndef START_ROUTER_ACCGW_ROUTER_PROTOCOL_H
#define START_ROUTER_ACCGW_ROUTER_PROTOCOL_H

/*
0              1               2               3               4
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|0 0 1 0 0 0 1 1|     version     |            reserved         |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|               payloadlen（Must, if no payload, len = 0）       |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                        payload (Option)                       \
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

1Byte: magic_number 0x23
2Byte: version, 协议版本号  0x01
3-4Byte: 保留位
5-8Byte:  载荷长度,可以为0
9-...Btye:  实际载荷 示例：
为JSON格式
{"cmd":"start.handshake","result":"1","data":{"proxyip":"2.3.4.5","dstport":"80","instanceid":"abcdefgh"}}
*/

// https://iwiki.woa.com/pages/viewpage.action?pageId=1340740571

#include <malloc.h>

#define ROUTER_PROTOCOL_MAGIC 0x23
#define ROUTER_PROTOCOL_VERSION 0x01
#define ROUTER_PROTOCOL_CACHE_THRESHOLD 2097152

#define ROUTER_CMD_START_HANDSHAKE "start.handshake"
#define ROUTER_CMD_START_HEARTBEAT "start.heartbeat"
#define ROUTER_CMD_START_FINISH "start.finish"

#pragma pack(1)

typedef struct RouterProtocolHead {
  uint8_t magic;  // router protocol
  uint8_t version;
  uint16_t reserved;
  uint32_t payloadlen;  // payload len
  uint8_t payload[0];
} RouterProtocolHead;

#pragma pack()

int parseRouterProtocol(const char* data, uint32_t len) {
  if (len < sizeof(RouterProtocolHead)) {
    return 0;
  }
  RouterProtocolHead* head = (RouterProtocolHead*)data;
  if (head->magic != ROUTER_PROTOCOL_MAGIC) {
    DEBUG_LOG_E("RouterProtocol magic error");
    return -1;
  }
  if (head->version != ROUTER_PROTOCOL_VERSION) {
    DEBUG_LOG_E("RouterProtocol version error:");
    return -1;
  }
  int payloadLen = ntohl(head->payloadlen);
  int totalLen = payloadLen + sizeof(RouterProtocolHead);
  if (len < totalLen) {
    return 0;
  }
  if (totalLen > ROUTER_PROTOCOL_CACHE_THRESHOLD) {
    DEBUG_LOG_E("RouterProtocol recv payload len too large:%d", totalLen);
    return -1;
  }
  DEBUG_LOG_V("RouterProtocol parse payload(%d):%s", payloadLen, head->payload);
  return payloadLen;
}

size_t formRouterProtocol(uint8_t** req, const char* data, uint32_t len) {
  size_t reqLen = sizeof(RouterProtocolHead) + len;
  *req = (uint8_t*)(malloc((reqLen)));
  if (*req == NULL) {
    return 0;
  }
  memset(*req, 0, reqLen);
  RouterProtocolHead* protocolHead = (RouterProtocolHead*)(*req);
  protocolHead->magic = ROUTER_PROTOCOL_MAGIC;
  protocolHead->version = ROUTER_PROTOCOL_VERSION;
  protocolHead->reserved = 0;
  protocolHead->payloadlen = htonl(len);
  memmove(protocolHead->payload, data, len);
  return reqLen;
}

#endif  // START_ROUTER_ACCGW_ROUTER_PROTOCOL_H
