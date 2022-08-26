//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

// https://github.com/mainroach/sandbox/blob/master/chrome-event-trace/event_trace_plat.h

#include <ibis/util/platform.hpp>

#if defined(IBIS_BUILD_PLATFORM_WINDOWS)
#include <ibis/event_trace/detail/platform_windows.hpp>
#elif defined(IBIS_BUILD_PLATFORM_LINUX)
#include <ibis/event_trace/detail/platform_linux.hpp>
#else
#error "No supported platform"
#endif

namespace ibis::tool::event_trace {

// ----------------------------------------------------------------------------
// Process info
// ----------------------------------------------------------------------------
namespace current_proc {

using id_type = detail::pid_type;

inline id_type id() { return detail::current_proc_id(); }

// Note, the concrete value isn't important since it serves as marker on JSON
inline constexpr id_type UNKNOWN = 0;

}  // namespace current_proc

// ----------------------------------------------------------------------------
// Thread info
// ----------------------------------------------------------------------------
// FixMe: To gather the thread id the C++ std::thread API doesn't work here since an object is
// required, see [std::thread](https://en.cppreference.com/w/cpp/thread/thread). Hence
// Posix Threads API is used on non Windows and hope that we are still portable.
namespace current_thread {

using id_type = detail::tid_type;

inline id_type id() { return detail::current_thread_id(); }

// Note, the concrete value isn't important since it serves as marker on JSON
inline constexpr id_type UNKNOWN = 0;

}  // namespace current_thread

}  // namespace ibis::tool::event_trace
