/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef UDP_CONNECTIONS_UNIQUE_ID_H
#define UDP_CONNECTIONS_UNIQUE_ID_H

#include <stddef.h>
#include <stdint.h>

typedef uint64_t UdpServerConnectionsUniqueId;

UdpServerConnectionsUniqueId udpServerConnectionsUniqueIdFromIndex(size_t index);
size_t udpServerConnectionsUniqueIdGetIndex(UdpServerConnectionsUniqueId uniqueIndex);

#endif
