//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <ibis/event_trace/event_trace.hpp>
#include <ibis/event_trace/trace_log.hpp>

#include <array>
#include <string_view>

#include <iostream>
#include <csignal>

namespace ibis::tool::event_trace {

//
// topping top level class for writing logs into file
//

topping::topping() { register_termination_handler(); }

void topping::init(std::string const& json_filename)
{
    sink = std::make_unique<file_sink>(json_filename);
    TraceLog::GetInstance().BeginLogging();
}

topping::~topping()
{
    TraceLog::GetInstance().Flush();
    TraceLog::GetInstance().EndLogging();
}

void topping::termination_handler([[maybe_unused]] int signum)
{
    // FixMe: instance()->is_tracing()
    // if (is_tracing) {
    std::cerr << "Ctrl-C detected! Flushing trace.\n";

    TraceLog::GetInstance().Flush();

    // Note: no termination here, it's not this responsibility!
}

void topping::register_termination_handler()
{
    // Check on 'ignored' SIGINT signal and reenable it to avoid altering while registering.

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    if (std::signal(SIGINT, &topping::termination_handler) == SIG_IGN) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
        std::signal(SIGINT, SIG_IGN);
    }
}

void topping::OutputCallback(std::string_view json_str)
{
    if (sink == nullptr) {
        std::cout << "***WARNING***: Intent to write on closed file handle: '" << json_str << "'\n";
        return;
    }

    if constexpr (false) {
        // determine size of string buffer for TraceLog::Flush()
        std::size_t const sz = json_str.size() * sizeof(std::string::value_type);
        static std::size_t sz_max;
        sz_max = std::max(sz, sz_max);
        std::cout << "[write just " << sz << " bytes out (max seen = " << sz_max << " bytes)]\n";
    }

    sink->write(json_str);
}

topping::file_sink::file_sink(std::string const& json_filename)
    : ostream{ json_filename }
{
}

topping::file_sink::~file_sink()
{
    std::cerr << "Shutting down tracing. Flush events.\n";
    TraceLog::GetInstance().Flush();
    ostream.flush();
}

}  // namespace ibis::tool::event_trace
