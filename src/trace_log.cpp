//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <ibis/event_trace/trace_log.hpp>
#include <ibis/event_trace/category.hpp>
#include <ibis/event_trace/trace_event.hpp>
#include <ibis/event_trace/event_trace.hpp>

#include <ibis/event_trace/detail/clock.hpp>
#include <ibis/event_trace/detail/platform.hpp>

#include <fmt/format.h>

#include <cassert>
#include <algorithm>
#include <string_view>
#include <iostream>

namespace /* anonymous */ {

// Flag to indicate whether we captured the current thread before
static thread_local bool current_thread_id_captured;

template <typename Arg, typename... Args>
void dbg_print(Arg&& arg, Args&&... args)
{
    if constexpr ((false)) { // NOLINT(readability-simplify-boolean-expr)
        std::cout << std::forward<Arg>(arg);
        ((std::cout << std::forward<Args>(args)), ...);
    }
}

}  // namespace

namespace ibis::tool::event_trace {

//
// TraceLog
//

TraceLog::TraceLog()
    : process_id_{ current_proc::id() }
    , process_id_hash_{ std::hash<current_proc::id_type>()(process_id_) }
    , enabled_{ false }
{
    // both requires reserve(), see https://coliru.stacked-crooked.com/a/bb1b21a7696f1249
    logged_events_.reserve(TraceLog::BUFFER_SZ);
    flush_events_.reserve(TraceLog::BUFFER_SZ);
}

void TraceLog::SetProcessID(current_proc::id_type process_id)
{
    process_id_ = process_id;
    process_id_hash_ = std::hash<current_proc::id_type>()(process_id);
}

void TraceLog::SetEnabled(bool enabled)
{
    if (enabled) {
        // FixMe: category enable all ??? check comments inside chrome's original
        // SetEnabled(std::vector<std::string>(), std::vector<std::string>());
        enabled_ = true;
    }
    else {
        std::scoped_lock scoped_lock(lock_);

        if (!enabled_) {
            return;
        }

        enabled_ = false;

        // FixMe:
        // - category clear_all()
        // - set disabled "flags"
#if 0
        included_categories_.clear();
        excluded_categories_.clear();

        for (int i = 0; i < g_category_index; i++) {
            g_category_enabled[i] = 0;
        }
#endif
#if 0
        AddThreadNameMetadataEvents(); // requires trace_event args as 'M' (metadata)
        //AddClockSyncMetadataEvents(); // only android
#endif
    }

    Flush();
}

float TraceLog::GetEventBufferPercentFull() const
{
    return static_cast<float>(logged_events_.size()) / static_cast<float>(TraceLog::BUFFER_SZ);
}

std::int32_t TraceLog::AddTraceEventInternal(                           // --
    TraceEvent::phase phase,                                            // --
    std::string_view category_name, std::string_view event_name,        // --
    std::uint64_t trace_id, TraceEvent::flag flags,                     // --
    std::int32_t threshold_begin_id, clock::duration_type threshold,    // --
    TraceEvent::storage_ptr ptr,                                        // --
    std::string_view arg1_name, trace_value arg1_value                  // --
    )
{
    assert(category_name.size() > 0 && "category_name must not be empty");
    assert(event_name.size() > 0 && "event_name must not be empty");

    if (logged_events_.size() >= TraceLog::BUFFER_SZ) {
        return TraceLog::EVENT_ID_NONE;
    }

    std::scoped_lock scoped_lock(lock_);

    current_thread::id_type const thread_id = current_thread::id();

    // don't move the time capture point, so it's independet of the code path below
    auto const time_point = clock::time<>::now();

    // record the name of the calling thread, if not done already.
    if (!current_thread_id_captured) {
        std::cout << "Debug: Thread ID " << thread_id << " just initialized TLS\n";
        auto const iter = std::find(thread_ids_seen.begin(), thread_ids_seen.end(), thread_id);

        if (iter == thread_ids_seen.end()) {
            std::cout << "Debug: Add Thread ID " << thread_id << "\n";
            thread_ids_seen.emplace_back(thread_id);
        }
        else {
            // This is a thread id that's seen before, but potentially with a
            // new name.
            // std::cout << "Debug: Thread ID seen, but TLS already initialized.\n";
        }
        // FixMe: Chromium's event_trace hasn't this!
        current_thread_id_captured = true;
    }

    // Check on threshold duration events, only record the event if the duration is greater than
    // the specified threshold duration.
    if (threshold_begin_id > TraceLog::EVENT_ID_NONE) {
        assert(phase == TraceEvent::phase::END);

        // the event ID where the scope started
        auto const event_id = static_cast<std::size_t>(threshold_begin_id);

        dbg_print("TraceEndOnScopeCloseThreshold '", category_name, "/", event_name, "':\n");

        // <begin event> has been flushed out before, discard <end event> since it's
        // unreachable
        // --------------------------------------------------------------------------
        // FixMe: CheckMe: Since we swap and flush vectors, vector's size() maybe not
        // appropriate!! Consider global, run- and lifetime ID (static counter increment)
        // --------------------------------------------------------------------------
        if (event_id >= logged_events_.size()) {
            dbg_print("  => discard event: not anymore logged, probably flushed before.\n");
            return TraceLog::EVENT_ID_NONE;
        }

        // Determine whether to drop the begin/end pair.
        auto const elapsed = time_point - logged_events_[event_id].timestamp();

        dbg_print(
            "  elapsed = ", std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count(),
            "ns, threshold = ", threshold.count(), "ns\n");

        if (elapsed < threshold) {
            // Remove <begin event> and do not add <end event>.
            // This will be expensive if there have been other events in the
            // mean time (should be rare).
            // --------------------------------------------------------------------------
            // FixMe: This invalidates all other event_id (even iterators, which isn't
            // problematic here)! This may affect other TraceEndOnScopeCloseThreshold events.
            // Maybe extend the flag e.g. with 0xFF = ORPHANED
            // --------------------------------------------------------------------------
            dbg_print("  => discard event\n");
            using difference_type =
                typename std::iterator_traits<std::vector<TraceEvent>::iterator>::difference_type;
            logged_events_.erase(logged_events_.begin() + static_cast<difference_type>(event_id));
            return TraceLog::EVENT_ID_NONE;
        }

        dbg_print("  => log event\n");
    }

    if ((flags & TraceEvent::flag::MANGLE_ID) != 0) {
        trace_id ^= process_id_hash_;
    }

    // use the current length of traced events as ID of event
    std::int32_t const event_id = static_cast<std::int32_t>(logged_events_.size());

    logged_events_.emplace_back(           // TraceEvent(...)
        thread_id, time_point,             // --
        phase, category_name, event_name,  // --
        trace_id, flags,                   // --
        std::move(ptr),                    // --
        arg1_name, arg1_value              // --
        );

    return event_id;
}

void TraceLog::Flush()
{
    {
        std::scoped_lock scoped_lock(lock_);
        flush_events_.swap(logged_events_);
        logged_events_.clear();

        // ensure, that clearing data doesn't resize memory capacity unintentionally.
        // assert(logged_events_.capacity() == BUFFER_SZ);
    }

    // FixMe: [C++20] using move constructor with reserved memory allows to use this pre-allocated
    // memory, see https://coliru.stacked-crooked.com/a/eaf6b311418f131e; maybe custom allocator is
    // the only reasonable-ish way of doing it. For making a custom allocator I'd probably have
    // std::allocator as a member and go through and implement all the needed functions keeping note
    // of the last requested allocation. A more idealistic implementation would be to have a class
    // template for the allocator    taking another allocator type as template parameter and storing
    // that allocator as a member that would be like a dependency injected custom allocator which is
    // good for composition which works into the Alexandrescu talk
    auto json_str = std::string();
    json_str.reserve(4096);  // FixMe: Check the size value

    auto const AppendEventsAsJSON = [](std::vector<TraceEvent> const& events, std::size_t start,
                                       std::size_t count, std::string& out) {
        for (std::size_t i = 0; i < count && (start + i) < events.size(); ++i) {
            events[i + start].AppendAsJSON(out);
        }
    };

    for (std::size_t i = 0; i < flush_events_.size(); i += TraceLog::BATCH_SZ) {
        json_str.clear();
        AppendEventsAsJSON(flush_events_, i, TraceLog::BATCH_SZ, json_str);
        output_callback(json_str);
    }
}

void TraceLog::BeginLogging()
{
    SetEnabled(true);

    auto buf = fmt::memory_buffer();
    auto back_inserter = std::back_inserter(buf);

    fmt::format_to(back_inserter, R"({{"traceEvents":[)" "\n");
    output_callback(to_string(buf));
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void TraceLog::EndLogging()  // -- maybe later static
{
    auto buf = fmt::memory_buffer();
    auto back_inserter = std::back_inserter(buf);

    fmt::format_to(back_inserter, R"(],"displayTimeUnit":"ns"}})" "\n");
    output_callback(to_string(buf)); 
}

void TraceLog::AddThreadNameMetadataEvents()
{
    for (auto const id : thread_ids_seen) {
        // buffer's worst case scenario: thread ID is of uint64, hence log10(2^64) ~ 20 digits.
        // With leading string 'thread-' (7 bytes) and '\0' at least 28 bytes are required.
        static constexpr std::size_t alloc_size = 32;
        static_assert(sizeof(decltype(id)) <= sizeof(std::uint64_t),
                      "string buffer to small for thread ID type!");

        // We could invoke TraceLog::copy() API to make a deep copy, but we shorten it to:

        // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
        TraceEvent::storage_ptr ptr = std::make_unique<char[]>(alloc_size);

        // TODO [C++20] use case for std::format
        // see https://stackoverflow.com/questions/60450200/forcing-format-to-n-to-use-terminating-zero
        auto constexpr snprintf_fmt = []() {  // quiet warnings
            if constexpr (std::is_same_v<current_thread::id_type, std::int32_t>) {
                return "thread-%d";  // e.g. Win32 DWORD (int32_t)
            }
            else {
                return "thread-%lu";  // long unsigned, e.g. Linux
            }
        }();
        auto const n = snprintf(ptr.get(), alloc_size - 1, snprintf_fmt, id);
        assert(!(n < 0));         // error must not occur here
        assert(n != alloc_size);  // truncated to limit, means buffer to small
        auto const count = static_cast<std::size_t>(n);
        ptr[count] = '\0';

        logged_events_.emplace_back(                     // TraceEvent(...)
            id, clock::time<>::now(),                    // -- thead_id, time point
            TraceEvent::phase::METADATA,                 // -- phase
            "__metadata", "thread_name",                 // -- category, event name
            0, TraceEvent::flag::NONE,                   // -- id, flags
            std::move(ptr),                              // --
            "name", std::string_view(ptr.get(), count)   // -- argument { key : value }
            );
    }
}

#if 0  // old way
void TraceLog::OutputCallback(std::string_view out)
{
    output_callback(out);

    // FixMe: static member accessed through instance [readability-static-accessed-through-instance]
    // but fixing:
    // topping::OutputCallback(out);
    // triggers: AddressSanitizer: heap-use-after-free in TraceLog::Flush()

    // clang-format off
    /** -----------------------------------------------------------------------
     ==56917==ERROR: AddressSanitizer: heap-use-after-free on address 0x7fe878f2d860 at pc 0x0000006c0cb7 bp 0x7ffe1bc0a7b0 sp 0x7ffe1bc0a7a8
    READ of size 8 at 0x7fe878f2d860 thread T0
    #0 0x6c0cb6 in TraceLog::Flush() (/home/olaf/work/CXX/ibis/source_instrumentation/build/bin/ibis+0x6c0cb6)
    #1 0x6c3c35 in topping::file_sink::~file_sink() (/home/olaf/work/CXX/ibis/source_instrumentation/build/bin/ibis+0x6c3c35)
    #2 0x533e1c in std::default_delete<topping::file_sink>::operator()(topping::file_sink*) const /usr/lib/gcc/x86_64-redhat-linux/10/../../../../include/c++/10/bits/unique_ptr.h:85:2
    #3 0x533e1c in std::unique_ptr<topping::file_sink, std::default_delete<topping::file_sink> >::~unique_ptr() /usr/lib/gcc/x86_64-redhat-linux/10/../../../../include/c++/10/bits/unique_ptr.h:361:4
    #4 0x7fe884fff236 in __run_exit_handlers (/lib64/libc.so.6+0x40236)
    #5 0x7fe884fff3df in exit (/lib64/libc.so.6+0x403df)
    #6 0x7fe884fe71e8 in __libc_start_main (/lib64/libc.so.6+0x281e8)
    #7 0x45488d in _start (/home/olaf/work/CXX/ibis/source_instrumentation/build/bin/ibis+0x45488d)

    0x7fe878f2d860 is located 96 bytes inside of 56000000-byte region [0x7fe878f2d800,0x7fe87c495600)
    freed by thread T0 here:
    #0 0x52d977 in operator delete(void*) (/home/olaf/work/CXX/ibis/source_instrumentation/build/bin/ibis+0x52d977)
    #1 0x541dbc in __gnu_cxx::new_allocator<TraceEvent>::deallocate(TraceEvent*, unsigned long) /usr/lib/gcc/x86_64-redhat-linux/10/../../../../include/c++/10/ext/new_allocator.h:133:2
    #2 0x541dbc in std::allocator_traits<std::allocator<TraceEvent> >::deallocate(std::allocator<TraceEvent>&, TraceEvent*, unsigned long) /usr/lib/gcc/x86_64-redhat-linux/10/../../../../include/c++/10/bits/alloc_traits.h:492:13
    #3 0x541dbc in std::_Vector_base<TraceEvent, std::allocator<TraceEvent> >::_M_deallocate(TraceEvent*, unsigned long) /usr/lib/gcc/x86_64-redhat-linux/10/../../../../include/c++/10/bits/stl_vector.h:354:4
    #4 0x541dbc in std::_Vector_base<TraceEvent, std::allocator<TraceEvent> >::~_Vector_base() /usr/lib/gcc/x86_64-redhat-linux/10/../../../../include/c++/10/bits/stl_vector.h:335:2
    #5 0x541dbc in std::vector<TraceEvent, std::allocator<TraceEvent> >::~vector() /usr/lib/gcc/x86_64-redhat-linux/10/../../../../include/c++/10/bits/stl_vector.h:683:7
    #6 0x541dbc in TraceLog::~TraceLog() /home/olaf/work/CXX/ibis/source_instrumentation/build/../source/common/include/ibis/event_trace/trace_log.hpp:11:7
    #7 0x7fe884fff236 in __run_exit_handlers (/lib64/libc.so.6+0x40236)
    ** ----------------------------------------------------------------------- */
    // clang-format on

    //topping::instance().OutputCallback(out);
}
#endif

void TraceLog::BufferFullCallback() { TraceLog::GetInstance().Flush(); }

}  // namespace ibis::tool::event_trace
