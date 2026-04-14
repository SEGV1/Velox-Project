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

#ifndef VELOXD_MUX_IOHANDLE_H
#define VELOXD_MUX_IOHANDLE_H

#include <functional>
#include <memory>

class EventReactor;

class IoHandle
{
public:
    IoHandle(int fd);
    ~IoHandle();
    // Non-copyable, Non-moveable
    IoHandle(const IoHandle&) = delete;
    IoHandle& operator=(const IoHandle&) = delete;
    IoHandle(IoHandle&&) = delete;
    IoHandle& operator=(IoHandle&&) = delete;

    void setOnReadCB(std::function<void()> callback);
    void setOnWriteCB(std::function<void()> callback);
    void setOnCloseCB(std::function<void()> callback);
    void setOnErrorCB(std::function<void()> callback);

    // For IoHandle creator use
    void enableReading();
    void disableReading();
    void enableWriting();
    void disableWriting();
    void disableAll();

    // For who control the EventReactor use
    void setOwnerEventReactor(EventReactor*);
    void unregisterSelf();

    // For EventReactor.poller use
    int eventsInterested();
    void setEventsReceived(int revt);
    void handleEventsReceived();
    int fd();

private:
    struct Impl;
    std::unique_ptr<Impl> _pImpl;
};

#endif // VELOXD_MUX_IOHANDLE_H
