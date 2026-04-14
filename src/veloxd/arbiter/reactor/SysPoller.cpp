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

#include "SysPoller.h"

#include <iostream>
#include <sstream>
#include <sys/epoll.h>
#include <unistd.h>
#include <vector>

#include "veloxd/Logging.h"
#include "veloxd/arbiter/IoHandle.h"

using std::vector;
using std::shared_ptr;
using std::make_shared;

struct SysPoller::Impl
{
    int epollfd;

    // muduo.net.poller.EpollSysPoller choose 16 as initialMaxEvents.
    static const int maxEventsReceivedEachPoll = 64;
};

SysPoller::SysPoller()
{
    _pImpl.reset(new SysPoller::Impl);

    // linux kernel since 2.6.8 will ignore the epoll_create arguments,
    // so, just set it to 1
    int epfd = epoll_create(1);
    _pImpl->epollfd = epfd;
}

SysPoller::~SysPoller()
{
    ::close(_pImpl->epollfd);
}

void
SysPoller::addChannel(IoHandle* c)
{
    struct epoll_event ev;
    ev.events = c->eventsInterested();
    // That's why when epoll_wait returns, we can know
    // which channel got events: we stored the channel
    // ptr in epoll_event.
    ev.data.ptr = (void*)c;

    int ret = ::epoll_ctl(_pImpl->epollfd, EPOLL_CTL_ADD, c->fd(), &ev);
    if (ret == -1) {
        int errno_ = errno;
        VELOXD_LOG(error) << "SysPoller can't add channel {fd=" << c->fd()
                             << ",events=" << c->eventsInterested() << "}"
                             << " : " << std::string(strerror(errno_));
    }
}

void
SysPoller::removeChannel(IoHandle* c)
{
    int ret = ::epoll_ctl(_pImpl->epollfd, EPOLL_CTL_DEL, c->fd(), NULL);
    if (ret == -1) {
        int errno_ = errno;
        VELOXD_LOG(error) << "SysPoller can't remove channel {fd=" << c->fd()
                             << ",events=" << c->eventsInterested() << "}"
                             << " : " << std::string(strerror(errno_));
    }
}

void
SysPoller::updateChannel(IoHandle* c)
{
    struct epoll_event ev;
    ev.events = c->eventsInterested();
    ev.data.ptr = (void*)c;

    int ret = ::epoll_ctl(_pImpl->epollfd, EPOLL_CTL_MOD, c->fd(), &ev);
    if (ret == -1) {
        int errno_ = errno;
        VELOXD_LOG(error) << "SysPoller can't mod channel {fd=" << c->fd()
                             << ",events=" << c->eventsInterested() << "}"
                             << " : " << std::string(strerror(errno_));
    }
}

void
SysPoller::poll(int timeoutMs, std::vector<IoHandle*>& actives)
{
    struct epoll_event eventsReceived[_pImpl->maxEventsReceivedEachPoll];

    int nevents = ::epoll_wait(_pImpl->epollfd, eventsReceived,
                               _pImpl->maxEventsReceivedEachPoll, timeoutMs);

    if (nevents == -1) {
        int errno_ = errno;
        VELOXD_LOG(error)
            << "SysPoller poll failed: " << std::string(strerror(errno_));
    }

    if (nevents == 0) {
        return;
    }

    {
        std::vector<int> active_fds;
        for (int i = 0; i < nevents; i++) {
            auto sChannel =
                static_cast<IoHandle*>(eventsReceived[i].data.ptr);
            active_fds.push_back(sChannel->fd());
        }

        std::ostringstream oss;
        std::copy(active_fds.begin(), active_fds.end() - 1,
                  std::ostream_iterator<int>(oss, ","));
        oss << active_fds.back();

        VELOXD_LOG(info)
            << "SysPoller polled " << nevents << " fd(s): " << oss.str();
    }

    for (int i = 0; i < nevents; i++) {
        auto channel =
            static_cast<IoHandle*>(eventsReceived[i].data.ptr);
        channel->setEventsReceived(eventsReceived[i].events);
        actives.push_back(channel);
    }
}
