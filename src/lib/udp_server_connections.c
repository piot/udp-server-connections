/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <flood/in_stream.h>
#include <flood/out_stream.h>
#include <secure-random/secure_random.h>
#include <udp-connections-serialize/serialize.h>
#include <udp-connections-serialize/server_in.h>
#include <udp-connections-serialize/server_out.h>
#include <udp-server-connections/connections.h>
#include <udp-server-connections/unique_id.h>

static uint64_t extremelyUnsecureCipher(uint64_t publicKey, uint64_t secretKey)
{
    return publicKey ^ secretKey;
}

static int inChallenge(UdpServerConnections* self, struct sockaddr_in sockAddr, FldInStream* inStream)
{
    UdpConnectionsSerializeClientNonce clientNonce;
    udpConnectionsSerializeServerInChallenge(inStream, &clientNonce);

    // Challenge is done to avoid at least the simplest forms of IP spoofing
    uint64_t constructedChallenge = extremelyUnsecureCipher(clientNonce, self->secretChallengeKey);

    CLOG_C_VERBOSE(&self->log, "received challenge request from client nonce %016lX and constructed challenge %016lX",
                   clientNonce, constructedChallenge)

    FldOutStream outStream;
    uint8_t buf[32];
    fldOutStreamInit(&outStream, buf, 32);

    udpConnectionsSerializeServerOutChallengeResponse(&outStream, clientNonce, constructedChallenge);

    return udpServerSend(self->udpServer, buf, outStream.pos, &sockAddr);
}

static int findFreeConnectionIndex(UdpServerConnections* self)
{
    for (size_t i = 0; i < self->connectionCapacity; ++i) {
        UdpServerConnectionsRemote* connection = &self->connections[i];
        if (connection->fullConnectionId == 0) {
            return (int) i;
        }
    }
    return -2;
}

static int inConnect(UdpServerConnections* self, struct sockaddr_in sockAddr, FldInStream* inStream)
{
    UdpConnectionsSerializeClientNonce clientNonce;
    UdpConnectionsSerializeServerChallenge serverChallengeFromClient;

    udpConnectionsSerializeServerInConnect(inStream, &clientNonce, &serverChallengeFromClient);

    uint64_t calculatedClientNonce = extremelyUnsecureCipher(serverChallengeFromClient, self->secretChallengeKey);
    if (calculatedClientNonce != clientNonce) {
        CLOG_C_SOFT_ERROR(&self->log, "challenge didnt work out")
        return -2;
    }

    int foundIndex = findFreeConnectionIndex(self);
    if (foundIndex < 0) {
        CLOG_C_SOFT_ERROR(&self->log, "no free connections")
        return foundIndex;
    }

    UdpServerConnectionsRemote* connection = &self->connections[foundIndex];
    UdpServerConnectionsUniqueId uniqueId = udpServerConnectionsUniqueIdFromIndex(foundIndex);
    UdpConnectionsSerializeConnectionId constructedConnectionId = uniqueId;
    connection->fullConnectionId = constructedConnectionId;
    connection->addr = sockAddr;
    connection->clientNonce = clientNonce;

    CLOG_C_DEBUG(&self->log, "connection established: nonce:%016lX connectionId:%016lX (index:%d)", clientNonce,
                 constructedConnectionId, foundIndex)

    FldOutStream outStream;
    uint8_t buf[128];
    fldOutStreamInit(&outStream, buf, 128);

    udpConnectionsSerializeServerOutConnectResponse(&outStream, clientNonce, constructedConnectionId);

    return udpServerSend(self->udpServer, buf, outStream.pos, &sockAddr);
}

static int inDatagram(UdpServerConnections* self, struct sockaddr_in sockAddr, FldInStream* inStream, uint8_t cmd)
{
    switch (cmd) {
        case UdpConnectionsSerializeCmdChallenge:
            return inChallenge(self, sockAddr, inStream);
        case UdpConnectionsSerializeCmdConnect:
            return inConnect(self, sockAddr, inStream);
        default:
            CLOG_C_SOFT_ERROR(&self->log, "illegal cmd %02X", cmd)
            return -5;
    }

    return 0;
}

