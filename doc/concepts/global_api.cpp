#include <variant>
#include <string_view>
#include <cstdint>
#include <cassert>
#include <cstring>
#include <iostream>
#include <array>
#include <type_traits>
#include <tuple>
#include <vector>

// Shows API concept

using namespace std::literals::string_view_literals;

constexpr auto MAX_ARGS = 2;

template<typename T> concept ArgNameT = std::convertible_to<T, std::string_view>;
template<typename T> concept ArgValueT = std::convertible_to<T, int>;

struct TraceValue {
    int value;
};

struct TraceEvent {
    template<typename... ArgNames, typename... ArgValues>
    TraceEvent(std::string_view event_name_, ArgNames&& ...arg_names_, ArgValues&& ...arg_values_) 
    : event_name{ event_name_ }
    , arg_names{ std::forward<ArgNames>(arg_names_)...}
    , arg_values{ std::forward<ArgValues>(arg_values_)...}
    {}
    std::string event_name;
    // This memory layout to minimize footprint (not like array<tuple<string,value>>)
    std::array<std::string_view, MAX_ARGS> arg_names;
    std::array<TraceValue, MAX_ARGS> arg_values;
};

struct TraceLog {
    static TraceLog& instance() {
        static TraceLog static_instance;
        return static_instance;
    };
    template<typename... Args> void AddTraceEvent(std::string_view name, Args&& ...args);
    template<typename... Args> void AddTraceEventPrivate(std::string_view name, int payload, Args&& ...args);

    using arg_type = struct { std::string_view name; int value; };

    auto make_arg_types() { return std::tuple<>(); }

    template<ArgNameT FirstName, ArgValueT FirstValue, typename... Args>
    auto make_arg_types(const FirstName &name, const FirstValue &value, const Args&... args) {
        assert(std::string_view{name}.empty() == false && "arg_name must not be empty!");
        return std::tuple_cat(
            std::make_tuple(arg_type{name, value}), 
            make_arg_types(args...)
        );
    }

    std::vector<TraceEvent> events;
};

template<typename... Args>
void AddEvent(std::string_view event_name, Args&& ...args) { // free function
    TraceLog::instance().AddTraceEvent(event_name, std::forward<Args>(args)...);
}

// implementation
template<typename... Args>
void TraceLog::AddTraceEvent(std::string_view event_name, Args&& ...args) {
    int payload = 69;
    AddTraceEventPrivate(event_name, payload, std::forward<Args>(args)...);
}
template<typename... Args>
void TraceLog::AddTraceEventPrivate(std::string_view event_name, int payload, Args&& ...args) {
    //events.emplace_back(event_name, ???); // emplace TraceEvent
}

int main() {
    
    AddEvent("Batman", "is bad"sv, 4711);

    return 0;
}
