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
