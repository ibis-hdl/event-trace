//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <windows.h>
#include <cstdint>

namespace ibis::tool::event_trace::detail {

// ----------------------------------------------------------------------------
// Process info
// ----------------------------------------------------------------------------

// The result of the call is constant during runtime / life time of the instance, nevertheless 
// threads etc are used. Threads on Windows are first class citizens. So threads are not 
// mimicked as some sort of process.
//
// See:
// - [GetCurrentProcessId function (processthreadsapi.h)](
//    https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getcurrentprocessid)
//
using pid_type = int32_t;  // DWORD
inline pid_type current_proc_id() { return GetCurrentProcessId(); }

// ----------------------------------------------------------------------------
// Thread info
// ----------------------------------------------------------------------------
// FixMe: To gather the thread id the C++ std::thread API doesn't work here since an object is
// required, see [std::thread](https://en.cppreference.com/w/cpp/thread/thread). Hence
// Posix Threads API is used on non Windows and hope that will be still portable.
//
// see:
// - [GetCurrentThreadId function (processthreadsapi.h)](
//    https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getcurrentthreadid)
// - [How large is a DWORD with 32- and 64-bit code?](
//    https://stackoverflow.com/questions/39419/how-large-is-a-dword-with-32-and-64-bit-code)
using tid_type = int32_t;  // DWORD
inline tid_type current_thread_id() { return GetCurrentThreadId(); }

}  // namespace ibis::tool::event_trace::detail
