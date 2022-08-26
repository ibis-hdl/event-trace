//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <sys/types.h>

#include <iostream>

extern "C" {

pid_t getpid() {
    // std::cout << "mock getpid()\n";
    return 1000;
}

}
