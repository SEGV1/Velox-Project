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

#ifndef VELOXD_CONF_H
#define VELOXD_CONF_H

#include "veloxd/base/Singleton.h"

#include <string>
using std::string;

/** Singleton.
 */
struct Conf : public Singleton<Conf>
{
    friend class Singleton<Conf>;

    struct Logging
    {
        string level = "debug"; // debug, info, warning, error, fatal
        bool to_stdout = false;
        string dir = "/var/log/veloxd";
    } logging;

    struct Tun
    {
        string name = "";
        string addr_cidr = "192.168.168.8/24";
    } tundev;

    struct Stack
    {
        string addr = "192.168.168.10";
    } stack;

    string conf_file = "/etc/veloxd/veloxd.conf";

private:
    Conf() = default;
};

#endif // VELOXD_CONF_H
