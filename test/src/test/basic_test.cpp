//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <ibis/event_trace/event_trace.hpp>
#include <ibis/event_trace/category.hpp>

#include <testsuite/mock_std_clock.hpp>
#include <testsuite/mock_trace_log.hpp>
#include <testsuite/namespace_alias.hpp>

#include <ibis/util/support/cxx/overloaded.hpp>

#include <boost/test/unit_test.hpp>
#include <boost/test/tools/output_test_stream.hpp>

#include <iomanip>

///
/// BOOST TEST requires that the types must be streamable, here we go ...
///
namespace std {

std::ostream& operator<<(std::ostream& os, ibis::tool::event_trace::clock::time_point_type tp)
{
    os << std::chrono::time_point_cast<std::chrono::nanoseconds>(tp).time_since_epoch().count();
    return os;
}

std::ostream& operator<<(std::ostream& os, std::pair<bool, bool> pair)
{
    os << "std::pair(" << std::boolalpha << pair.first << ", " << pair.second << ")";
    return os;
}

std::ostream& operator<<(std::ostream& os,
                         ibis::tool::event_trace::TraceValue::variant_type const& variant)
{
    using ibis::util::overloaded;
    std::visit(  // --
        overloaded{
            [&os]([[maybe_unused]] std::monostate v) { os << "TraceValue<std::monostate>"; },
            [&os]([[maybe_unused]] bool v) { os << "TraceValue<bool>"; },
            [&os]([[maybe_unused]] uint64_t v) { os << "TraceValue<uint64_t>"; },
            [&os]([[maybe_unused]] int64_t v) { os << "TraceValue<int64>"; },
            [&os]([[maybe_unused]] double v) { os << "TraceValue<double>"; },
            [&os]([[maybe_unused]] std::string_view sv) { os << "TraceValue<std::string_view>"; },
            [&os]([[maybe_unused]] void const* ptr) { os << "TraceValue<pointer>"; },
        },
        variant);
    return os;
}
}  // namespace std

///
/// This output callback fixture is intended to render the logged events. Each test case has it's
/// own callback (fixture). To ensure a clean start each time, the used output stream is created new
/// on each time.
///
struct callback_fixture {
    static callback_fixture& instance()
    {
        static callback_fixture instance;
        return instance;
    }

    void setup()
    {
        // std::cout << "Callback Fixture Setup\n";
        os = std::make_unique<btt::output_test_stream>();
    }
    void teardown()
    {
        // std::cout << "Callback Fixture Teardown\n";
    }

    void output(std::string_view sv) { *os << sv; };

    std::string result_str() const
    {
        assert(os != nullptr);
        return os->str();
    }

private:
    static inline std::unique_ptr<btt::output_test_stream> os = nullptr;
};

///
/// Boost.Test allows only one fixture at each test case. Following the documentation
/// [Single test case fixture](
///  https://www.boost.org/doc/libs/1_76_0/libs/test/doc/html/boost_test/tests_organization/fixtures/case.html#boost_test.tests_organization.fixtures.case.f0)
/// note #4,  it is still possible to define a class inheriting from several fixtures, that will act
/// as a proxy fixture.
///
struct proxy_fixture
    : public testsuite::clock_fixture
    , public callback_fixture {
    void setup()
    {
        clock_fixture::setup();
        callback_fixture::setup();
    }
    void teardown()
    {
        clock_fixture::teardown();
        callback_fixture::teardown();
    }
};

///
/// Act as global fixture for TraceLog's OutputCallback, called at setup time of whole test suite.
///
struct callback_initializer_fixture {
    void setup()
    {
        using ibis::tool::event_trace::TraceLog;

        TraceLog::GetInstance().setOutputCallback(
            [](std::string_view sv) { callback_fixture::instance().output(sv); });
    }
    void teardown() {}
};

BOOST_AUTO_TEST_SUITE(common_instrumentation_utils)

