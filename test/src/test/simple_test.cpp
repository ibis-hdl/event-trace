//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <ibis/event_trace/trace_log.hpp>
#include <ibis/event_trace/trace_event.hpp>
#include <ibis/event_trace/scoped_event.hpp>
#include <ibis/event_trace/event_trace.hpp>

#include <testsuite/mock_clock.hpp>
#include <testsuite/namespace_alias.hpp>

#include <boost/test/unit_test.hpp>
#include <boost/test/tools/output_test_stream.hpp>

namespace testsuite {

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
        ots = std::make_unique<btt::output_test_stream>();
    }
    void teardown()
    {
        // std::cout << "Callback Fixture Teardown\n";
    }

    void output(std::string_view sv) { *ots << sv; };

    std::string result_str() const
    {
        assert(ots != nullptr);
        return ots->str();
    }

private:
    static inline std::unique_ptr<btt::output_test_stream> ots = nullptr;
};

///
/// Global fixture for TraceLog's OutputCallback, called once abd only at setup time of whole test 
/// suite.
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

///
/// Boost.Test allows only one fixture at each test case. Following the documentation at
/// [Single test case fixture](
///  https://www.boost.org/doc/libs/1_80_0/libs/test/doc/html/boost_test/tests_organization/fixtures/case.html#boost_test.tests_organization.fixtures.case.f0)
/// footnote #4: "it is still possible to define a class inheriting from several fixtures, that 
/// will act as a proxy fixture."
///
struct testcase_fixture
    : public clock_fixture
    , public callback_fixture 
{
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

} // namespace testsuite

// -------------------------------------------------------------------------------------------------
BOOST_AUTO_TEST_SUITE(common_instrumentation_utils)

using testsuite::callback_initializer_fixture;
using testsuite::testcase_fixture;

BOOST_TEST_GLOBAL_FIXTURE(callback_initializer_fixture);

BOOST_FIXTURE_TEST_CASE(simple_trace, testcase_fixture)
{
    namespace event_trace = ::ibis::tool::event_trace;
    using event_trace::TraceLog;
    using event_trace::TraceEvent;
    using event_trace::category;

    TraceLog::GetInstance().BeginLogging();

    {
        static auto const category_enabled = category::get("compile_scope");
        auto const scope = event_trace::scope_guard(category_enabled, "compile end");

        if (category_enabled) {
            AddTraceEvent(TraceEvent::phase::BEGIN,                      // --
                          category_enabled.category_name(), "compile start",  // --
                          0, TraceEvent::flag::NONE,                     // --
                          "compile file", "ibis.cpp");
        }
    }  // test scope end

    TraceLog::GetInstance().Flush();
    TraceLog::GetInstance().EndLogging();

    std::cout << "\nResult:\n" << result_str() << "\n";
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
BOOST_AUTO_TEST_SUITE_END()
