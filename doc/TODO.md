# Original articles

- [In-depth: Using Chrome://tracing to view your inline profiling data](
  https://www.gamedeveloper.com/programming/in-depth-using-chrome-tracing-to-view-your-inline-profiling-data)
  with GH repository
  [mainroach/sandbox](https://github.com/mainroach/sandbox/tree/master/chrome-event-trace)
- [Chrome Tracing as Profiler Frontend](https://aras-p.info/blog/2017/01/23/Chrome-Tracing-as-Profiler-Frontend/)
- [Trace Event Format (2016)](https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU/edit)

# TODO

- **urgent**: make it compile again as stand-alone, with `sanitize`
- refactor allocator code of TraceLog::AddTraceEvent() into own class (which gets instantiated, e.g. by
  template arg) with variadic args
- make the API C++ variadic
- better tests with static dependency injection like 
  [Are there facilities in std::chrono to assist with injecting system_clock for unit testing](
  https://stackoverflow.com/questions/33606482/are-there-facilities-in-stdchrono-to-assist-with-injecting-system-clock-for-un),
  

# Concept for variadic template

## Remarks

### Arg{Name, Value} constrains

---
> Always use non-empty arg names!

```cpp
`alway_assert(arg_name.data() != nullptr && !arg_name.empty(), "You fool");` // Debug, Release mode?
```

Otherwise TraceEvent::AppendAsJSON() will fail since the code contains something like:

```cpp
        if (arg1_name_ != nullptr) {
            json.attributeObject("args", [&]() { json.attribute(arg1_name_, arg1_value_); });
        }
```

---

Even the rework/new code using std::array depends on non-empty arg_names. Otherwise a new
member `tracking_cnt` arg count to track the number of arguemnts will be required (which makes the
TraceEvent class bigger). Iterating over std::array from first to end, where end is always determined
by compile size, see https://godbolt.org/z/en6qa18Ga

Nevertheless, using empty name doesn't make sense. =>

## will become relevant later

see

- https://thenewcpp.wordpress.com/2011/11/23/variadic-templates-part-1-2/
- https://thenewcpp.wordpress.com/2011/11/29/variadic-templates-part-2/
- https://thenewcpp.wordpress.com/2012/02/15/variadic-templates-part-3-or-how-i-wrote-a-variant-class/
- https://www.cppstories.com/2021/non-terminal-variadic-args/ with
  https://quuxplusone.github.io/blog/2020/02/12/source-location/


## concept, waits for variadic templates
https://godbolt.org/z/en4sro4vn (gets obsolete, too complex)


---
> Alipha: https://godbolt.org/z/8j6W987f9 - not was intended :( - but god approach finally!!
---

## Start Simple

Current state:
- API: https://godbolt.org/z/x7n9Y4hdE
- Arg {name,Value} handling: https://godbolt.org/z/f5bz1c15P , 2nd: https://godbolt.org/z/8Edch148d
- Shows array filling: https://godbolt.org/z/GY9G8YPGf
- JSON printing: https://godbolt.org/z/vMY9W1T7z

- SO version of API: https://godbolt.org/z/szW4fn55K
- template arg transform mini test: https://godbolt.org/z/7WoWGhWPj doesn't work yet
    ```cpp
    #include <tuple>
    #include <iostream>

    template <typename... T>
    void print_names(T&&... names) {
        std::cout << "names: ";
        ((std::cout << std::forward<T>(names)), ...);
        std::cout << "\n";
    }
    template <typename... T>
    void print_values(T&&... values) {
        std::cout << "values: ";
        ((std::cout << std::forward<T>(values)), ...);
        std::cout << "\n";
    }


    template <typename Tuple, size_t... Is>
    void printHelper(Tuple&& names_and_vals, std::index_sequence<Is...>) {
        print_names((std::get<Is*2>(names_and_vals), ...));
        //print_values((std::get<Is*2 + 1>(names_and_vals)), ...);
    #if 1
        ((std::cout << std::get<Is*2>(names_and_vals) << std::endl), ...);
        ((std::cout << std::get<Is*2 + 1>(names_and_vals) << std::endl), ...);
    #endif
    }

    template <typename... NamesAndVals>
    void print(NamesAndVals&&... names_and_vals) {
        printHelper(std::forward_as_tuple(names_and_vals...),
                    std::make_index_sequence<sizeof...(NamesAndVals)/2>{});
    }

    int main() {
        print("x", 1, "y", 2, "z", 3);
    }
    ```

## FixMe

- fix AddTraceEvent API, these can resolve category's name from proxy directly:
  ```cpp
    AddTraceEvent(TraceEvent::phase::BEGIN,
                category_proxy__.category_name(), event_name,
                TraceID::NONE, TraceEvent::flag::NONE, ...);
  ```

### concept for API

---
> still not working: https://godbolt.org/z/x7n9Y4hdE
---

```cpp
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
```

### template variadic parameters for TraceEvent

---
> working TraceValue concept compiles:
- https://godbolt.org/z/rvec1fbsT - TraceEvent with variadic parameters and simple template parameter checks
---

```
```

### Fill a std::array with variadic parameters (for ArgName, ArgValue))

see https://godbolt.org/z/en6qa18Ga will work for ArgValues

```cpp
template<unsigned SZ = 4>
struct Array
{
    template <typename... Args>
    Array(Args&&... args) noexcept {
        static_assert(sizeof...(Args) <= SZ, "Too many args");
        std::size_t idx = 0;
        // The comma operator is left-associative, ideally one would use a left unary fold.
        // Cast to void to ensures the use of built-in comma operator.
        (...,void(entries[idx++] = args));
    }
    void print_on(std::ostream& os) const;

    std::array<int, SZ> entries;
};
```

Note: this approach will not work with ArgNames and run into trouble in real project, see
[Remarks: Arg{Name, Value} constrains](#argname-value-constrains)

A solution is:

see: https://godbolt.org/z/x61WY387r

```cpp
template<unsigned SZ = 4>
struct Array
{
    template <typename... Args>
    Array(Args&&... args) noexcept {

        static_assert(sizeof...(Args) <= SZ, "Too many args");

        (assert(!args.empty()), ...); // reject empty arg_name(s)

        std::size_t idx = 0;
        // The comma operator is left-associative, ideally one would use a left unary fold.
        // Cast to void to ensures the use of built-in comma operator.
        (...,void(entries[idx++] = args));
    }

    void print_on(std::ostream& os) const {
        os << '[';
        std::copy_if(entries.begin(), entries.end(), std::ostream_iterator<std::string>(os, " "),
            [](auto const& str){ return !str.empty(); });
        os << ']';
    }
};
```

# Experience

## MSVC requires special handling of constructor for variant type (TraceValue)

see https://godbolt.org/z/17f3WGMzG (older https://godbolt.org/z/PoK55MxEe)

```cpp
struct Value {
    using variant_type = std::variant<  // --
    std::monostate,                 // mark default constructible empty state
    bool, uint64_t, int64_t, double, char const *, void const *>;

    Value(auto value_) : value{ value_ } {}
#if 1 // required for MSVC
    Value(UnsignedInteger auto value_) : value{ uint64_t{value_} } { }
    Value(SignedInteger auto value_)  : value{ int64_t{value_} } {}
#endif
    Value(std::string_view sv) : value{ sv.data() } {}
```

## Concept for TraceValue Name/Value pair

see https://coliru.stacked-crooked.com/a/67ccdc9a5fd68987

```cpp
struct TraceValue {
    using value_type = std::variant<std::monostate, int64_t, char const*, double>;
    TraceValue(auto val) : value{ val } {}
    value_type value;
};

template <typename T> concept ArgValueT = std::convertible_to<T, TraceValue::value_type>;

template<ArgValueT ArgValue>
void foo(ArgValue v) {}

template<typename ArgValue> requires ArgValueT<ArgValue>
void bar(ArgValue v) {}

void baz(ArgValueT auto v) {}

int main() {
    foo(42);
    bar(42.0);
    baz("Hello World");
    //foo(new int); // compile error
    return 0;
}
```

## variadic templates with two types fails to compile

see https://godbolt.org/z/9zffcWMEa (older https://godbolt.org/z/M3o8zPMqM)

```cpp
template<typename T> concept NameT = std::convertible_to<T, std::string_view>;
template<typename T> concept ValueT = std::convertible_to<T, int>;

struct TraceEvent {
    template<NameT ...ArgName, ValueT ...ArgValue>
    TraceEvent(ArgName&&... arg_name, ArgValue&&... arg_value)
    {
        ((std::cout << std::forward<ArgName>(arg_name) << " : " << std::forward<ArgValue>(arg_value) << '\n'), ...);
    }
};

int main() {
    auto const v1 = TraceEvent("arg1", 42);
    return 0;
}
```

with:

```
candidate template ignored: constraints not satisfied [with ArgName = <>, ArgValue = <const char (&)[5], int>]
```

The constrains/rule is not strong enough, but how to?

## Using tuple_cat

see https://coliru.stacked-crooked.com/a/e1c93a218480b240

```cpp
#include <tuple>
#include <iostream>

template<typename... FirstArgs, typename... SecondArgs>
auto merge(std::tuple<FirstArgs...> first, std::tuple<SecondArgs...> second) {
     return std::tuple_cat(first, second);
}

// [Pretty-print std::tuple](https://stackoverflow.com/questions/6245735/pretty-print-stdtuple)
template<class Ch, class Tr, class... Args>
auto& operator<<(std::basic_ostream<Ch, Tr>& os, std::tuple<Args...> const& t) {
    std::apply([&os](auto&&... args) {
        ((os << args << " "), ...);
    }, t);
    return os;
}

int main() {
    auto const t = merge(std::tuple(2), std::tuple(4U, 'a'));
    std::cout << t << '\n';
}
```

gives `2 4 a`

## Compile time fixed array with type deduction required

see https://godbolt.org/z/YM65j3Ta1

```
template<unsigned SZ>
struct Array
{
    template <typename... Args>
    Array(Args&&... args) noexcept : entries{args...} {
    }

    void print_on(std::ostream& os) const {
        std::copy(entries.begin(), entries.end(), std::ostream_iterator<int>(os, " "));
    }

    std::array<int, SZ> entries;
};

// deduction guide to deduce the size NTTP (Non-Type Template Parameter)
template<typename... Args>
Array(Args&&... args) -> Array<sizeof...(args)>;
```

deduction guides are a mechanism of saying "oh you're calling a ctor that looks like this? well here's what the corresponding class template argument should be then"
like suppose you wanna make a container that supports an iterator pair constructor
then you have a `template<typename T> struct Container { ... };` where the value type is T
and you declare a ctor like `template<typename It> Container(It begin, It end)`
but then if I write `Container data(p_begin, p_end);` how is the compiler supposed to know what T is?
I have to use a deduction guide to tell it that T should be  `It::value_type`
(or more correctly std::iterator_traits<It>::value_type)

# URLs


https://github.com/uucidl/uu.spdr
https://github.com/jlfwong/speedscope
https://github.com/ottojo/Profiler
https://github.com/hrydgard/minitrace
https://github.com/KjellKod/g3log
https://github.com/boostorg/log
https://github.com/google/UIforETW (Windows only?)

https://docs.microsoft.com/en-us/windows/win32/tracelogging/tracelogging-c-cpp-tracelogging-examples


xx
https://www.cppstories.com/2021/non-terminal-variadic-args/
