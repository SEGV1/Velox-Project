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

#ifndef VELOXD_MUX_EVENTREACTOR_H
#define VELOXD_MUX_EVENTREACTOR_H

#include <memory>

class IoHandle;

/** Enhanced Reactor pattern (node.js)
 *
 * The EventReactor module exposes two concepts: EventReactor and IoHandle
 *
 * Notes:
 *  - The author dislikes the automated feature where IoHandle can register
 *     itself into an EventReactor, so all Channels in Veloxd must explicitly specify
 *     an EventReactor when registering.
 *
 *  - However, IoHandle does have the automated feature of unregistering
 *     itself from the EventReactor upon destruction, and also of automatically notifying
 *     the EventReactor when its interested events change.
 *
 *  - EventReactor is not responsible for the lifecycle of IoHandle
 */
class EventReactor
{
public:
    EventReactor();
    ~EventReactor();
    // Non-copyable, Non-moveable
    EventReactor(const EventReactor&) = delete;
    EventReactor& operator=(const EventReactor&) = delete;
    EventReactor(EventReactor&&) = delete;
    EventReactor& operator=(EventReactor&&) = delete;

    void addChannel(IoHandle*);
    void removeChannel(IoHandle*);
    void updateChannel(IoHandle*);
    bool hasChannel(IoHandle*);

    void loop();
    void stop();

private:
    struct Impl;
    std::unique_ptr<Impl> _pImpl;
};

#endif
