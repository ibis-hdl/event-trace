//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <chrono>

namespace ibis::tool::event_trace::clock {

using clock_type = std::chrono::steady_clock;
using time_point_type = clock::clock_type::time_point; //std::chrono::time_point<clock_type>;
using duration_type = clock::time_point_type::duration;

/// Convenience value for no duration to initialize the timestamp in TraceEvent with the
/// correct type.
static inline constexpr auto time_point_zero =
    std::chrono::time_point<clock_type, std::chrono::nanoseconds>(std::chrono::nanoseconds(0));

/// Convenience value for threshold's duration.
static inline constexpr auto duration_zero = clock_type::duration::zero();

///
/// clock source for generating time stamps.
///
/// @see Concept [Coliru](https://coliru.stacked-crooked.com/a/9374003ad8dc61c8)
template<typename clock_impl = clock_type>
struct time {
    static time_point_type now() noexcept { return clock_impl::now(); }
};

}  // namespace ibis::tool::event_trace::clock
