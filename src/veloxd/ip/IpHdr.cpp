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

#include "veloxd/ip/IpHdr.h"

int
IpHdrs::hlen(const IpHdr* iphdr)
{
    return iphdr->ihl;
}

bool
IpHdrs::isFrag(const IpHdr* iphdr)
{
    __le16 off = __be16_to_cpu(iphdr->frag_off);
    return !((off & IP_OFF_MASK) == 0 && (off & IP_MF_MASK) == 0);
}

bool
IpHdrs::moreFrag(const IpHdr* iphdr)
{
    return __be16_to_cpu(iphdr->frag_off) & IP_MF_MASK;
}

IpHdrs::ProtocolX
IpHdrs::protocol(const IpHdr* iphdr)
{
    static enum ProtocolX protoxmap[255];
    // g++: sorry, unimplemented
    //
    // = {
    //         [1] = ProtocolX::ICMP,
    //         [2] = ProtocolX::IGMP,
    //         [6] = ProtocolX::TCP,
    //         [17] = ProtocolX::UDP,
    protoxmap[1] = ProtocolX::ICMP;
    protoxmap[2] = ProtocolX::IGMP;
    protoxmap[6] = ProtocolX::TCP;
    protoxmap[17] = ProtocolX::UDP;

    return protoxmap[iphdr->protocol];
}

bool
IpHdrs::checksum(const IpHdr* iphdr)
