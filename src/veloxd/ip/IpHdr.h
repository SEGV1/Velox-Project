/*
 * Velox, an userspace TCP/IP network stack.
 * Copyright (C) 2026 SEGV1
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.

 * You should have received a copy of the GNU General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef VELOXD_IP_IPHDR_H
#define VELOXD_IP_IPHDR_H

#define IP_OFF_MASK 0x1fff
#define IP_DF_MASK 0x4000
#define IP_MF_MASK 0x2000

#include <cstdint>

// Let's just steal from linux
#include <linux/ip.h> // for iphdr
using IpHdr = ::iphdr;

class IpHdrs
{
public:
    enum class ProtocolX
    {
        IP,
        ICMP,
        IGMP,
        UDP,
        TCP,
    };

    /* IP L1 */
    static int hlen(const IpHdr*);

    /* IP L2 */
    static bool isFrag(const IpHdr*);
    static bool moreFrag(const IpHdr*);

    /* IP L3 */
    static ProtocolX protocol(const IpHdr*);
    static bool checksum(const IpHdr*);
};

class IpIdGenerator
{
public:
    // rule of zero

    __be16 next();

private:
    uint16_t last = 0;
};

#endif // VELOXD_IP_IPHDR_H
