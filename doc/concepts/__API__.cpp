// API

// Fix: NameT is useless - finally only string_view

template <typename NameT, typename Arg1_KeyT, typename Arg1_ValueT> // free function
std::int32_t AddTraceEvent(                     // --
    TraceEvent::phase phase,                           // --
    std::string_view category_name, NameT event_name,  // --
    std::uint64_t trace_id, TraceEvent::flag flags,    // --
    Arg1_KeyT arg1_name, Arg1_ValueT arg1_value)
{
    return TraceLog::GetInstance().AddTraceEvent(        // --
        phase, category_name, event_name,                // --
        trace_id, flags,                                 // --
        arg1_name, arg1_value,                           // arg { key : value }
        TraceLog::EVENT_ID_NONE, clock::duration_zero);  // Threshold support
}

// FiME: on scope close, there aren't any args, hence no allocation is required and TraceLog's internals
// are called. Internals shall be private - make a 'non-storing' API call possible.
// solution: tag dispatch?? template<BooleanT T> allocate : std::true_type {}; ...
class TraceEndOnScopeCloseThreshold {
    ...
    void set_threshold_begin_id(std::int32_t event_id) { threshold_begin_id = event_id; }
    void AddEvent()  {
        TraceLog::GetInstance().AddTraceEventInternal(                           // --
            TraceEvent::phase::END, category_proxy.category_name(), event_name,  // --
            TraceID::NONE, TraceEvent::flag::NONE,                               // --
            std::string_view(), TraceValue(),                                    // -- no arguments
            threshold_begin_id, threshold,                                       // --
            nullptr);
    }
...
};

// FixMe: Move allocate stuff into own class, e.g. as template arg to allow exchange of them
// This would decouple the TraceEvent::storage_ptr, since Event doesn't allocate, but specifies
// the type of (unique) pointer.
template <typename EventNameT, typename Arg1_KeyT, typename Arg1_ValueT>
inline std::int32_t TraceLog::AddTraceEvent(                // --
    TraceEvent::phase phase,                                // --
    std::string_view category_name, EventNameT event_name,  // --
    std::uint64_t trace_id, TraceEvent::flag flags,         // --
    Arg1_KeyT arg1_name, Arg1_ValueT arg1_value,            // --
    std::int32_t threshold_begin_id, clock::duration_type threshold)
{
    std::size_t alloc_size = 0;
    std::size_t offset = 0;
    accumulate_size(alloc_size, event_name);
    accumulate_size(alloc_size, arg1_name);
    accumulate_size(alloc_size, arg1_value);
    TraceEvent::storage_ptr ptr = allocate(alloc_size);
    deep_copy(ptr, offset, event_name);
    deep_copy(ptr, offset, arg1_name);
    deep_copy(ptr, offset, arg1_value);

    return AddTraceEventInternal(       // --
        phase,                          // --
        category_name, event_name,      // --
        trace_id, flags,                // --
        arg1_name, arg1_value,          // -- argument (key : value)
        threshold_begin_id, threshold,  // --
        std::move(ptr));
}

std::int32_t TraceLog::AddTraceEventInternal(  // -- TODO: rename to StoreTraceEvent
    TraceEvent::phase phase, 
    std::string_view category_name, std::string_view event_name,
    std::uint64_t trace_id, TraceEvent::flag flags,                   // --
    std::string_view arg1_name, TraceValue arg1_value,                // --
    std::int32_t threshold_begin_id, clock::duration_type threshold,  // --
    TraceEvent::storage_ptr ptr)
{
...
    // use the current length of traced events as ID of event
    std::int32_t const event_id = static_cast<std::int32_t>(logged_events_.size());

    logged_events_.emplace_back(           // TraceEvent(...)
        thread_id, time_point,             // --
        phase, category_name, event_name,  // --
        trace_id, flags,                   // --
        arg1_name, arg1_value,             // --
        std::move(ptr));

    return event_id;
}

// ---------------------------------------------------------------------------------------------------------

#define INTERNAL_TRACE_EVENT_ADD_SCOPED(cat_name, event_name, ...)                                 \
    static auto const EVENT_TRACE_PRIVATE_UNIQUE_NAME(category_proxy) = category::get(cat_name);   \
    TraceEndOnScopeClose EVENT_TRACE_PRIVATE_UNIQUE_NAME(scope_guard)(                             \
        EVENT_TRACE_PRIVATE_UNIQUE_NAME(category_proxy), event_name);                              \
    if (EVENT_TRACE_PRIVATE_UNIQUE_NAME(category_proxy)) {                                         \
        AddTraceEvent(TraceEvent::phase::BEGIN,                                                    \
                      EVENT_TRACE_PRIVATE_UNIQUE_NAME(category_proxy).category_name(), event_name, \
                      TraceID::NONE, TraceEvent::flag::NONE, ##__VA_ARGS__);                       \
    }

#define INTERNAL_TRACE_EVENT_ADD_SCOPED_IF_LONGER_THAN(threshold, cat_name, event_name, ...)     \
    static auto const EVENT_TRACE_PRIVATE_UNIQUE_NAME(category_proxy) = category::get(cat_name); \
    TraceEndOnScopeCloseThreshold EVENT_TRACE_PRIVATE_UNIQUE_NAME(scope_guard)(                  \
        EVENT_TRACE_PRIVATE_UNIQUE_NAME(category_proxy), event_name, threshold);                 \
    if (EVENT_TRACE_PRIVATE_UNIQUE_NAME(category_proxy)) {                                       \
        auto const EVENT_TRACE_PRIVATE_UNIQUE_NAME(begin_event_id) =                             \
            AddTraceEvent(TraceEvent::phase::BEGIN,                                              \
                          EVENT_TRACE_PRIVATE_UNIQUE_NAME(category_proxy).category_name(),       \
                          event_name, TraceID::NONE, TraceEvent::flag::NONE, ##__VA_ARGS__);     \
        EVENT_TRACE_PRIVATE_UNIQUE_NAME(scope_guard)                                             \
            .set_threshold_begin_id(EVENT_TRACE_PRIVATE_UNIQUE_NAME(begin_event_id));            \
    }

#define INTERNAL_TRACE_EVENT_ADD(phase, cat_name, event_name, flags, ...)                         \
    do {                                                                                          \
        static auto const EVENT_TRACE_PRIVATE_UNIQUE_NAME(category_proxy) =                       \
            category::get(cat_name);                                                              \
        if (EVENT_TRACE_PRIVATE_UNIQUE_NAME(category_proxy)) {                                    \
            AddTraceEvent(phase, EVENT_TRACE_PRIVATE_UNIQUE_NAME(category_proxy).category_name(), \
                          event_name, TraceID::NONE, flags, ##__VA_ARGS__);                       \
        }                                                                                         \
    } while (0)

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
