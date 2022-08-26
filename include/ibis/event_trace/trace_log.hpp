//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <ibis/event_trace/trace_event.hpp>
#include <ibis/event_trace/detail/platform.hpp>

#include <vector>
#include <map>
#include <mutex>
#include <functional>
#include <string_view>
#include <iostream>
#include <cstring>

namespace testsuite::mock {
struct TraceLog;
}

namespace ibis::tool::event_trace {

class TraceLog {
    /// mock friend required for testing copy implementation.
    friend ::testsuite::mock::TraceLog;

private:
    TraceLog();

public:
    static TraceLog& GetInstance()
    {
        static TraceLog static_instance;
        return static_instance;
    };

public:
    /// Marker for required deep copy and memory to be allocated, for use of non-static or
    /// non-NULL terminated strings. The constructor is overloaded to support different kind of
    /// string types.
    ///
    /// @note This class doesn't own any memory.
    ///
    class copy {
        /// Make the parent class friend to hide the critical API from user.
        friend TraceLog;

    public:
        /// Construct a new copy object from C char array.
        explicit copy(const char* const name)
            : sv(name)
        {
        }

        /// Construct a new copy object from string_view.
        explicit copy(std::string_view name)
            : sv(name)
        {
        }

        /// Construct a new copy object from string reference.
        explicit copy(std::string const& name)
            : sv(name)
        {
        }

    public:
        std::string_view get_sv() const { return sv; }

        operator std::string_view() const { return sv; }

        operator trace_value() const { return trace_value(sv); }

    private:
        /// Get the size of the string including terminating '\0'.
        std::size_t alloc_size() const { return sv.size() + 1; }

        /// Makes a '\0' terminated deep copy of the string specified at construction time, then
        /// the object is represented by the new memory address.
        void deep_copy(TraceEvent::storage_ptr& base_ptr, std::size_t& start_offset)
        {
            auto* raw_ptr = base_ptr.get() + start_offset;

            // '\0' terminated deep copy
            auto const count = sv.copy(raw_ptr, sv.size());
            base_ptr[start_offset + count] = '\0';

            // rebind to storage pointer
            this->sv = std::move(std::string_view(raw_ptr, count));

            start_offset += count + 1;  // next start offset to base_ptr
        }

    private:
        std::string_view sv;
    };

public:
    /// Event identifier to express that there is none valid ID. The ID gets a meaning, if a
    /// TraceEvent is to be recorded, if a given time is exceeded (namely
    /// TraceEndOnScopeCloseThreshold). Then the ID of the beginning event.
    static constexpr int32_t EVENT_ID_NONE = -1;

public:
    using output_callback_type = std::function<void(std::string_view json)>;

    void setOutputCallback(output_callback_type&& callback)
    {
        output_callback = std::move(callback);
    }

    // FixMe: better solution required, chrome trace has a global callback
    // static void OutputCallback(std::string_view out);
    static void BufferFullCallback();

public:
    ///
    /// @brief Adds an overloaded TraceEvent without arguments.
    ///
    /// @tparam EventNameT Type of the event_name, basically convertible  to string.
    /// @param phase TraceEvent's phase to indicate the nature of an event entry.
    /// @param category_name Category name.
    /// @param event_name Event name.
    /// @param trace_id TraceLog's ID for tracing.
    /// @param flags TraceEvent's flags.
    /// @param threshold_begin_id Event ID at begin of threshold timing scope.
    /// @param threshold The threshold value.
    /// @return The TraceLog ID.
    ///
    std::int32_t AddTraceEvent(                                         // --
        TraceEvent::phase phase,                                        // --
        std::string_view category_name, std::string_view event_name,    // --
        std::uint64_t trace_id, TraceEvent::flag flags,                 // --
        std::int32_t threshold_begin_id, clock::duration_type threshold);

