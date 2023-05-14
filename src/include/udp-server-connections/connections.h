/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef UDP_SERVER_CONNECTIONS_H
#define UDP_SERVER_CONNECTIONS_H

#include <stdint.h>
#include <udp-connections-serialize/serialize.h>
#include <udp-server/udp_server.h>
#include <datagram-transport/multi.h>

typedef struct UdpServerConnectionsRemote {
    struct sockaddr_in addr;
    struct UdpServerConnections* parent;
    UdpConnectionsSerializeConnectionId fullConnectionId;
    UdpConnectionsSerializeClientNonce clientNonce;
} UdpServerConnectionsRemote;

typedef struct UdpServerConnections {
    struct UdpServerSocket* udpServer;
    UdpServerConnectionsRemote connections[16];
    size_t connectionCapacity;
    DatagramTransportMulti multiTransport;
    UdpConnectionsSerializeServerChallenge secretChallengeKey;
    Clog log;
} UdpServerConnections;

void udpServerConnectionsInit(UdpServerConnections* self, UdpServerSocket* udpServer, Clog log);

#endif
