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

#ifndef VELOXD_VELOXD_H
#define VELOXD_VELOXD_H

#include <memory>

#include "veloxd/base/Singleton.h"

class Veloxd : public Singleton<Veloxd>
{
    friend class Singleton<Veloxd>;

public:
    enum class exit_status
    {
        SUCCESS = 0,
        FAILURE = 1,
        BAD_CMDLINE,
        BAD_CONF_FILE,
    };

public:
    void init(int argc, char* argv[]);
    void run();
    void stop();
    void exit(enum exit_status);

private:
    Veloxd();
    ~Veloxd();

    void readConf(int argc, char* argv[]);
    void initSignalActions();
    void initLogging();

private:
    struct Impl;
    std::unique_ptr<Impl> _pImpl;
};

#endif // VELOXD_VELOXD_H
