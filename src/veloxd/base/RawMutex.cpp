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

#include "veloxd/base/RawMutex.h"

RawMutex::RawMutex()
    : _holder(0)
{
    pthread_mutex_init(&_mutex, NULL);
}

RawMutex::~RawMutex()
{
    pthread_mutex_destroy(&_mutex);
}

void
RawMutex::lock()
{
    pthread_mutex_lock(&_mutex);
}

void
RawMutex::tryLock()
{
    pthread_mutex_trylock(&_mutex);
}

void
RawMutex::unlock()
{
    pthread_mutex_unlock(&_mutex);
}

bool
RawMutex::isLockedByThisThread()
{
    return pthread_self() == _holder;
}

MutexScope::MutexScope(RawMutex& lock)
    : _mutex(lock)
{
    _mutex.lock();
}

MutexScope::~MutexScope()
{
    _mutex.unlock();
}
