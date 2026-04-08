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

#ifndef VELOXD_UDP_UDPSESSION_H
#define VELOXD_UDP_UDPSESSION_H

#include "veloxd/Session.h"

class FrameBuf;

class UdpSession : public Session
{
public:
    // let compiler generate ctor/dctor

    // override functions
    virtual int send(const std::string& buf) override;
    virtual int send(const struct in_addr& faddr, __be16 fport,
                     const std::string& buf) override;
    virtual void recv(struct sockaddr_in& peeraddr,
                      const std::shared_ptr<FrameBuf>&) override;

    virtual __be16 nextAvailLocalPort() override;
};

#endif
