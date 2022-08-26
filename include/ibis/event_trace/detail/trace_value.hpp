//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <variant>
#include <string_view>
#include <type_traits>
#include <cstdint>
#include <cstring>
#include <cassert>

namespace ibis::tool::event_trace {

template<typename T>
concept UnsignedIntegerT = !std::is_same_v<bool, T> && std::is_integral_v<T> && std::is_unsigned_v<T>;

template<typename T>
concept SignedIntegerT = !std::is_same_v<bool, T> && std::is_integral_v<T> && std::is_signed_v<T>;

class trace_value {
public:
    /// The variant type of the types supported as arguments.
    /// The ```std::variant``` is able to hold more than these values
    /// @see Concept at [Coliru](https://coliru.stacked-crooked.com/a/cf7664e5aab1e340) and
    /// basic std::variant testing done at
    /// [Coliru](https://coliru.stacked-crooked.com/a/5838b6bed8339df1)
    ///
    /// MSVC is the reason for overloading the constructor this way, since 
    /// @code{.cpp}
    /// trace_value integer(42UL);
    /// @endcode
    /// like [here](https://godbolt.org/z/PoK55MxEe) fails to compile (I'm not sure about why).
    /// Unsigned long differs on platform Unix/Windows (8 vs. 4 Bytes).
    /// @see [Compiler Explorer](https://godbolt.org/z/48xac31rs) 
    ///
    using variant_type = std::variant<  // --
        std::monostate,                 // mark default constructible empty state
        bool, std::uint64_t, std::int64_t, double, char const *, void const *>;

public:
    trace_value() = default;
    trace_value(trace_value const&) = default;
    trace_value &operator=(trace_value const&) = default;

    trace_value(trace_value &&) = default;
    trace_value &operator=(trace_value &&) = default;

    trace_value(auto value_)
        : value{ value_ }
    {
    }

    trace_value(UnsignedIntegerT auto value_)
        : value{ std::uint64_t{value_} }
    {
    }

    trace_value(SignedIntegerT auto value_)
        : value{ std::int64_t{value_} }
    {
    }

    trace_value(std::string_view sv)
        : value{ sv.data() }
    {
    }

    ~trace_value() = default;

    bool empty() const
    {
        // test on std::monostate which marks the empty state
        return value.index() == 0;
    }

    variant_type const &data() const { return value; }

private:
    variant_type value;
};

}  // namespace ibis::tool::event_trace
