/*
 *
 *    Copyright (c) 2020-2021 Project CHIP Authors
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
 *      This file defines the class <tt>Inet::IPAddress</tt> and
 *      related enumerated constants. The CHIP Inet Layer uses objects
 *      of this class to represent Internet protocol addresses of both
 *      IPv4 and IPv6 address families. (IPv4 addresses are stored
 *      internally as IPv4-Mapped IPv6 addresses.)
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <type_traits>

#include <lib/core/CHIPError.h>
#include <lib/support/BitFlags.h>
#include <lib/support/DLLUtil.h>

#include <net/if.h>
#include <netinet/in.h>

/**
 * SockAddr should be used when calling any API that returns (by copying into
 * it) a sockaddr, because that will need enough storage that it can hold data
 * for any socket type.
 *
 * It can also be used when calling an API that accepts a sockaddr, to simplify
 * the type-punning needed.
 */
union SockAddr
{
    sockaddr any;
    sockaddr_in in;
    sockaddr_in6 in6;
    sockaddr_storage storage;
};

/**
 * SockAddrWithoutStorage can be used any time we want to do the sockaddr
 * type-punning but will not store the data ourselves (e.g. we're working with
 * an existing sockaddr pointer, and reintepret it as a
 * pointer-to-SockAddrWithoutStorage).
 */
union SockAddrWithoutStorage
{
    sockaddr any;
    sockaddr_in in;
    sockaddr_in6 in6;
};

namespace chip {
namespace Inet {

/**
 * @brief   Internet protocol address
 *
 * @details
 *  The CHIP Inet Layer uses objects of this class to represent Internet
 *  protocol addresses (independent of protocol version).
 *
 */
class DLL_EXPORT IPAddressImplSockets
{
public:
    /**
     * Maximum length of the string representation of an IP address, including a terminating NUL.
     */
    static constexpr uint16_t kMaxStringLength = INET6_ADDRSTRLEN;

    IPAddressImplSockets() = default;

    /**
     * @fn      ToIPv6() const
     *
     * @brief   Extract the IPv6 address as a platform data structure.
     *
     * @details
     *  Use <tt>ToIPv6() const</tt> to extract the content as an IPv6 address,
     *  if possible. IPv4 addresses and the unspecified address are extracted
     *  as <tt>[::]</tt>.
     *
     *  The result is either of type <tt>struct in6_addr</tt> (on POSIX) or
     *  <tt>ip6_addr_t</tt> (on LwIP).
     *
     * @return  The encapsulated IPv4 address, or \c [::] if the address is
     *      either unspecified or not an IPv4 address.
     */

    in6_addr ToIPv6(const void * addr, const uint32_t size) const;

#if INET_CONFIG_ENABLE_IPV4
    in_addr ToIPv4() const;
#endif // INET_CONFIG_ENABLE_IPV4


    protected:
    static void ConvertIPv6(void * dest, const uint32_t size, const in6_addr & address);
    inline static bool IsGenericIPv6(const SockAddrWithoutStorage & sockaddr) {return (sockaddr.any.sa_family == AF_INET6); }
    inline static in6_addr GetIpv6Addr(const SockAddrWithoutStorage & sockaddr) {return sockaddr.in6.sin6_addr;}

#if INET_CONFIG_ENABLE_IPV4
    static void ConvertIPv4(void * dest, const uint32_t size, const in_addr & address);
    inline static bool IsGenericIPv4(const SockAddrWithoutStorage & sockaddr) {return (sockaddr.any.sa_family == AF_INET); }
    inline static in_addr GetIpv4Addr(const SockAddrWithoutStorage & sockaddr) {return sockaddr.in.sin_addr;}
#endif // INET_CONFIG_ENABLE_IPV4

};

using IPAddressImpl = IPAddressImplSockets;
using IPv6AddressType = in6_addr;
using IPv4AddressType = in_addr;
using IPGenericAddress = sockaddr;
using IPGenericAddressWithoutStorage = SockAddrWithoutStorage;
static_assert(std::is_trivial<IPAddressImpl>::value, "IPAddressImpl is not trivial");

} // namespace Inet
} // namespace chip
