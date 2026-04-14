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

#include "veloxd/Session.h"

#include <arpa/inet.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

#include "veloxd/Conf.h"
#include "veloxd/Logging.h"

using std::weak_ptr;
using std::shared_ptr;

Session::Session()
{
    {
        VELOXD_LOG(debug) << "New Session";
    }

    bzero(&faddr, sizeof(struct in_addr));
    bzero(&fport, sizeof(__be16));

    bzero(&laddr, sizeof(struct in_addr));
    {
        const string& addr_str = Conf::get()->stack.addr;
        inet_pton(AF_INET, addr_str.c_str(), &laddr);
    }

    bzero(&lport, sizeof(__be16));
}

Session::~Session()
{
    VELOXD_LOG(debug) << "Destroy Session";
}

int
Session::connect(const struct in_addr& faddr, __be16 fport)
{
    this->faddr = faddr;
    this->fport = fport;

    if (lport == 0) {
        this->bind();
    }

    return 0;
}

void
Session::setOnConnEstabCB(const std::function<void()>&)
{
}

IoHandle*
Session::getConnEstabNotifyChannel()
{
    return nullptr;
}

void
Session::disconnect()
{
    bzero(&faddr, sizeof(struct in_addr));
    fport = 0;
}

int
Session::send(const std::string& buf)
{
    if (lport == 0) {
        this->bind();
    }

    if (fport == 0) {
        return ENOTCONN;
    }
    return 0;
}

int
Session::send(const struct in_addr& faddr, __be16 fport, const std::string& buf)
{
    if (lport == 0) {
        this->bind();
    }

    if (fport != 0) {
        return EISCONN;
    }
    return 0;
}

void
Session::recv(const std::shared_ptr<FrameBuf>&)
{
    // nothing need to do
}

void
Session::recv(sockaddr_in& peeraddr, const std::shared_ptr<FrameBuf>&)
{
}

int
Session::bind()
{
    if (lport == 0) {
        lport = nextAvailLocalPort();
    }
}

weak_ptr<Endpoint>
Session::socket()
{
    return weak_ptr<Endpoint>{ _socket };
}

void
Session::setSocket(const std::weak_ptr<Endpoint>& so)
{
    _socket = so;
}
