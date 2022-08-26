//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <ibis/event_trace/trace_event.hpp>

#include <type_traits>

#include <atomic>
#include <random>

namespace ibis::tool::event_trace {

template<typename T>
concept TraceIdIntegralT = !std::is_same_v<bool, T> && std::is_integral_v<T>;

///
/// @brief TraceID encapsulates an ID that can either be an integer or pointer. Pointers  are
/// mangled with the Process ID so that they are unlikely to collide when the same pointer is used
/// on different processes.
//
// FIXME Is the (old) documentation still correct regards to mangling with process ID??
///
/// Concept: @see [Coliru](https://coliru.stacked-crooked.com/a/8ffbcf33de92f088)
///
class TraceID {
public:
    using value_type = std::uint64_t;

public:
    static constexpr value_type NONE = 0;

public:
    explicit TraceID(TraceIdIntegralT auto id, [[maybe_unused]] TraceEvent::flag& flags)
        : id_value(static_cast<value_type>(id))
    {
    }

    explicit TraceID(void const* ptr, TraceEvent::flag& id_flags)
        : id_value(static_cast<value_type>(reinterpret_cast<std::uintptr_t>(ptr)))
    {
        static_assert(sizeof(TraceID::value_type) == sizeof(std::uintptr_t),
                      "TraceID type can't hold sizeof pointer!");
        id_flags |= TraceEvent::flag::MANGLE_ID;
    }

    value_type value() const { return id_value; }

    /// construct an ID only for JSON, to allow overloading
    static TraceID as_TraceID(auto id) { return TraceID(id); }

    ///
    /// @brief Generate a per process random number biased counter value.
    ///
    /// @return TraceID::value_type an globally-unique id which can be used as a flow id or async
    /// event id.
    ///
    /// Use this to get an unique ID instead of implementing an own counter and hashing it with a
    /// random value. Nevertheless, consider using TRACE_ID_LOCAL(this) to avoid storing
    /// additional data if possible.
    ///
    static TraceID::value_type next_global()
    {
        static const value_type seed = []() {
            static std::mt19937_64 gen64;
            return gen64();
        }();
        static std::atomic<value_type> counter;
        std::atomic<value_type> id{ counter++ };
        return seed ^ id;
    }

private:
    explicit TraceID(value_type id)
        : id_value(id)
    {
    }

private:
    value_type const id_value;
};

}  // namespace ibis::tool::event_trace
