//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <ibis/event_trace/trace_event.hpp>
#include <ibis/event_trace/event_trace.hpp>  // FixMe what a named bullshit
#include <ibis/event_trace/detail/json_formatter.hpp>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <range/v3/view/zip.hpp>
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/iterator/stream_iterators.hpp>

#include <chrono>

namespace ibis::tool::event_trace {

void TraceEvent::AppendAsJSON(std::string& out) const
{
    using std::chrono::nanoseconds;
    using std::chrono::time_point_cast;
    namespace views = ranges::views;

    auto const newline = true; // JSON cosmetic flag

    // time_point_cast's return type is int64.
    std::int64_t const time_int64 =
        time_point_cast<nanoseconds>(timestamp_).time_since_epoch().count();
    current_proc::id_type const process_id = TraceLog::GetInstance().process_id();

    // {fmt} lib printing concept, see https://godbolt.org/z/vMY9W1T7z
    // note: fmt::memory_buffer's inline_buffer_size = 500 per default
    auto buf = fmt::basic_memory_buffer<char, 500>();
    auto back_inserter = std::back_inserter(buf);

    // clang-format off
    fmt::format_to(back_inserter, 
        R"({{"cat":"{}","pid":{},"tid":{},"ph":"{}","ts":{},"name":"{}")",
        category_name_,
        process_id,
        thread_id_,
        phase_,
        time_int64,
        event_name_
    );
    // clang-format on

    if(arg_names[0] != nullptr) { // one or more args, append "args" JSON object
        fmt::format_to(back_inserter, R"(,"args":{{)");
        auto comma = "";
        for(std::tuple<char const*, trace_value const&> arg : ranges::views::zip(arg_names, arg_values)) {
            if(std::get<0>(arg) == nullptr) { break; }
            fmt::format_to(back_inserter, R"({}"{}":{})", // --
                           comma, std::get<0>(arg), std::get<1>(arg));
            comma = ",";
        }
        fmt::format_to(back_inserter, R"(}})");
    }
    else { /* there are no args */ }
    
    if((flags & TraceEvent::flag::HAS_ID) != 0) {
        fmt::format_to(back_inserter, R"(,"id":{})", TraceID::as_TraceID(trace_id_));
    }
    
    fmt::format_to(back_inserter, "}},");
    
    if(newline) { fmt::format_to(back_inserter, "\n"); }

    out.append(buf.begin(), buf.end());
}

}  // namespace ibis::tool::event_trace
