/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <secure-random/secure_random.h>
#include <udp-server-connections/unique_id.h>

UdpServerConnectionsUniqueId udpServerConnectionsUniqueIdFromIndex(size_t index)
{
    uint64_t upperPart = secureRandomUInt64();
    uint8_t lowerIndex = index & 0xff;

    UdpServerConnectionsUniqueId uniqueIndex = ((uint64_t) upperPart << 8) | lowerIndex;

    return uniqueIndex;
}

size_t udpServerConnectionsUniqueIdGetIndex(UdpServerConnectionsUniqueId uniqueIndex)
{
    return uniqueIndex & 0xff;
}
