//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <ibis/event_trace/detail/trace_value.hpp>
#include <ibis/event_trace/detail/clock.hpp>
#include <ibis/event_trace/detail/platform.hpp>

#include <string_view>
#include <vector>
#include <memory>
#include <array>
#include <type_traits>

#include <atomic>
#include <random>

namespace ibis::tool::event_trace {

#if !defined(IBIS_TRACE_EVENT_MAX_ARGS)
#define IBIS_TRACE_EVENT_MAX_ARGS 4U
#endif

// TraceEvent's argument constraints
template<typename T> concept ArgNameT = std::convertible_to<T, std::string_view>;
template<typename T> concept ArgValueT = std::convertible_to<T, trace_value>;
template<typename... T> concept CountArgT = requires(T... args) { // concept: https://godbolt.org/z/dEGsd781K
    // count must be even due to name/value pared args
    requires (sizeof...(T)) % 2 == 0;
    // the count is limited by arg count's absolute size
    requires (sizeof...(T)) / (2*IBIS_TRACE_EVENT_MAX_ARGS+1) == 0;
};

///
/// The trace event to be stored.
///
class TraceEvent {
public:
    /// Phase indicates the nature of an event entry. E.g. part of a begin/end pair.
    enum phase : char {
        UNSPECIFIED = '?',
        BEGIN = 'B',        ///< Duration event begin.
        END = 'E',          ///< Duration event end.
        COMPLETE = 'X',     ///< Complete events with duration.
        INSTANT = 'I',      ///< Instant event.
        ASYNC_BEGIN = 'S',  ///< start (deprecated)
        ASYNC_STEP = 'T',   ///< step (deprecated)
        ASYNC_END = 'F',    ///< finish (deprecated)
        METADATA = 'M',     ///< Metadata event.
        COUNTER = 'C'       ///< Counter event.
    };

    /// Flags used for several purposes.
    enum flag : uint8_t {
        NONE = 0,
        HAS_ID = 1U << 0U,
        MANGLE_ID = 1U << 1U,
    };

public:
    //NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
    using storage_ptr = std::unique_ptr<char[]>;

public:
    TraceEvent() = delete;
    TraceEvent(TraceEvent const&) = delete;
    TraceEvent& operator=(TraceEvent const&) = delete;

    TraceEvent(TraceEvent&&) = default;
    TraceEvent& operator=(TraceEvent&&) = default;

    ~TraceEvent() = default;

public:
    TraceEvent(current_thread::id_type thread_id, clock::time_point_type timestamp,
               TraceEvent::phase phase,                                      // --
               std::string_view category_name, std::string_view event_name,  // --
               std::uint64_t trace_id, TraceEvent::flag flags_,              // --
               storage_ptr ptr,                                              //
               std::string_view arg1_name, trace_value arg1_value
    )
    : category_name_(category_name.data())
    , event_name_(event_name.data())
    //, arg_names(arg1_name)
    //, arg_values(arg1_value)
    , thread_id_(thread_id)
    , timestamp_(timestamp)
    , trace_id_(trace_id)
    , copy_storage(std::move(ptr))
    , phase_(phase)
    , flags(flags_)
    {
        // check on correct terminated string literals since storage is using C strings
        assert(strlen(category_name_) == category_name.size() && "unexpected strlen for category_name");
        assert(strlen(event_name_) == event_name.size() && "unexpected strlen for event_name");

        // FIXME compile fix during transition to new API
        arg_names[0] = arg1_name.data();
        arg_values[0] = arg1_value;
        // FIXME this must be variadic! also, this is bullshit!
        if (arg_names[0] != nullptr) {
            assert(strlen(arg_names[0]) == arg1_name.size() && "unexpected strlen for arg_name");
        }
    }

public:
    // Serialize event data to JSON
    void AppendAsJSON(std::string& out) const;

public:
    clock::time_point_type timestamp() const { return timestamp_; }

    std::string_view name() const { return event_name_; }

public:
    static constexpr std::size_t const ARGS_SZ = IBIS_TRACE_EVENT_MAX_ARGS;

private:
    // Note: use of raw char pointer to save space, sizeof(std::string_view{}) > sizeof(char const*),
    // e.g. 16 vs. 8 bytes.
    using string_type = char const*;

private:
    // these are ordered by size (largest first) for optimal aligned storage.
    std::array<trace_value, ARGS_SZ> arg_values;   // MAX_ARGS * 16 bytes
    std::array<string_type, ARGS_SZ> arg_names = { nullptr };    // MAX_ARGS * 8 bytes

    string_type category_name_;  // 8 bytes
    string_type event_name_;     // 8 bytes

    current_thread::id_type thread_id_ = current_thread::UNKNOWN;   // 8 bytes
    clock::time_point_type timestamp_ = clock::time_point_zero;     // 8 bytes
    std::uint64_t trace_id_ = 0;                                    // 8 bytes

    storage_ptr copy_storage = nullptr;  // 8 bytes

    TraceEvent::phase phase_ = TraceEvent::phase::UNSPECIFIED;  // 1 byte
    TraceEvent::flag flags = TraceEvent::flag::NONE;            // 1 byte
};

}  // namespace ibis::tool::event_trace

//
// Implementation
//

namespace ibis::tool::event_trace {

inline TraceEvent::flag operator|(TraceEvent::flag lhs, TraceEvent::flag rhs)
{
    return static_cast<TraceEvent::flag>(  // --
        static_cast<unsigned>(lhs) | static_cast<unsigned>(rhs));
}

inline TraceEvent::flag& operator|=(TraceEvent::flag& lhs, TraceEvent::flag rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

}  // namespace ibis::tool::event_trace
