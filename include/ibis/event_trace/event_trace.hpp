//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

// https://github.com/mainroach/sandbox/blob/master/chrome-event-trace/event_trace.h
// https://chromium.googlesource.com/chromium/src/+/master/base/trace_event/trace_event.h

#include <ibis/event_trace/trace_log.hpp>
#include <ibis/event_trace/category.hpp>
#include <ibis/event_trace/scoped_event.hpp>
#include <ibis/event_trace/trace_id.hpp>

#include <string_view>
#include <variant>
#include <optional>
#include <iostream>

#include <fstream>
#include <memory>

#if 0
// Concept Check/Test

static inline void foo(std::string_view sv) { std::cout << sv << '\n'; }

// https://wandbox.org/permlink/jZGuCwmRo5qhAPgJ
// https://stackoverflow.com/questions/17339789/how-to-call-a-function-on-all-variadic-template-args/17340003#17340003

template <typename... T>
inline void trace_event(std::string_view category, T... args)
{
    static_assert(std::conjunction_v<std::is_convertible<T, std::string_view>...>);

    foo(category);
    (foo(args), ...);
}

#endif

namespace ibis::tool::event_trace {

// ****************************************************************************
// Macros
// @todo __VA_ARGS__ is a GNU extension, with C++20 replace it with __VA_OPT__; generally get rid
// off macros
// ****************************************************************************

// ----------------------------------------------------------------------------
// unique name macro
// ----------------------------------------------------------------------------

/// Helpers for generating unique variable names in the form of
/// __event_trace_uniq_<label>_<UniqueID>
#define EVENT_TRACE_PRIVATE_UNIQUE_ID __LINE__
#define EVENT_TRACE_PRIVATE_UNIQUE_NAME(label) \
    EVENT_TRACE_PRIVATE_CONCAT(event_trace_uniq, label, EVENT_TRACE_PRIVATE_UNIQUE_ID)

// concat helper macros
#define EVENT_TRACE_PRIVATE_CONCAT(a, b, c) EVENT_TRACE_PRIVATE_CONCAT_(a, b, c)
#define EVENT_TRACE_PRIVATE_CONCAT_(a, b, c) __##a##_##b##_##c

// ----------------------------------------------------------------------------
// User's trace event macros
// ----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
/// Records a pair of begin and end events called "event_name" for the current scope, with 0, 1 or 2
/// associated arguments. If the category is not enabled, then this does nothing.
///
/// Note: ```category_name``` strings must have application lifetime (statics or literals).
///
#define TRACE_EVENT0(category_name, event_name) \
    INTERNAL_TRACE_EVENT_ADD_SCOPED(category_name, event_name)
#define TRACE_EVENT1(category_name, event_name1_name, arg1_val) \
    INTERNAL_TRACE_EVENT_ADD_SCOPED(category_name, event_name, arg1_name, arg1_val)

///////////////////////////////////////////////////////////////////////////////
/// Records a single BEGIN event called "event_name" immediately, with 0, 1 or 2  associated
/// arguments. If the category is not enabled, then this does nothing.
///
/// Note: ```category_name``` strings must have application lifetime (statics or literals).
///
#define TRACE_EVENT_BEGIN0(category_name, event_name)                             \
    INTERNAL_TRACE_EVENT_ADD(TraceEvent::phase::BEGIN, category_name, event_name, \
                             TraceEvent::flag::NONE)

#define TRACE_EVENT_BEGIN1(category_name, event_name, arg1_name, arg1_val)        \
    INTERNAL_TRACE_EVENT_ADD(TraceEvent::phase::BEGIN, category_name, event_name, \
                             TraceEvent::flag::NONE, arg1_name, arg1_val)

///////////////////////////////////////////////////////////////////////////////
/// Records a single END event for "event_name" immediately. If the category is not enabled, then
/// this does nothing.
///
/// Note: ```category_name``` strings must have application lifetime (statics or literals).
///
#define TRACE_EVENT_END0(category_name, event_name)                             \
    INTERNAL_TRACE_EVENT_ADD(TraceEvent::phase::END, category_name, event_name, \
                             TraceEvent::flag::NONE, arg1_name, arg1_val)

#define TRACE_EVENT_END1(category_name, event_name)                             \
    INTERNAL_TRACE_EVENT_ADD(TraceEvent::phase::END, category_name, event_name, \
                             TraceEvent::flag::NONE, arg1_name, arg1_val)

///////////////////////////////////////////////////////////////////////////////
/// Time threshold event:  Only record the event if the duration is greater than the specified
/// threshold_us (time in microseconds). If the category is not enabled, then this does nothing.
///
/// Records a pair of begin and end events called "event_name" for the current scope, with 0, 1 or 2
/// associated arguments.
///
/// Note: ```category_name``` strings must have application lifetime (statics or literals).
///
#define TRACE_EVENT_IF_LONGER_THAN0(threshold_us, category_name, event_name) \
    INTERNAL_TRACE_EVENT_ADD_SCOPED_IF_LONGER_THAN(threshold_us, category_name, event_name)

#define TRACE_EVENT_IF_LONGER_THAN1(threshold_us, category_name, event_name, arg1_name, arg1_val) \
    INTERNAL_TRACE_EVENT_ADD_SCOPED_IF_LONGER_THAN(threshold_us, category_name, event_name,       \
                                                   arg1_name, arg1_val)

///////////////////////////////////////////////////////////////////////////////
/// Records a single event called "event_name" immediately, with 0, 1 or 2
/// associated arguments. If the category is not enabled, then this
/// does nothing.
///
/// Note: ```category_name``` strings must have application lifetime (statics or literals).
///
#define TRACE_EVENT_INSTANT0(category_name, event_name)                             \
    INTERNAL_TRACE_EVENT_ADD(TraceEvent::phase::INSTANT, category_name, event_name, \
                             TraceEvent::flag::NONE)

#define TRACE_EVENT_INSTANT1(category_name, event_name, arg1_name, arg1_val)        \
    INTERNAL_TRACE_EVENT_ADD(TraceEvent::phase::INSTANT, category_name, event_name, \
                             TraceEvent::flag::NONE, arg1_name, arg1_val)

///////////////////////////////////////////////////////////////////////////////
/// Records the value of a counter called "event_name" immediately. Value
/// must be representable as a 64 bit integer.
///
/// Note: ```category_name``` strings must have application lifetime (statics or literals).
///
#define TRACE_COUNTER1(category_name, event_name, value)                            \
    INTERNAL_TRACE_EVENT_ADD(TraceEvent::phase::COUNTER, category_name, event_name, \
                             TraceEvent::flag::NONE, "value", static_cast<std::int64_t>(value))

// ToDo: add the "Arg2" version

///////////////////////////////////////////////////////////////////////////////
/// Records the value of a counter called "event_name" immediately. Value
/// must be representable as a 64 bit integer.
///
/// @a id is used to disambiguate counters with the same name. It must either be a pointer or an
/// integer value up to 64 bits. If it's a pointer, the bits will be XOR-ed with a hash of the
/// process ID so that the same pointer on two different processes will not collide.
///
/// Note: ```category_name``` strings must have application lifetime (statics or literals).
///
#define TRACE_COUNTER_ID1(category_name, event_name, id, value)                                 \
    INTERNAL_TRACE_EVENT_ADD_WITH_ID(TraceEvent::phase::COUNTER, category_name, event_name, id, \
                                     TraceEvent::flag::NONE, "value",                           \
                                     static_cast<std::int64_t>(value))

// ToDo: add the "Arg2" version

///////////////////////////////////////////////////////////////////////////////
/// Records a single ASYNC_BEGIN event called "event_name" immediately, with 0, 1 or 2 associated
/// arguments. If the category is not enabled, then this does nothing.
///
/// An asynchronous operation can consist of multiple phases. The first phase is defined by the
/// ASYNC_BEGIN calls. Additional phases can be defined using the ASYNC_STEP_BEGIN macros. When the
/// operation completes, call ASYNC_END. An async operation can span threads and processes, but all
/// events in that operation must use the same @a name and @a id. Each event can have its own args.
///
/// @a id is used to match the ASYNC_BEGIN event with the ASYNC_END event. ASYNC events are
/// considered to match if their category, name and id values all match. @a id must either be a
/// pointer or an integer value up to 64 bits. If it's a pointer, the bits will be XOR-ed with a
/// hash of the process ID so that the same pointer on two different processes will not collide.
///
/// Note: ```category_name``` strings must have application lifetime (statics or literals).
///
#define TRACE_EVENT_ASYNC_BEGIN0(category_name, event_name, id)                                 \
    INTERNAL_TRACE_EVENT_ADD_WITH_ID(TraceEvent::phase::ASYNC_BEGIN, category_name, event_name, \
                                     id, TraceEvent::flag::NONE)

#define TRACE_EVENT_ASYNC_BEGIN1(category_name, event_name, id, arg1_name, arg1_val)            \
    INTERNAL_TRACE_EVENT_ADD_WITH_ID(TraceEvent::phase::ASYNC_BEGIN, category_name, event_name, \
                                     id, TraceEvent::flag::NONE, arg1_name, arg1_val)

///////////////////////////////////////////////////////////////////////////////
/// Records a single ASYNC_STEP event for @a step immediately. If the category is not enabled, then
/// this does nothing.
///
/// The @a event_name and @a id must match the ASYNC_BEGIN event above. The @a step param identifies
/// this step within the async event. This should be called at the beginning of the next phase of an
/// asynchronous operation.
///
/// Note: ```category_name``` strings must have application lifetime (statics or literals).
///
#define TRACE_EVENT_ASYNC_BEGIN_STEP0(category_name, event_name, id, step)                         \
    INTERNAL_TRACE_EVENT_ADD_WITH_ID(TraceEvent::phase::ASYNC_STEP, category_name, event_name, id, \
                                     TraceEvent::flag::NONE, "step", step)

// ToDo: add the "Arg2" version

///////////////////////////////////////////////////////////////////////////////
/// Records a single ASYNC_END event for "event_name" immediately. If the category is not enabled,
/// then this does nothing.
#define TRACE_EVENT_ASYNC_END0(category_name, event_name, id)                                     \
    INTERNAL_TRACE_EVENT_ADD_WITH_ID(TraceEvent::phase::ASYNC_END, category_name, event_name, id, \
                                     TraceEvent::flag::NONE)

#define TRACE_EVENT_ASYNC_END1(category_name, event_name, id, arg1_name, arg1_val)                \
    INTERNAL_TRACE_EVENT_ADD_WITH_ID(TraceEvent::phase::ASYNC_END, category_name, event_name, id, \
                                     TraceEvent::flag::NONE, arg1_name, arg1_val)

// ToDo: add the Arg2 version

// ----------------------------------------------------------------------------
// Implementation details of trace event macros
// ----------------------------------------------------------------------------

// FIXME: TraceID is an own class,  but handled in API as uint64!! Even for JSON!!

///
/// Macro to create static category proxy
// FixMe: rename to: TRACE_EVENT_PRIVATE_GET_CATEGORY_PROXY
///
#define INTERNAL_TRACE_EVENT_GET_CATEGORY_INFO(category_name) \
    static auto const EVENT_TRACE_PRIVATE_UNIQUE_NAME(category) = category::get(category_name)

// ------------------------------------------------------------------------------------------------

///
/// Macro to create static category and add the begin event if the category is enabled. Also adds
/// the end event when the scope ends.
///
#define INTERNAL_TRACE_EVENT_ADD_SCOPED(cat_name, event_name, ...)                                 \
    static auto const EVENT_TRACE_PRIVATE_UNIQUE_NAME(category_proxy) = category::get(cat_name);   \
    scope_guard EVENT_TRACE_PRIVATE_UNIQUE_NAME(scope_guard)(                                      \
        EVENT_TRACE_PRIVATE_UNIQUE_NAME(category_proxy), event_name);                              \
    if (EVENT_TRACE_PRIVATE_UNIQUE_NAME(category_proxy)) {                                         \
        AddTraceEvent(TraceEvent::phase::BEGIN,                                                    \
                      EVENT_TRACE_PRIVATE_UNIQUE_NAME(category_proxy).category_name(), event_name, \
                      TraceID::NONE, TraceEvent::flag::NONE, ##__VA_ARGS__);                       \
    }

// ------------------------------------------------------------------------------------------------

/// Macro to create static category and add begin  event if the category is enabled. Also adds the
/// end event when the scope  ends. If the elapsed time is < threshold time, the begin/end pair is
/// erased.
#define INTERNAL_TRACE_EVENT_ADD_SCOPED_IF_LONGER_THAN(threshold, cat_name, event_name, ...)     \
    static auto const EVENT_TRACE_PRIVATE_UNIQUE_NAME(category_proxy) = category::get(cat_name); \
    scope_threshold_guard EVENT_TRACE_PRIVATE_UNIQUE_NAME(scope_guard)(                          \
        EVENT_TRACE_PRIVATE_UNIQUE_NAME(category_proxy), event_name, threshold);                 \
    if (EVENT_TRACE_PRIVATE_UNIQUE_NAME(category_proxy)) {                                       \
        auto const EVENT_TRACE_PRIVATE_UNIQUE_NAME(begin_event_id) =                             \
            AddTraceEvent(TraceEvent::phase::BEGIN,                                              \
                          EVENT_TRACE_PRIVATE_UNIQUE_NAME(category_proxy).category_name(),       \
                          event_name, TraceID::NONE, TraceEvent::flag::NONE, ##__VA_ARGS__);     \
        EVENT_TRACE_PRIVATE_UNIQUE_NAME(scope_guard)                                             \
            .set_threshold_begin_id(EVENT_TRACE_PRIVATE_UNIQUE_NAME(begin_event_id));            \
    }

// ------------------------------------------------------------------------------------------------

///
/// Macro to create static category proxy and add event if the category is enabled.
///
#define INTERNAL_TRACE_EVENT_ADD(phase, cat_name, event_name, flags, ...)                         \
    do {                                                                                          \
        static auto const EVENT_TRACE_PRIVATE_UNIQUE_NAME(category_proxy) =                       \
            category::get(cat_name);                                                              \
        if (EVENT_TRACE_PRIVATE_UNIQUE_NAME(category_proxy)) {                                    \
            AddTraceEvent(phase, EVENT_TRACE_PRIVATE_UNIQUE_NAME(category_proxy).category_name(), \
                          event_name, TraceID::NONE, flags, ##__VA_ARGS__);                       \
        }                                                                                         \
    } while (0)
// ------------------------------------------------------------------------------------------------

///
/// Macro to create static category and add event if the category is enabled.
///
#define INTERNAL_TRACE_EVENT_ADD_WITH_ID(phase, cat_name, event_name, id, flags, ...)             \
    do {                                                                                          \
        static auto const EVENT_TRACE_PRIVATE_UNIQUE_NAME(category_proxy) =                       \
            category::get(cat_name);                                                              \
        if (EVENT_TRACE_PRIVATE_UNIQUE_NAME(category_proxy)) {                                    \
            TraceEvent::flag trace_event_flags = flags | TraceEvent::flag::HAS_ID;                \
            TraceID trace_event_trace_id(id, trace_event_flags);                                  \
            AddTraceEvent(phase, EVENT_TRACE_PRIVATE_UNIQUE_NAME(category_proxy).category_name(), \
                          event_name, trace_event_trace_id.value(), trace_event_flags,            \
                          ##__VA_ARGS__);                                                         \
        }                                                                                         \
    } while (0)

/// ------------------------------------
/// Global overloads
///
/// FixMe: use the proxy as argument to gather the category_name from this, or at least make an
/// overload.
/// ------------------------------------

///
/// @brief No argument version
///
/// @param phase TraceEvent's phase to indicate the nature of an event entry.
/// @param category_name Category name.
/// @param event_name Event name.
/// @param flags TraceEvent's flags.
/// @return The ID of the stored event.
///
template <typename NameT>
inline std::int32_t AddTraceEvent(                     // --
    TraceEvent::phase phase,                           // --
    std::string_view category_name, NameT event_name,  // --
    std::uint64_t trace_id = TraceID::NONE, TraceEvent::flag flags = TraceEvent::flag::NONE)
{
    return TraceLog::GetInstance().AddTraceEvent(  // --
        phase, category_name, event_name,          // --
        trace_id, flags,                           // --
        TraceLog::EVENT_ID_NONE, clock::duration_zero);
}

///
/// @brief One argument version.
///
/// @param phase TraceEvent's phase to indicate the nature of an event entry.
/// @param category_name Category name.
/// @param event_name Event name.
/// @param trace_id TraceEvent's TraceID.
/// @param flags TraceEvent's flags.
/// @param arg1_name Name for the optional argument.
/// @param arg1_value Value for the optional argument.
/// @return The ID of the stored event.
///
template <typename NameT, typename Arg1_KeyT, typename Arg1_ValueT>
inline std::int32_t AddTraceEvent(                     // --
    TraceEvent::phase phase,                           // --
    std::string_view category_name, NameT event_name,  // --
    std::uint64_t trace_id, TraceEvent::flag flags,    // --
    Arg1_KeyT arg1_name, Arg1_ValueT arg1_value)
{
    return TraceLog::GetInstance().AddTraceEvent(        // --
        phase, category_name, event_name,                // --
        trace_id, flags,                                 // --
        TraceLog::EVENT_ID_NONE, clock::duration_zero,   // Threshold support
        arg1_name, arg1_value                            // arg { key : value }
    );
}

///
/// @brief
///
///
/// FixMe: How to cope with different signal handlers in different compile units?
/// FixMe: Allow std::cerr as sink
class topping {
    topping();

public:
    static topping& instance()
    {
        static topping static_instance;
        return static_instance;
    }

public:
    static void init(std::string const& json_filename);
    ~topping();

    static void OutputCallback(std::string_view json_str);

private:
    static void termination_handler([[maybe_unused]] int signum);
    static void register_termination_handler();

private:
    struct file_sink;
    static inline std::unique_ptr<file_sink> sink;
};

struct topping::file_sink {
    file_sink(std::string const& json_filename);
    ~file_sink();

    void write(std::string_view sv) { ostream << sv; }

    std::ofstream ostream;
};

}  // namespace ibis::tool::event_trace
