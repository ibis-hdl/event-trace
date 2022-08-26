//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <ibis/event_trace/detail/clock.hpp>

#include <iostream>
#include <memory>
#include <cassert>

namespace testsuite {

class clock_fixture {
private:
    /// counter guard, which increments by @arg increment the counter at *destruction* time. This is
    /// primary convenience function. This gives a beauty start point counts since the previous
    /// count value get returned before the value is incremented..
    template <typename T>
    struct increment_guard {
        increment_guard(T& counter_, T increment_)
            : value{ counter_ }
            , increment{ increment_ }
        {
        }

        ~increment_guard() noexcept { value += increment; }

        T& value;
        T const increment;
    };

public:
    // FIXME: ctor private etc.
    static clock_fixture& instance()
    {
        static clock_fixture static_instance;
        return static_instance;
    }

    void setup() { count_value = START; }
    void teardown() {}

public:
    std::int64_t count_state() const { return count_value; }

    std::int64_t count()
    {
        auto guard = increment_guard{ count_value, count_increment };
        return count_value;
    }

    constexpr std::int64_t start_count() const { return START; }
    constexpr std::int64_t delta_count() const { return RESOLUTION; }

    static ibis::tool::event_trace::clock::time_point_type as_time_point(std::int64_t t_ns)
    {
        using namespace ibis::tool::event_trace;

        return clock::time_point_type(std::chrono::nanoseconds(t_ns));
    }

    static ibis::tool::event_trace::clock::duration_type ns_resolution()
    {
        using namespace std::chrono;
        auto const delta = as_time_point(START + RESOLUTION) - as_time_point(START);
        return duration_cast<nanoseconds>(delta);
    }

    void advance(ibis::tool::event_trace::clock::duration_type duration_adjust)
    {
        using namespace std::chrono;
        using namespace ibis::tool::event_trace;

        auto const fw = clock::time_point_type(duration_adjust);
        count_value += time_point_cast<nanoseconds>(fw).time_since_epoch().count();
    }

private:
    static constexpr std::int64_t START = 0;        // 1'000'000;
    static constexpr std::int64_t RESOLUTION = 10;  // eq. 10ns

private:
    static inline std::int64_t count_value = START;
    std::int64_t count_increment = RESOLUTION;
};

namespace mock::clock {

    using clock_type = ibis::tool::event_trace::clock::clock_type;
    using time_point_type = ibis::tool::event_trace::clock::time_point_type;
    using duration_type = ibis::tool::event_trace::clock::duration_type;

    struct mock_clock 
    {
        static time_point_type now() noexcept
        {
            using std::chrono::nanoseconds;

            auto const count = testsuite::clock_fixture::instance().count();
            return time_point_type(nanoseconds(count));
        }
    };

}

}  // namespace testsuite
