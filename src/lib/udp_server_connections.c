/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <udp-server-connections/connections.h>

static int udpServerConnectionsReceive(void* _self, int* connection, uint8_t* buf, size_t maxDatagramSize)
{
    UdpServerConnections* self = (UdpServerConnections*) _self;
    struct sockaddr_in receivedFromAddr;
    int octetsReceived = udpServerReceive(self->udpServer, buf, &maxDatagramSize, &receivedFromAddr);
    if (octetsReceived == 0) {
        return 0;
    }

    if (octetsReceived < 0) {
        return octetsReceived;
    }

    for (size_t i = 0; i < self->connectionCapacity; ++i) {
        if (receivedFromAddr.sin_addr.s_addr == self->connections[i].addr.sin_addr.s_addr &&
            receivedFromAddr.sin_port == self->connections[i].addr.sin_port) {
            *connection = i;
            return octetsReceived;
        }
    }

    for (size_t i = 0; i < self->connectionCapacity; ++i) {
        if (self->connections[i].addr.sin_addr.s_addr == 0) {
            *connection = i;
            self->connections[i].addr = receivedFromAddr;
            CLOG_DEBUG("Received from new udp address. Assigned to connectionId: %zu", i)
            return octetsReceived;
        }
    }

    CLOG_SOFT_ERROR("could not receive from udp server")
    return -94;
}

static int udpServerConnectionsSend(void* _self, int connectionId, const uint8_t* buf, size_t octetCount)
{
    UdpServerConnections* self = (UdpServerConnections*) _self;

    UdpServerConnectionsRemote* connection = &self->connections[connectionId];

    return udpServerSend(self->udpServer, buf, octetCount, &connection->addr);
}

void udpServerConnectionsInit(UdpServerConnections* self, UdpServerSocket* udpServer)
{
    self->udpServer = udpServer;
    self->connectionCapacity = 16;
    for (size_t i = 0; i < self->connectionCapacity; ++i) {
        self->connections[i].addr.sin_addr.s_addr = 0;
        self->connections[i].parent = self;
    }

    self->multiTransport.self = self;
    self->multiTransport.receive = udpServerConnectionsReceive;
    self->multiTransport.send = udpServerConnectionsSend;
}
