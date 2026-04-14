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

#ifndef VELOXD_BASE_RAWMUTEX_H
#define VELOXD_BASE_RAWMUTEX_H

#include <pthread.h>

class RawMutex
{
public:
    RawMutex();
    ~RawMutex();
    // Non-copyable, Non-moveable
    RawMutex(const RawMutex&) = delete;
    RawMutex& operator=(const RawMutex&) = delete;
    RawMutex(RawMutex&&) = delete;
    RawMutex& operator=(RawMutex&&) = delete;

    void lock();
    void tryLock();
    void unlock();
    bool isLockedByThisThread();

private:
    pthread_mutex_t _mutex;
    pthread_t _holder;
};

/** MutexScope object can automatically release mutex at it's end of
 *  lifetime
 *
 * Especially useful when an expection occur.
 */
class MutexScope
{
public:
    MutexScope(RawMutex&);
    ~MutexScope();

private:
    RawMutex& _mutex;
};

#endif // VELOXD_BASE_RAWMUTEX_H