BOOST_TEST_GLOBAL_FIXTURE(callback_initializer_fixture);

using namespace ibis::tool::event_trace;

//-----------------------------------------------------------------------------
//
// The filter to select which category entries are allowed to log. The filter supports RE - for
// detailed RE check, use [Regex101](https://regex101.com/) Here we test only the basic concept
// since we trust in std::regex
//
BOOST_AUTO_TEST_CASE(category_filter)
{
    category::filter filter("foo, -bar");

    BOOST_TEST(filter.count() == 2);

    BOOST_TEST(filter.result_of("foo") == std::pair(true, true));
    BOOST_TEST(filter.result_of("foot") == std::pair(true, true));
    BOOST_TEST(filter.result_of("barfooted") == std::pair(true, true));  // 1st match wins!
    BOOST_TEST(filter.result_of("bar") == std::pair(true, false));
    BOOST_TEST(filter.result_of("bart") == std::pair(true, false));
    BOOST_TEST(filter.result_of("centibar") == std::pair(true, false));
}

//-----------------------------------------------------------------------------
//
// Test the proxy of catalog's entry items.
//
BOOST_AUTO_TEST_CASE(category_proxied_entries)
{
    // test helper to check proxy basic functionality
    auto const proxy_state = [](auto cat) {
        auto const event_proxy = category::get(cat);
        if (event_proxy) {
            // std::cout << cat << " is true\n";
            return true;
        }
        // std::cout << cat << " is false\n";
        return false;
    };

    // ------------------------------------------------------------------------
    // starting test with default category filter ...
    // ------------------------------------------------------------------------

    // insert/append categories
    category::instance().append({ { "foo", true }, { "bar", false } });

    // existing category with the enable state given on construction time
    BOOST_TEST(proxy_state("foo") == true);
    BOOST_TEST(proxy_state("bar") == false);

    // create a new category, enable state depend on category filter result
    BOOST_TEST(proxy_state("foo2") == true);
    BOOST_TEST(proxy_state("bar2") == true);

    // append more categories
    auto const cat_list = { category::entry::value_type{ "foo3", true }, { "bar3", false } };
    category::instance().append(cat_list);

    BOOST_TEST(proxy_state("foo3") == true);
    BOOST_TEST(proxy_state("bar3") == false);

    // category::instance().dump(std::cout);

    // ------------------------------------------------------------------------
    // now replace default category filter ...
    // ------------------------------------------------------------------------

    category::instance().set_filter(category::filter("foo[23], -bar[23]"));
    // category::instance().dump(std::cout);

    BOOST_TEST(proxy_state("foo") == true);   // not affected
    BOOST_TEST(proxy_state("bar") == false);  // not affected
    BOOST_TEST(proxy_state("foo2") == true);
    BOOST_TEST(proxy_state("bar2") == false);
    BOOST_TEST(proxy_state("foo3") == true);
    BOOST_TEST(proxy_state("bar3") == false);

    // ------------------------------------------------------------------------
    // now replace default category filter once more, this time inverse ...
    // ------------------------------------------------------------------------

    category::instance().SetEnabled("-foo[23], bar[23]");
    // category::instance().dump(std::cout);

    BOOST_TEST(proxy_state("foo") == true);   // not affected
    BOOST_TEST(proxy_state("bar") == false);  // not affected
    BOOST_TEST(proxy_state("foo2") == false);
    BOOST_TEST(proxy_state("bar2") == true);
    BOOST_TEST(proxy_state("foo3") == false);
    BOOST_TEST(proxy_state("bar3") == true);

    // ------------------------------------------------------------------------
    // check proxy's enable "link" capability to category registry
    // ------------------------------------------------------------------------

    {
        auto const proxy = category::get("batz");
        BOOST_TEST(proxy.enabled() == true);  // enabled by default
        category::instance().SetEnabled("-batz");
        BOOST_TEST(proxy.enabled() == false);  // now diabled

        // shouldn't be changed
        BOOST_TEST(proxy_state("foo") == true);
        BOOST_TEST(proxy_state("bar") == false);
        BOOST_TEST(proxy_state("foo2") == false);
        BOOST_TEST(proxy_state("bar2") == true);
        BOOST_TEST(proxy_state("foo3") == false);
        BOOST_TEST(proxy_state("bar3") == true);
    }
}

