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

#include "ShellOp.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "veloxd/Logging.h"

ShellOp::ShellOp(const char* cmd, ...)
{
    va_list ap;

    va_start(ap, cmd);
    vsnprintf(_buf, 128, cmd, ap);
    va_end(ap);
}

void
ShellOp::run()
{
    // I don't know whether logger will make a deep copy of `char*' msg,
    // so, for safty, I make it a `string' which doesn't has the problem
    // of shadow/deep copy
    VELOXD_LOG(info) << "Running command " << std::string(_buf);

    system(_buf);
}
