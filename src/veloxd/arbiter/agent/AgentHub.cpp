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

#include "AgentHub.h"

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "veloxd/Logging.h"
#include "veloxd/Veloxd.h"
#include "veloxd/arbiter/IoHandle.h"
#include "rpc/rpc.h"

using std::shared_ptr;
using std::make_shared;

struct AgentHub::Impl
{
    shared_ptr<IoHandle> listenChannel;
    int fd;

    std::function<void(int connfd)> onNewConnectionFunc;

    void onReadReady();
};

AgentHub::AgentHub()
{
    _pImpl.reset(new AgentHub::Impl);

    /*  1. Prepare data required for creating/binding the UNIX socket */

    const char* socketPath =
        RPC_MASTER_LISTENER_SOCKET_DIR "/" RPC_MASTER_LISTENER_SOCKET_NAME;

    struct sockaddr_un sa;
    memset(&sa, 0, sizeof(struct sockaddr_un));
    sa.sun_family = AF_UNIX;
    strncpy(sa.sun_path, socketPath, sizeof(sa.sun_path) - 1);

    /*  2. Create/bind the UNIX socket */

    // Ensure the parent directory exists & remove any leftover files
    ::mkdir(RPC_MASTER_LISTENER_SOCKET_DIR,
            S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    unlink(socketPath);

    int listenfd = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_NONBLOCK, 0);
    int ret =
        bind(listenfd, (const struct sockaddr*)&sa, sizeof(struct sockaddr_un));
    if (ret == -1) {
        int errno_ = errno;
        VELOXD_LOG(fatal) << "Failed to create AgentHub socket: "
                             << std::string(strerror(errno_));
        Veloxd::get()->stop();
        Veloxd::get()->exit(Veloxd::exit_status::FAILURE);
    }

    VELOXD_LOG(debug) << "AgentHub fd: " << listenfd;

    /*  3. IoHandle (for EventReactor use) of self */

    _pImpl->fd = listenfd;

    auto listenCh = make_shared<IoHandle>(listenfd);
    listenCh->setOnReadCB([this]() { this->_pImpl->onReadReady(); });
    listenCh->enableReading();
    _pImpl->listenChannel = listenCh;
}

AgentHub::~AgentHub()
{
    ::close(_pImpl->fd);
    ::unlink(RPC_MASTER_LISTENER_SOCKET_DIR
             "/" RPC_MASTER_LISTENER_SOCKET_NAME);
}

void
AgentHub::listen()
{
    int ret = ::listen(_pImpl->fd, 20);
    if (ret == -1) {
        int errno_ = errno;
        VELOXD_LOG(fatal) << "Failed to listen AgentHub socket: "
                             << std::string(strerror(errno_));
        Veloxd::get()->stop();
        Veloxd::get()->exit(Veloxd::exit_status::FAILURE);
    }
}

std::shared_ptr<IoHandle>
AgentHub::getIoHandle()
{
    return _pImpl->listenChannel;
}

void
AgentHub::onNewConnection(const std::function<void(int sockfd)>& callback)
{
    _pImpl->onNewConnectionFunc = callback;
}

void
AgentHub::Impl::onReadReady()
{
    auto listenfd = this->fd;
    auto callback = this->onNewConnectionFunc;

    /*  1. accept */

    int connfd;
    while ((connfd = ::accept(listenfd, NULL, NULL)) != -1) {
        if (callback) {
            callback(connfd);
        }
    }

    /*  2. check errno */

    int errno_ = errno;
    if (errno_ != EWOULDBLOCK) {
        VELOXD_LOG(error)
            << "AgentHub.accept: " << std::string(strerror(errno_));
    }
}
