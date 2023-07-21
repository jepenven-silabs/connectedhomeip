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


#include <openthread/icmp6.h>
#include <openthread/ip6.h>

#if INET_CONFIG_ENABLE_IPV4
#error Forbidden : native Open Thread implementation with IPV4 enabled
#endif

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
class DLL_EXPORT IPAddressImplOpenThread
{
public:
    /**
     * Maximum length of the string representation of an IP address, including a terminating NUL.
     */
#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN OT_IP6_ADDRESS_STRING_SIZE
#endif
    static constexpr uint16_t kMaxStringLength = OT_IP6_ADDRESS_STRING_SIZE;

    IPAddressImplOpenThread() = default;

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

    otIp6Address ToIPv6() const;

    protected:
    static void ConvertIPv6(void * dest, uint32_t size, otIp6Address address);


};

using IPAddressImpl = IPAddressImplOpenThread;
using IPv6AddressType = otIp6Address;
static_assert(std::is_trivial<IPAddressImpl>::value, "IPAddressImpl is not trivial");

} // namespace Inet
} // namespace chip
