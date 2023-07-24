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
class DLL_EXPORT IPAddressImplLwIP
{
public:
    /**
     * Maximum length of the string representation of an IP address, including a terminating NUL.
     */
    static constexpr uint16_t kMaxStringLength = IP6ADDR_STRLEN_MAX;

    IPAddressImplLwIP() = default;

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

    ip6_addr_t ToIPv6(const void * addr, const uint32_t size) const;


    /**
     * @fn      ToImplAddr() const
     *
     * @brief   Extract the IP address as a LwIP ip_addr_t structure.
     *
     * @details
     *  Use <tt>ToImplAddr() const</tt> to extract the content as an IP address,
     *  if possible.
     *
     * @return  An LwIP ip_addr_t structure corresponding to the IP address.
     */
    ip_addr_t ToImplAddr(void) const;

    /**
     * @brief   Convert the INET layer address type to its underlying LwIP type.
     *
     * @details
     *  Use <tt>ToLwIPAddrType(IPAddressType)</tt> to convert the IP address type
     *  to its underlying LwIP address type code.
     */
    static lwip_ip_addr_type ToLwIPAddrType(IPAddressType);

    (void) const;

#if INET_CONFIG_ENABLE_IPV4
    ip4_addr_t ToIPv4(const void * addr, const uint32_t size) const;
#endif // INET_CONFIG_ENABLE_IPV4

    protected:
    static void ConvertIPv6(void * dest, const uint32_t size, const otIp6Address & address);


};

using IPAddressImpl = IPAddressImplLwIP;
using IPv6AddressType = in6_addr;
using IPAddressType = ip_addr_t;
#if INET_CONFIG_ENABLE_IPV4
using IPv4AddressType = ip4_addr_t;
#endif // INET_CONFIG_ENABLE_IPV4

static_assert(std::is_trivial<IPAddressImpl>::value, "IPAddressImpl is not trivial");

} // namespace Inet
} // namespace chip