// ----------------------------------------------------------------------------
//
// Check the correct behavior of TraceLog::copy() capabilities. Since the relevant members are
// private, a mock class is used. The code below mimics the behaviour of
// TraceLog::AddTraceEvent().
//
BOOST_AUTO_TEST_CASE(copy_marker)
{
    using namespace std::literals;

    char const cstr[] = "(Hello World cstr)";
    std::string_view const sv = "[Hello World sv]";
    std::string const str = "{Hello World str}";

    // small helper to get the char type pruned address
    auto const as_void_ptr = [](auto const* str) { return static_cast<void const* const>(str); };

    void const* const cstr_ptr = as_void_ptr(cstr);
    void const* const sv_ptr = as_void_ptr(sv.data());
    void const* const str_ptr = as_void_ptr(str.data());

    // ------------------------------------------------------------------------
    // test if there is no TraceLog::copy, shall be left as is
    // ------------------------------------------------------------------------
    {
        std::size_t alloc_size = 0;
        std::size_t offset = 0;

        ::testsuite::TraceLogMock::accumulate_size(alloc_size, cstr);
        ::testsuite::TraceLogMock::accumulate_size(alloc_size, sv);
        ::testsuite::TraceLogMock::accumulate_size(alloc_size, str);

        TraceEvent::storage_ptr ptr = ::testsuite::TraceLogMock::allocate(alloc_size);

        ::testsuite::TraceLogMock::deep_copy(ptr, offset, cstr);
        ::testsuite::TraceLogMock::deep_copy(ptr, offset, sv);
        ::testsuite::TraceLogMock::deep_copy(ptr, offset, str);

        BOOST_TEST(alloc_size == 0);
        BOOST_TEST(offset == 0);
        BOOST_TEST(!ptr);  // nullptr

        // still point to origin
        BOOST_TEST(as_void_ptr(cstr) == cstr_ptr);
        BOOST_TEST(as_void_ptr(sv.data()) == sv_ptr);
        BOOST_TEST(as_void_ptr(str.data()) == str_ptr);
    }

    // ------------------------------------------------------------------------
    // now with TraceLog::copy
    // ------------------------------------------------------------------------
    {
        TraceLog::copy cstr_cpy(cstr);
        TraceLog::copy sv_cpy(sv);
        TraceLog::copy str_cpy(str);

        std::size_t alloc_size = 0;
        std::size_t offset = 0;

        ::testsuite::TraceLogMock::accumulate_size(alloc_size, cstr_cpy);
        ::testsuite::TraceLogMock::accumulate_size(alloc_size, sv_cpy);
        ::testsuite::TraceLogMock::accumulate_size(alloc_size, str_cpy);

        TraceEvent::storage_ptr ptr = ::testsuite::TraceLogMock::allocate(alloc_size);

        ::testsuite::TraceLogMock::deep_copy(ptr, offset, cstr_cpy);
        ::testsuite::TraceLogMock::deep_copy(ptr, offset, sv_cpy);
        ::testsuite::TraceLogMock::deep_copy(ptr, offset, str_cpy);

        BOOST_TEST(alloc_size == (strlen(cstr) + sv.size() + str.size() + 3));  //  3x '\0'
        BOOST_TEST(ptr.get() != nullptr);  // something is allocated

        // string size shall be the same ...
        BOOST_TEST(cstr_cpy.get_sv().size() == strlen(cstr));
        BOOST_TEST(sv_cpy.get_sv().size() == sv.size());
        BOOST_TEST(str_cpy.get_sv().size() == str.size());

        // also string contents ...
        BOOST_TEST(cstr_cpy.get_sv() == cstr);
        BOOST_TEST(sv_cpy.get_sv() == sv);
        BOOST_TEST(str_cpy.get_sv() == str);

        // .. but point to different storage/pointers
        BOOST_TEST(as_void_ptr(cstr_cpy.get_sv().data()) != cstr_ptr);
        BOOST_TEST(as_void_ptr(sv_cpy.get_sv().data()) != sv_ptr);
        BOOST_TEST(as_void_ptr(str_cpy.get_sv().data()) != str_ptr);
    }
}

