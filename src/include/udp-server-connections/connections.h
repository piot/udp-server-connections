/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef UDP_SERVER_CONNECTIONS_H
#define UDP_SERVER_CONNECTIONS_H

#include <stdint.h>
#include <udp-server/udp_server.h>
#include <udp-transport/multi.h>

typedef struct UdpServerConnectionsRemote {
    struct sockaddr_in addr;
    struct UdpServerConnections* parent;
} UdpServerConnectionsRemote;

typedef struct UdpServerConnections {
    struct UdpServerSocket* udpServer;
    UdpServerConnectionsRemote connections[16];
    size_t connectionCapacity;
    DatagramTransportMultiInOut multiTransport;
} UdpServerConnections;

void udpServerConnectionsInit(UdpServerConnections* self, UdpServerSocket* udpServer);

#endif
