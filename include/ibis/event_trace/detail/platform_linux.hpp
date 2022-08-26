//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

namespace ibis::tool::event_trace::detail {

// ----------------------------------------------------------------------------
// Process info
// ----------------------------------------------------------------------------
using pid_type = pid_t;
inline pid_type current_proc_id() { return getpid(); }

// ----------------------------------------------------------------------------
// Thread info
// ----------------------------------------------------------------------------
// FixMe: To gather the thread id the C++ std::thread API doesn't work here since an object is
// required, see [std::thread](https://en.cppreference.com/w/cpp/thread/thread). Hence
// Posix Threads API is used on non Windows and hope that we are still portable.
using tid_type = pthread_t;
inline tid_type current_thread_id() noexcept { return pthread_self(); }

}  // namespace ibis::tool::event_trace::detail