// ----------------------------------------------------------------------------
//
// Check the correct behavior of TraceValue
//
BOOST_AUTO_TEST_CASE(trace_value)
{
    // construct them

    {
        TraceValue null;
        BOOST_TEST(null.empty() == true);
    }
    {
        TraceValue boolean(true);
        BOOST_TEST(!boolean.empty() == true);

        BOOST_REQUIRE_NO_THROW(std::get<bool>(boolean.data()));
        BOOST_TEST(std::get<bool>(boolean.data()) == true);
    }
    {
        TraceValue integer(42);
        BOOST_TEST(!integer.empty() == true);

        BOOST_REQUIRE_NO_THROW(std::get<int64_t>(integer.data()));
        BOOST_TEST(std::get<int64_t>(integer.data()) == 42);
    }
    {
        TraceValue integer(42LU);
        BOOST_TEST(!integer.empty() == true);

        BOOST_REQUIRE_NO_THROW(std::get<uint64_t>(integer.data()));
        BOOST_TEST(std::get<uint64_t>(integer.data()) == 42);
    }
    {
        TraceValue real(3.14);
        BOOST_TEST(!real.empty() == true);

        BOOST_REQUIRE_NO_THROW(std::get<double>(real.data()));
        BOOST_TEST(std::get<double>(real.data()) == 3.14);
    }
    {
        std::string_view const sv = "Hello World";

        TraceValue string_view(sv);
        BOOST_TEST(!string_view.empty() == true);

        BOOST_REQUIRE_NO_THROW(std::get<char const*>(string_view.data()));
        BOOST_TEST(std::get<char const*>(string_view.data()) == sv);
    }
    {
        char const* cstr = "Hello World";

        TraceValue cstring(cstr);
        BOOST_TEST(!cstring.empty() == true);

        BOOST_REQUIRE_NO_THROW(std::get<char const*>(cstring.data()));
        BOOST_TEST(std::get<char const*>(cstring.data()) == cstr);
    }
    {
        int i = 42;
        void const* const ptr = &i;

        TraceValue pointer(ptr);
        BOOST_TEST(!pointer.empty() == true);

        BOOST_REQUIRE_NO_THROW(std::get<void const*>(pointer.data()));
        BOOST_TEST(std::get<void const*>(pointer.data()) == ptr);
    }
}

// ----------------------------------------------------------------------------
// check the clock_fixture self, there are two instances
// ----------------------------------------------------------------------------
#if 1
BOOST_FIXTURE_TEST_CASE(time_point_fixture_test_1, testsuite::clock_fixture)
{
    {
        auto const t1 = clock::now();
        auto const t2 = clock::now();

        BOOST_TEST(t1 == clock_fixture::as_time_point(clock_fixture::start_count()));

        BOOST_TEST(t2 == clock_fixture::as_time_point(clock_fixture::start_count() +
                                                      clock_fixture::delta_count()));
    }

    clock_fixture::instance().teardown();
    clock_fixture::instance().setup();

    {
        auto const t1 = clock::now();
        auto const t2 = clock::now();

        BOOST_TEST(t1 == clock_fixture::as_time_point(clock_fixture::start_count()));

        BOOST_TEST(t2 == clock_fixture::as_time_point(clock_fixture::start_count() +
                                                      clock_fixture::delta_count()));
    }
}
#endif

