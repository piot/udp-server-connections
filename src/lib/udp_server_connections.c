/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <udp-server-connections/connections.h>
#include <udp-transport/udp_transport.h>
#include <clog/clog.h>

void udpServerConnectionsInit(UdpServerConnections* self, UdpServerSocket* udpServer)
{
    self->udpServer = udpServer;
    self->connectionCapacity = 16;
    for (size_t i = 0; i < self->connectionCapacity; ++i) {
        self->connections[i].addr.sin_addr.s_addr = 0;
        self->connections[i].parent = self;
    }
}

static int wrappedSend(void* _self, const uint8_t* data, size_t size)
{
    UdpServerConnectionsRemote* self = _self;
    return udpServerSend(self->parent->udpServer, data, size, &self->addr);
}

int udpServerConnectionsReceive(UdpServerConnections* self, int* connection, uint8_t* buf,
                                           size_t maxDatagramSize, bool* wasConnected, UdpTransportOut* response)
{
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
            response->self = &self->connections[i];
            response->send = wrappedSend;
            *wasConnected = false;
            return octetsReceived;
        }
    }

    for (size_t i = 0; i < self->connectionCapacity; ++i) {
        if (self->connections[i].addr.sin_addr.s_addr == 0) {
            *connection = i;
            response->self = &self->connections[i];
            response->send = wrappedSend;
            self->connections[i].addr = receivedFromAddr;
            *wasConnected = true;
            CLOG_DEBUG("Received from new udp address. Assigned to connectionId: %zu", i)
            return octetsReceived;
        }
    }

    CLOG_SOFT_ERROR("could not receive from udp server")
    return -94;
}

int udpServerConnectionsSend(UdpServerConnections* self, int connectionId, uint8_t* buf, size_t
octetCount)
{
    UdpServerConnectionsRemote* connection = &self->connections[connectionId];

    return udpServerSend(self->udpServer, buf, octetCount, &connection->addr);
}