    ///
    /// Adds an overloaded TraceEvent for arguments.
    ///
    /// @tparam Arg1_KeyT Type of the key name, basically convertible to string.
    /// @tparam Arg1_ValueT Type of the argument value.
    /// @param phase TraceEvent's phase to indicate the nature of an event entry.
    /// @param category_name Category name.
    /// @param event_name Event name.
    /// @param trace_id TraceLog's ID for tracing.
    /// @param flags TraceEvent's flags.
    /// @param arg1_name Key name of the optional argument.
    /// @param arg1_value Value of the optional argument.
    /// @param threshold_begin_id Event ID at begin of threshold timing scope.
    /// @param threshold The threshold value.
    /// @return The TraceLog ID.
    ///
    template <typename Arg1_KeyT, typename Arg1_ValueT>
    std::int32_t AddTraceEvent(                                         // --
        TraceEvent::phase phase,                                        // --
        std::string_view category_name, std::string_view event_name,    // --
        std::uint64_t trace_id, TraceEvent::flag flags,                 // --
        std::int32_t threshold_begin_id, clock::duration_type threshold, // --
        Arg1_KeyT arg1_name, Arg1_ValueT arg1_value                     // --
        );

    ///
    /// Adds a concrete event to the log.
    ///
    /// @param phase TraceEvent's phase to indicate the nature of an event entry.
    /// @param category_name Category name.
    /// @param event_name Event name.
    /// @param trace_id TraceLog's ID for tracing.
    /// @param flags TraceEvent's flags.
    /// @param arg1_name Key name of the optional argument.
    /// @param arg1_value Value of the optional argument.
    /// @param threshold_begin_id Event ID at begin of threshold timing scope.
    /// @param threshold The threshold value.
    /// @param ptr A smart pointer to the copied string data.
    /// @return std::int32_t
    ///
    std::int32_t AddTraceEventInternal(                                   // --
        TraceEvent::phase phase,                                          // --
        std::string_view category_name, std::string_view event_name,      // --
        std::uint64_t trace_id, TraceEvent::flag flags,                   // --
        std::int32_t threshold_begin_id, clock::duration_type threshold,  // --
        TraceEvent::storage_ptr ptr,                                      // --
        std::string_view arg1_name, trace_value arg1_value                // --        
        );

public:
    void SetProcessID(current_proc::id_type process_id);

    current_proc::id_type process_id() const { return process_id_; }

public:
    /// Helper method to enable/disable tracing for all categories.
    void SetEnabled(bool enabled);

    /// test on enabled/disabled tracing for all categories.
    bool IsEnabled() const { return enabled_; }

public:
    std::size_t GetEventsCount() const { return logged_events_.size(); }
    float GetEventBufferPercentFull() const;

public:
    /// Flushes all logged data to the callback.
    void Flush();

    /// simply annotates the stream with "[" and "]" respectively
    void BeginLogging();
    void EndLogging();

    // private:
    void AddThreadNameMetadataEvents();

private:
    /// Controls the number of trace events we will buffer in-memory
    /// before throwing them away.
    static constexpr std::size_t BUFFER_SZ = 500'000;

    /// Number of events to flush at once.
    static constexpr std::size_t BATCH_SZ = 1000;

private:
    /// Collect the amount of memory to be allocated if T is of copy-marker-type
    /// `TraceLog::copy`, otherwise nothings is done.
    template <typename T>
    static std::size_t accumulate_size(std::size_t& alloc_size, T str)
    {
        if constexpr (std::is_same_v<std::decay_t<decltype(str)>, TraceLog::copy>) {
            alloc_size += str.alloc_size();
        }
        return alloc_size;
    }

    /// Allocate memory of size @a alloc_size if required, otherwise ```nullptr``` is returned.
    static TraceEvent::storage_ptr allocate(std::size_t alloc_size)
    {
        TraceEvent::storage_ptr ptr = nullptr;
        if (alloc_size != 0) {
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
            ptr = std::make_unique<char[]>(alloc_size);
        }
        return ptr;
    }

