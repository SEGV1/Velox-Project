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

#ifndef VELOXD_IP_REASSEMBLY_FRAGGROUP_H
#define VELOXD_IP_REASSEMBLY_FRAGGROUP_H

#include <asm/byteorder.h>
#include <cstdint>
#include <ev.h>
#include <memory>

class FrameBuf;

struct FragGroup
{
    ev_timer timer; // C style of subclassing

    // identify an IP packet
    __be16 ip_id;
    __be16 ip_protocol;
    __be32 ip_saddr;
    __be32 ip_daddr;

    std::shared_ptr<FrameBuf> first;
    // std::shared_ptr<FrameBuf> last;
};

#endif // VELOXD_IP_REASSEMBLY_FRAGGROUP_H
