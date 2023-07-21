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

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <inet/IPAddressImplOpenThread.h>

#include <inet/InetError.h>
#include <lib/core/CHIPEncoding.h>
#include <lib/support/CodeUtils.h>

#include "arpa-inet-compatibility.h"

#include <stdint.h>
#include <string.h>

namespace chip {
namespace Inet {

otIp6Address IPAddressImplOpenThread::ToIPv6() const
{
    otIp6Address otAddr;
    static_assert(sizeof(otAddr.mFields.m32) == sizeof(Addr), "otIp6Address size mismatch");
    memcpy(otAddr.mFields.m32, Addr, sizeof(otAddr.mFields.m32));
    return otAddr;
}

void IPAddressImplOpenThread::ConvertIPv6(void * dest, uint32_t  size, otIp6Address & address)
{
    static_assert(sizeof(address.mFields.m32) == size, "otIp6Address size mismatch");
    memcpy(dest, address.mFields.m32, size);
}

} // namespace Inet
} // namespace chip