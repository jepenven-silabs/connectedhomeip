/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    Copyright (c) 2019 Google LLC.
 *    Copyright (c) 2013-2018 Nest Labs, Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *      This file implements the class <tt>Inet::IPAddress</tt> and
 *      related enumerated constants. The CHIP Inet Layer uses objects
 *      of this class to represent Internet protocol addresses of both
 *      IPv4 and IPv6 address families. (IPv4 addresses are stored
 *      internally as IPv4-Mapped IPv6 addresses.)
 *
 */

#include <inet/IPAddressImplSockets.h>
#include <stdint.h>
#include <cassert>

namespace chip {
namespace Inet {

in6_addr IPAddressImplSockets::ToIPv6(const void * addr, const uint32_t size) const
{
    in6_addr ipAddr;
    assert(sizeof(ipAddr) == size);
    memcpy(&ipAddr, addr, sizeof(ipAddr));
    return ipAddr;
}

void IPAddressImplSockets::ConvertIPv6(void * dest, const uint32_t  size, const in6_addr & address)
{
    if(dest != nullptr)
    {
        memcpy(dest, &address, sizeof(address));
    }
}

#if INET_CONFIG_ENABLE_IPV4
struct in_addr IPAddressImplSockets::ToIPv4(const void * addr) const
{
    struct in_addr ipv4Addr;
    ipv4Addr.s_addr = Addr[3];
    return ipv4Addr;
}

void IPAddressImplSockets::ConvertIPv4(void * dest, const uint32_t size, const in_addr & address)
{
    if(dest != nullptr)
    {
        dest[0] = 0;
        dest[1] = 0;
        dest[2] = htonl(0xFFFF);
        dest[3] = ipv4Addr.s_addr;
    }
}

#endif // INET_CONFIG_ENABLE_IPV4

} // namespace Inet
} // namespace chip