    /// make deep copy if T is of copy-marker-type `TraceLog::copy`, otherwise nothings is done.
    /// Note, in this case @str is modified, using the storage @a base_ptr as new memory for the
    /// string data.
    template <typename T>
    static void deep_copy(TraceEvent::storage_ptr& base_ptr, std::size_t& offset, T& str)
    {
        if constexpr (std::is_same_v<std::decay_t<decltype(str)>, TraceLog::copy>) {
            str.deep_copy(base_ptr, offset);
        }
    }

private:
    static inline output_callback_type output_callback = [](std::string_view) {};

private:
    std::mutex lock_;

    std::vector<TraceEvent> logged_events_;
    std::vector<TraceEvent> flush_events_;

    current_proc::id_type process_id_;

    std::vector<current_thread::id_type> thread_ids_seen;

    // Process ID Hash as TraceID to make it unlikely to collide with other processes.
    std::size_t process_id_hash_;

    bool enabled_;
};

/// --- TODO [C++20] concept
template <typename T>
struct valid_string_arg {
    static constexpr bool value = std::disjunction_v<     // --
        std::is_same<std::decay_t<T>, char const*>,       // --
        std::is_same<std::decay_t<T>, std::string_view>,  // --
        std::is_same<std::decay_t<T>, TraceLog::copy>>;
};

template <typename T>
inline constexpr bool valid_string_arg_v = valid_string_arg<T>::value;

/// ---

inline std::int32_t TraceLog::AddTraceEvent(                        // --
    TraceEvent::phase phase,                                        // --
    std::string_view category_name, std::string_view event_name,    // --
    std::uint64_t trace_id, TraceEvent::flag flags,                 // --
    std::int32_t threshold_begin_id, clock::duration_type threshold)
{
    //static_assert(valid_string_arg_v<EventNameT>, "Wrong Type for 'event_name' argument");

    std::size_t alloc_size = 0;
    std::size_t offset = 0;

    // collect the amount of memory to be allocated ...
    accumulate_size(alloc_size, event_name);

    // ... allocate if required ...
    TraceEvent::storage_ptr ptr = allocate(alloc_size);

    // ... and make deep copy if required
    deep_copy(ptr, offset, event_name);

    return AddTraceEventInternal(               // --
        phase,                                  // --
        category_name, event_name,              // --
        trace_id, flags,                        // --
        threshold_begin_id, threshold,          // --
        std::move(ptr),                         // --
        std::string_view{}, trace_value{}       // -- no argument        
        );
}

template <typename Arg1_KeyT, typename Arg1_ValueT>
inline std::int32_t TraceLog::AddTraceEvent(                        // --
    TraceEvent::phase phase,                                        // --
    std::string_view category_name, std::string_view event_name,    // --
    std::uint64_t trace_id, TraceEvent::flag flags,                 // --
    std::int32_t threshold_begin_id, clock::duration_type threshold, // --
    Arg1_KeyT arg1_name, Arg1_ValueT arg1_value                     // --
    )
{
    //static_assert(valid_string_arg_v<EventNameT>, "Wrong Type for 'event_name' argument");
    static_assert(valid_string_arg_v<Arg1_KeyT>, "Wrong Type for 'arg1_name' argument");

    // FIXME: with variadic templates allocate must be rewritten. Maybe move to own class
    // which simplifies testing.
    std::size_t alloc_size = 0;
    std::size_t offset = 0;

    // collect the amount of memory to be allocated ...
    accumulate_size(alloc_size, event_name);
    accumulate_size(alloc_size, arg1_name);
    accumulate_size(alloc_size, arg1_value);

    // ... allocate ...
    TraceEvent::storage_ptr ptr = allocate(alloc_size);

    // ... and make deep copy.
    deep_copy(ptr, offset, event_name);
    deep_copy(ptr, offset, arg1_name);
    deep_copy(ptr, offset, arg1_value);

    return AddTraceEventInternal(           // --
        phase,                              // --
        category_name, event_name,          // --
        trace_id, flags,                    // --
        threshold_begin_id, threshold,      // --
        std::move(ptr),                     // --
        arg1_name, arg1_value               // -- argument (key : value)
        );
}

}  // namespace ibis::tool::event_trace
