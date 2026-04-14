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

#ifndef VELOXD_AGENTHUB_H
#define VELOXD_AGENTHUB_H

#include <functional>
#include <memory>

class IoHandle;

class AgentHub
{
public:
    AgentHub();
    ~AgentHub();
    // Non-copyable, Non-moveable
    AgentHub(const AgentHub&) = delete;
    AgentHub& operator=(const AgentHub&) = delete;
    AgentHub(AgentHub&&) = delete;
    AgentHub& operator=(AgentHub&&) = delete;

    void listen();
    // clang-format off
    void onNewConnection(
        const std::function
        <
         void(int sockfd)
        > &
    );
    // clang-format on

    // AgentHub is responsible for the lifecycle of IoHandle
    std::shared_ptr<IoHandle> getIoHandle();

private:
    struct Impl;
    std::unique_ptr<Impl> _pImpl;
};

#endif // VELOXD_AGENTHUB_H
