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

#ifndef VELOXD_MUX_SYSPOLLER_H
#define VELOXD_MUX_SYSPOLLER_H

#include <memory>
#include <vector>

class IoHandle;

class SysPoller
{
public:
    SysPoller();
    ~SysPoller();
    // Non-copyable, Non-moveable
    SysPoller(const SysPoller&) = delete;
    SysPoller& operator=(const SysPoller&) = delete;
    SysPoller(SysPoller&&) = delete;
    SysPoller& operator=(SysPoller&&) = delete;

    void addChannel(IoHandle*);
    void removeChannel(IoHandle*);
    void updateChannel(IoHandle*);

    void poll(int timeoutMs,
              std::vector<IoHandle*>& activeChannelList);

private:
    struct Impl;
    std::unique_ptr<Impl> _pImpl;
};

#endif // VELOXD_MUX_SYSPOLLER_H