// ----------------------------------------------------------------------------
// Test the basic functionality without macros
// ----------------------------------------------------------------------------
BOOST_FIXTURE_TEST_CASE(trace_scope_1, proxy_fixture)
{
    TraceLog::GetInstance().BeginLogging();

    std::cout << "sizeof(TraceEvent) = " << sizeof(TraceEvent) << '\n';

    // scope begin
    {
        static auto const trace_event = category::get("trace_scope_1");
        TraceEndOnScopeClose const trace_event_profileScope(  // --
            trace_event, "trace_scope<end>");

        if (trace_event) {
            AddTraceEvent(TraceEvent::phase::BEGIN,                           // --
                          trace_event.category_name(), "simple_trace_scope",  // --
                          0, TraceEvent::flag::NONE);

            AddTraceEvent(TraceEvent::phase::BEGIN,                       // --
                          trace_event.category_name(), "static strings",  // --
                          0, TraceEvent::flag::NONE,                      // --
                          "optional.arg1", "trace_event_L450");

            AddTraceEvent(TraceEvent::phase::BEGIN,                                       // --
                          trace_event.category_name(), TraceLog::copy("EventName copy"),  // --
                          0, TraceEvent::flag::NONE);

            AddTraceEvent(TraceEvent::phase::BEGIN,                       // --
                          trace_event.category_name(), "Arg1::key copy",  // --
                          0, TraceEvent::flag::NONE,                      // --
                          TraceLog::copy("optional.arg1"), "trace_event_L459");

            AddTraceEvent(TraceEvent::phase::BEGIN,                         // --
                          trace_event.category_name(), "Arg1::value copy",  // --
                          0, TraceEvent::flag::NONE,                        // --
                          "optional.arg1", TraceLog::copy("trace_event_L464"));

            AddTraceEvent(TraceEvent::phase::BEGIN,                                      // --
                          trace_event.category_name(), TraceLog::copy("copy them all"),  // --
                          0, TraceEvent::flag::NONE,                                     // --
                          TraceLog::copy("optional.arg1"), TraceLog::copy("optional.value1"));
        }
    }
    // scope end

    // scope begin
    {
        static auto const trace_event = category::get("trace_scope_2");
        TraceEndOnScopeClose const trace_event_profileScope(  // --
            trace_event, "trace_scope<end>");

        if (trace_event) {
            AddTraceEvent(TraceEvent::phase::BEGIN,                           // --
                          trace_event.category_name(), "trace_scope<start>",  // --
                          0, TraceEvent::flag::NONE,                          // --
                          "compile file", __FILE__);
        }
    }
    // scope end

    {
        TraceLog::GetInstance().AddThreadNameMetadataEvents();
    }

    TraceLog::GetInstance().Flush();
    TraceLog::GetInstance().EndLogging();

    std::cout << "\nResult:\n" << result_str() << "\n";
}

