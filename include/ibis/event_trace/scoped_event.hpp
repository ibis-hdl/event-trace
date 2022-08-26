//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <ibis/event_trace/category.hpp>
#include <ibis/event_trace/trace_log.hpp>
#include <ibis/event_trace/trace_id.hpp>

#include <string_view>

namespace ibis::tool::event_trace {

///
/// Scope guard base.
///
/// Using CRTP to implement concrete scope guard implementation. On destruction it adds an
/// trace event to trace log.
/// On class' destruction the duration event 'end' is emitted.
///
/// Concept see [godbolt](https://godbolt.org/z/KGYcTYr8W)
///
template<typename DerivedT>
class scope_guard_base {

public:
    scope_guard_base(category::proxy proxy, std::string_view event_name_)
        : category_enabled(proxy)
        , event_name(event_name_)
    {}

    ~scope_guard_base() noexcept {
        if (category_enabled) {
            try {
                // e.g. TraceLog's emplace() of vector<TraveEvent> may throw
                add_event();
            }
            catch(std::exception const& e) {
                std::cerr << "ATTENTION: ~scope_guard() caught: '" << e.what() << "'\n";
            }
            catch(...) {
                std::cerr << "ATTENTION: ~scope_guard() caught: 'Unexpected exception'\n";
            }
        }
    }

public:
    scope_guard_base() = delete;
    scope_guard_base(scope_guard_base const&) = delete;
    scope_guard_base& operator=(scope_guard_base const&) = delete;
    scope_guard_base(scope_guard_base&&) = delete;
    scope_guard_base& operator=(scope_guard_base&&) = delete;

private:
    void add_event() const {
        auto const& derived = static_cast<DerivedT const&>(*this);
        // NextGen: always without args API (and order changed)
        TraceLog::GetInstance().AddTraceEventInternal(              // --
            TraceEvent::phase::END,                                 // --
            category_enabled.category_name(), event_name,           // --
            TraceID::NONE, TraceEvent::flag::NONE,                  // --
            derived.threshold_begin_id, derived.threshold,          // --
            nullptr,                                                // --
            std::string_view{}, trace_value{}                       // -- no arguments
            );
    }

private:
    category::proxy const category_enabled;
    std::string_view const event_name;
};

///
/// Simple scope guard, acts as common API for @ref scope_guard_base, intended to be used
/// with macros.
///
/// example usage:
/// @code{.cpp}
/// static auto const category_proxy__ = category::get("category_name");
/// auto const scope__ = scope_guard{ category_proxy__, "event_name" };
/// if (category_proxy__) {
///     AddTraceEvent(TraceEvent::phase::BEGIN,
///                   category_proxy__.category_name(), "event_name",
///                   TraceID::NONE, TraceEvent::flag::NONE, ...);
/// }
/// @endcode
///
class scope_guard : scope_guard_base<scope_guard>
{
    friend scope_guard_base;

public:
    scope_guard(category::proxy proxy, std::string_view event_name)
    : scope_guard_base::scope_guard_base(proxy, event_name)
    {}

private:
    std::int32_t const threshold_begin_id = TraceLog::EVENT_ID_NONE;
    clock::duration_type const threshold = clock::duration_zero;
};

///
/// Scope guard with duration threshold, acts as common API for @ref scope_guard_base; intended
/// to be used with macros.
///
/// example usage:
/// @code{.cpp}
/// static auto const category_proxy__ = category::get("category_name");
/// auto const scope__ = scope_guard{ category_proxy__, "event_name" };
/// if (category_proxy__) {
///     auto const id__ = AddTraceEvent(TraceEvent::phase::BEGIN,
///                   category_proxy__.category_name(), "event_name",
///                   TraceID::NONE, TraceEvent::flag::NONE, ...);
///     scope__.set_threshold_begin_id(id__);
/// }
/// @endcode
///
///
class scope_threshold_guard : scope_guard_base<scope_threshold_guard>
{
    friend scope_guard_base;

public:
    scope_threshold_guard(category::proxy proxy, std::string_view event_name, clock::duration_type threshold_)
    : scope_guard_base::scope_guard_base(proxy, event_name)
    , threshold{ threshold_ }
    {}

public:
    void set_threshold_begin_id(std::int32_t event_id) { threshold_begin_id = event_id; }

private:
    std::int32_t threshold_begin_id = TraceLog::EVENT_ID_NONE;
    clock::duration_type const threshold;
};

}  // namespace ibis::tool::event_trace
