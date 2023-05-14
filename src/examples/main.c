/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <clog/console.h>
#include <stdio.h>
#include <udp-server-connections/connections.h>

#if !TORNADO_OS_WINDOWS
#include <unistd.h>
#endif

clog_config g_clog;

int main(int argc, char* argv[])
{
    (void) argc;
    (void) argv;

    g_clog.log = clog_console;
    CLOG_VERBOSE("udp server example start")
    UdpServerSocket socket;

    udpServerStartup();

    int errorCode = udpServerInit(&socket, 27000, true);
    if (errorCode < 0) {
        CLOG_WARN("could not init %d", errorCode);
        return errorCode;
    }

    UdpServerConnections serverConnections;
    Clog log;
    log.config = &g_clog;
    log.constantPrefix = "udpConnectionsServer";
    udpServerConnectionsInit(&serverConnections, &socket, log);

    CLOG_VERBOSE("initialized")

    DatagramTransportMulti* multiTransport = &serverConnections.multiTransport;

#define MAX_BUF_SIZE (1200U)
    uint8_t buf[MAX_BUF_SIZE];
    uint8_t outBuf[MAX_BUF_SIZE];
    uint32_t tickId = 0;
    int connectionIndex;
    while (true) {
        size_t octetCountFound = datagramTransportMultiReceiveFrom(multiTransport, &connectionIndex, buf, MAX_BUF_SIZE);
        if (octetCountFound > 0) {
            tc_snprintf(outBuf, MAX_BUF_SIZE, "Nice to see '%s'", buf);
            //            CLOG_INFO("received connection:%d octetCount:%zu '%s'", connectionIndex, octetCountFound,
            //            buf);
            datagramTransportMultiSendTo(multiTransport, connectionIndex, outBuf, strlen(outBuf) + 1);
        }
        tickId++;
    }
}