// ----------------------------------------------------------------------------
// threshold test
// ---------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(threshold_test, proxy_fixture)
{
    TraceLog::GetInstance().BeginLogging();

    using namespace std::chrono_literals;

#if 0
    auto const count_value = []() {
        using namespace std::chrono;
        auto const count = testsuite::clock_fixture::instance().count_state();
        return clock::time_point_type(std::chrono::nanoseconds(count));
    };
#endif

    clock::duration_type threshold = 42us;

    // scope begin
    {
        // This event shall be logged

        // implementation of INTERNAL_TRACE_EVENT_ADD_SCOPED_IF_LONGER_THAN()
        static auto const category_proxy = category::get("threshold_test");
        TraceEndOnScopeCloseThreshold scope_guard(category_proxy, "longer_than_42us", threshold);

        if (category_proxy) {
            auto const begin_event_id =
                AddTraceEvent(TraceEvent::phase::BEGIN, category_proxy.category_name(),
                              "maybe_longer_than_42us", 0, TraceEvent::flag::NONE);
            scope_guard.set_threshold_begin_id(begin_event_id);
        };

        clock_fixture::instance().advance(42us);
    }
    // test scope end

    // scope begin
    {
        // This event shall not be logged

        // implementation of INTERNAL_TRACE_EVENT_ADD_SCOPED_IF_LONGER_THAN()
        static auto const category_proxy = category::get("threshold_test");
        TraceEndOnScopeCloseThreshold scope_guard(category_proxy, "shorter_than_42us", threshold);

        if (category_proxy) {
            auto const begin_event_id =
                AddTraceEvent(TraceEvent::phase::BEGIN, category_proxy.category_name(),
                              "shorter_than_42us", 0, TraceEvent::flag::NONE);
            scope_guard.set_threshold_begin_id(begin_event_id);
        };

        clock_fixture::instance().advance(40us);
    }
    // test scope end

    TraceLog::GetInstance().Flush();
    TraceLog::GetInstance().EndLogging();

    std::cout << "\nResult:\n" << result_str() << "\n";
}

