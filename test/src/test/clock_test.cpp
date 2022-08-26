// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later

#include <ibis/event_trace/detail/clock.hpp>

#include <testsuite/mock_clock.hpp>

#include <boost/test/unit_test.hpp>
#include <boost/test/tools/output_test_stream.hpp>

#include <iostream>

///
/// BOOST TEST requires that the types must be streamable, here we go ...
///
namespace std {

std::ostream& operator<<(std::ostream& os, ibis::tool::event_trace::clock::time_point_type tp)
{
    os << std::chrono::time_point_cast<std::chrono::nanoseconds>(tp).time_since_epoch().count();
    return os;
}

}  // namespace std


// -------------------------------------------------------


BOOST_AUTO_TEST_SUITE(common_instrumentation_utils)

BOOST_AUTO_TEST_CASE(mock_clock)
{
    using clock = ibis::tool::event_trace::clock::time<testsuite::mock::clock::mock_clock>;

    auto const now = clock::now();

    std::cout << now << '\n';
    std::cout << clock::now() << '\n';

    testsuite::clock_fixture::instance().teardown();
    testsuite::clock_fixture::instance().setup();

    std::cout << clock::now() << '\n';
    std::cout << clock::now() << '\n';
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
BOOST_AUTO_TEST_SUITE_END()