static int inPacketFromClient(UdpServerConnections* self, struct sockaddr_in sockAddr, FldInStream* inStream,
                              int* foundConnectionIndex, uint8_t* buf, size_t maxDatagramSize)
{
    // CLOG_C_VERBOSE(&self->log, "received packet from client")

    UdpConnectionsSerializeConnectionId connectionId;
    uint16_t octetCountFollowingInStream;
    *foundConnectionIndex = -1;
    int err = udpConnectionsSerializeServerInPacketFromClient(inStream, &connectionId, &octetCountFollowingInStream);
    if (err < 0) {
        return err;
    }

    if (octetCountFollowingInStream > maxDatagramSize) {
        CLOG_C_SOFT_ERROR(&self->log, "target buffer too small")
        return -25;
    }

    size_t connectionIndex = udpServerConnectionsUniqueIdGetIndex(connectionId);
    if (connectionIndex >= self->connectionCapacity) {
        CLOG_C_SOFT_ERROR(&self->log, "illegal connection index, out of capacity")
        return -44;
    }

    UdpServerConnectionsRemote* connection = &self->connections[connectionIndex];
    if (connection->fullConnectionId != connectionId) {
        CLOG_C_NOTICE(&self->log, "wrong connection id. maybe from an old connection?")
        return -64;
    }

    if (connection->addr.sin_addr.s_addr != sockAddr.sin_addr.s_addr) {
        CLOG_C_NOTICE(&self->log, "mismatch in sockaddr_in. will allow it in this version, but should be handled "
                                  "differently in future versions.")
    }

    if (inStream->size - inStream->pos < octetCountFollowingInStream) {
        CLOG_C_SOFT_ERROR(&self->log, "octet count error")
        return -63;
    }

    fldInStreamReadOctets(inStream, buf, octetCountFollowingInStream);
    *foundConnectionIndex = connectionIndex;

    return octetCountFollowingInStream;
}

static int udpServerConnectionsReceive(void* _self, int* connectionIndex, uint8_t* buf, size_t maxDatagramSize)
{
    UdpServerConnections* self = (UdpServerConnections*) _self;
    if (maxDatagramSize < 1200) {
        CLOG_C_ERROR(&self->log, "you must provide a buffer of at least 1200 octets, encountered:%d", maxDatagramSize)
    }
    struct sockaddr_in receivedFromAddr;
    size_t maxAndReceiveOctetCountFromUdpServer = maxDatagramSize;
    int octetsReceived = udpServerReceive(self->udpServer, buf, &maxAndReceiveOctetCountFromUdpServer,
                                          &receivedFromAddr);
    if (octetsReceived == 0) {
        return 0;
    }

    if (octetsReceived < 0) {
        return octetsReceived;
    }

    FldInStream inStream;

    fldInStreamInit(&inStream, buf, octetsReceived);
    uint8_t cmd;
    fldInStreamReadUInt8(&inStream, &cmd);
    if (cmd == UdpConnectionsSerializeCmdPacket) {
        return inPacketFromClient(self, receivedFromAddr, &inStream, connectionIndex, buf, maxDatagramSize);
    }

    int err = inDatagram(self, receivedFromAddr, &inStream, cmd);
    if (err < 0) {
        return err;
    }

    return udpServerConnectionsReceive(_self, connectionIndex, buf, maxDatagramSize);
}

static int udpServerConnectionsSend(void* _self, int connectionId, const uint8_t* buf, size_t octetCount)
{
    UdpServerConnections* self = (UdpServerConnections*) _self;
    if (connectionId < 0 || connectionId >= (int) self->connectionCapacity) {
        return -4;
    }

    UdpServerConnectionsRemote* connection = &self->connections[connectionId];

    uint8_t outBuf[1210];
    FldOutStream outStream;

    fldOutStreamInit(&outStream, outBuf, 1210);
    udpConnectionsSerializeServerOutPacketHeader(&outStream, connection->fullConnectionId, octetCount);
    fldOutStreamWriteOctets(&outStream, buf, octetCount);

    return udpServerSend(self->udpServer, outBuf, outStream.pos, &connection->addr);
}

void udpServerConnectionsInit(UdpServerConnections* self, UdpServerSocket* udpServer, Clog log)
{
    self->log = log;
    self->udpServer = udpServer;
    self->connectionCapacity = 16;
    for (size_t i = 0; i < self->connectionCapacity; ++i) {
        self->connections[i].addr.sin_addr.s_addr = 0;
        self->connections[i].parent = self;
        self->connections[i].fullConnectionId = 0;
        self->connections[i].clientNonce = 0;
    }

    self->multiTransport.self = self;
    self->multiTransport.receiveFrom = udpServerConnectionsReceive;
    self->multiTransport.sendTo = udpServerConnectionsSend;
    self->secretChallengeKey = secureRandomUInt64();
}