// ----------------------------------------------------------------------------
// macro test
// ----------------------------------------------------------------------------
BOOST_FIXTURE_TEST_CASE(trace_macro_expansion, proxy_fixture)
{
    TraceLog::GetInstance().BeginLogging();

    int value = 41;
    {
        // -------------------------------------------------------------------------------------
        TRACE_EVENT0("category_name", "event_name");
        // ... expands to:
#if 0
        static auto const __event_trace_uniq_category_proxy_414 = category::get("category_name");
        TraceEndOnScopeClose __event_trace_uniq_scope_guard_414(__event_trace_uniq_category_proxy_414,
                                                                "event_name");
        if (__event_trace_uniq_category_proxy_414) {
            AddTraceEvent(TraceEvent::phase::BEGIN,
                        __event_trace_uniq_category_proxy_414.category_name(), "event_name", 0,
                        TraceEvent::flag::NONE);
        };
#endif

        // -------------------------------------------------------------------------------------
        TRACE_EVENT_INSTANT0("category_name", "event_name");
        // ... expands to:
#if 0
        do {
            static auto const __event_trace_uniq_category_proxy_428 = category::get("category_name");
            if (__event_trace_uniq_category_proxy_428) {
                AddTraceEvent(TraceEvent::phase::INSTANT,
                            __event_trace_uniq_category_proxy_428.category_name(), "event_name", 0,
                            TraceEvent::flag::NONE);
            }
        } while (0);

#endif

        // -------------------------------------------------------------------------------------
        TRACE_COUNTER1("category_name", "event_name", value++);
        // ... expands to:
#if 0
        do {
            static auto const __event_trace_uniq_category_proxy_452 =
                category::get("category_name");
            if (__event_trace_uniq_category_proxy_452) {
                AddTraceEvent(TraceEvent::phase::COUNTER,
                              __event_trace_uniq_category_proxy_452.category_name(), "event_name",
                              0, TraceEvent::flag::NONE, "value", static_cast<std::int64_t>(42));
            }
        } while (0);
#endif

        // -------------------------------------------------------------------------------------
        TRACE_COUNTER_ID1("category_name", "event_name", 16, value++);
        // ... expands to:
#if 0
        do {
            static auto const __event_trace_uniq_category_proxy_467 =
                category::get("category_name");
            if (__event_trace_uniq_category_proxy_467) {
                TraceEvent::flag trace_event_flags =
                    TraceEvent::flag::NONE | TraceEvent::flag::HAS_ID;
                TraceID trace_event_trace_id(16, trace_event_flags);
                AddTraceEvent(TraceEvent::phase::COUNTER,
                              __event_trace_uniq_category_proxy_467.category_name(), "event_name",
                              trace_event_trace_id.data(), trace_event_flags, "value",
                              static_cast<std::int64_t>(42));
            }
        } while (0);
#endif

        // -------------------------------------------------------------------------------------
        TRACE_EVENT_ASYNC_BEGIN0("category_name", "event_name", 16);
        // ... expands to:
#if 0
        do {
            static auto const __event_trace_uniq_category_proxy_487 =
                category::get("category_name");
            if (__event_trace_uniq_category_proxy_487) {
                TraceEvent::flag trace_event_flags =
                    TraceEvent::flag::NONE | TraceEvent::flag::HAS_ID;
                TraceID trace_event_trace_id(16, trace_event_flags);
                AddTraceEvent(TraceEvent::phase::ASYNC_BEGIN,
                              __event_trace_uniq_category_proxy_487.category_name(), "event_name",
                              trace_event_trace_id.data(), trace_event_flags);
            }
        } while (0);

#endif

        // -------------------------------------------------------------------------------------
        int step = value++;
        TRACE_EVENT_ASYNC_BEGIN_STEP0("category_name", "event_name", 16, step);
        // ... expands to:
#if 0
        do {
            static auto const __event_trace_uniq_category_proxy_507 =
                category::get("category_name");
            if (__event_trace_uniq_category_proxy_507) {
                TraceEvent::flag trace_event_flags =
                    TraceEvent::flag::NONE | TraceEvent::flag::HAS_ID;
                TraceID trace_event_trace_id(16, trace_event_flags);
                AddTraceEvent(TraceEvent::phase::ASYNC_STEP,
                              __event_trace_uniq_category_proxy_507.category_name(), "event_name",
                              trace_event_trace_id.data(), trace_event_flags, "step", step);
            }
        } while (0);
#endif

        // -------------------------------------------------------------------------------------
        TRACE_EVENT_ASYNC_END0("category_name", "event_name", 16);
        // ... expands to:
#if 0
        do {
            static auto const __event_trace_uniq_category_proxy_525 =
                category::get("category_name");
            if (__event_trace_uniq_category_proxy_525) {
                TraceEvent::flag trace_event_flags =
                    TraceEvent::flag::NONE | TraceEvent::flag::HAS_ID;
                TraceID trace_event_trace_id(16, trace_event_flags);
                AddTraceEvent(TraceEvent::phase::ASYNC_END,
                              __event_trace_uniq_category_proxy_525.category_name(), "event_name",
                              trace_event_trace_id.data(), trace_event_flags);
            }
        } while (0);
#endif

        // -------------------------------------------------------------------------------------
        using namespace std::chrono_literals;

        std ::cout << "clock's resolution: " << clock_fixture::ns_resolution().count() << "ns \n";
        clock::duration_type threshold = 42us;
        INTERNAL_TRACE_EVENT_ADD_SCOPED_IF_LONGER_THAN(threshold, "category_name",
                                                       "longer_than_42us");

        clock_fixture::instance().advance(100us);

        // ... expands to:
#if 0
        static auto const __event_trace_uniq_category_proxy_542 = category::get("category_name");
        TraceEndOnScopeCloseThreshold __event_trace_uniq_scope_guard_542(
            __event_trace_uniq_category_proxy_542, "event_name", treshold);
        if (__event_trace_uniq_category_proxy_542) {
            std::int32_t __event_trace_uniq_begin_event_id_542 = AddTraceEvent(
                TraceEvent::phase::BEGIN, __event_trace_uniq_category_proxy_542.category_name(),
                "event_name", 0, TraceEvent::flag::NONE);
            __event_trace_uniq_scope_guard_542.set_threshold_begin_id(
                __event_trace_uniq_begin_event_id_542);
        };

#endif

    }  // test scope end

    TraceLog::GetInstance().Flush();
    TraceLog::GetInstance().EndLogging();

    std::cout << "\nResult:\n" << result_str() << "\n";
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
BOOST_AUTO_TEST_SUITE_END()
