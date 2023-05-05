/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef UDP_SERVER_CONNECTIONS_H
#define UDP_SERVER_CONNECTIONS_H

#include <stdint.h>
#include <udp-server/udp_server.h>

struct UdpTransportOut;

typedef struct UdpServerConnectionsRemote {
    struct sockaddr_in addr;
    struct UdpServerConnections* parent;
} UdpServerConnectionsRemote;

typedef struct UdpServerConnections {
    struct UdpServerSocket* udpServer;
    UdpServerConnectionsRemote connections[16];
    size_t connectionCapacity;
} UdpServerConnections;

void udpServerConnectionsInit(UdpServerConnections* self, UdpServerSocket* udpServer);
int udpServerConnectionsReceive(UdpServerConnections* self, int* connection, uint8_t* buf,
                                           size_t maxDatagramSize, bool* wasConnected, struct UdpTransportOut* response);
int udpServerConnectionsSend(UdpServerConnections* self, int connectionId, uint8_t* buf, size_t
octetCount);

#endif
