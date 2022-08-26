//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <pthread.h>

#include <iostream>

extern "C" {

pthread_t pthread_self()
{
    // std::cout << "mock pthread_self()\n";
    return 1000000;
}
}