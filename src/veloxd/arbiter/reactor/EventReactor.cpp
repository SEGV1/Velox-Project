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

#include "veloxd/arbiter/EventReactor.h"

#include <algorithm>
#include <vector>

#include "veloxd/Logging.h"
#include "veloxd/arbiter/IoHandle.h"
#include "veloxd/arbiter/reactor/SysPoller.h"

using std::shared_ptr;
using std::vector;

struct EventReactor::Impl
{
    bool stopflag = false;

    std::unique_ptr<SysPoller> poller;
    static const int pollTimeoutMs = -1; // 1ms
    std::vector<IoHandle*> channels;
    std::vector<IoHandle*> activeChannels;

    bool handingEvents = false;
};

EventReactor::EventReactor()
{
    _pImpl.reset(new EventReactor::Impl);
    _pImpl->poller.reset(new SysPoller);
}

EventReactor::~EventReactor() = default;

void
EventReactor::addChannel(IoHandle* c)
{
    VELOXD_LOG(info) << "Adding a IoHandle";

    if (!this->hasChannel(c)) {
        _pImpl->poller->addChannel(c);
        _pImpl->channels.push_back(c);
    } else {
        VELOXD_LOG(error)
            << "Trying to add a Channel already in this EventReactor";
    }
}

void
EventReactor::removeChannel(IoHandle* c)
{
    VELOXD_LOG(debug) << "Removing a IoHandle";

    if (this->hasChannel(c)) {
        _pImpl->poller->removeChannel(c);

        auto& channels = _pImpl->channels;
        auto it = std::find(channels.begin(), channels.end(), c);
        _pImpl->channels.erase(it);

    } else {
        VELOXD_LOG(error)
            << "Trying to remove a Channel not in this EventReactor";
    }
}

void
EventReactor::updateChannel(IoHandle* c)
{
    VELOXD_LOG(debug) << "Modifying a IoHandle";

    if (this->hasChannel(c)) {
        _pImpl->poller->updateChannel(c);
    } else {
        VELOXD_LOG(error)
            << "Trying to update a Channel not in this EventReactor";
    }
}

bool
EventReactor::hasChannel(IoHandle* c)
{
    auto it = std::find(_pImpl->channels.cbegin(), _pImpl->channels.cend(), c);
    if (it != _pImpl->channels.cend()) {
        return true;
    } else {
        return false;
    }
}

void
EventReactor::loop()
{
    _pImpl->stopflag = false;
    while (!_pImpl->stopflag) {
        auto actives = _pImpl->activeChannels;
        actives.clear();
        _pImpl->poller->poll(_pImpl->pollTimeoutMs, actives);

        _pImpl->handingEvents = true;
        for (const auto& c : actives) {
            c->handleEventsReceived();
        }
        _pImpl->handingEvents = false;
    }
}

void
EventReactor::stop()
{
    _pImpl->stopflag = true;
}
