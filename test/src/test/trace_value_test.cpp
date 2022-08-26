//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <ibis/event_trace/detail/trace_value.hpp>

#include <testsuite/type_traits.hpp>
#include <testsuite/namespace_alias.hpp>

#include <ibis/util/overloaded.hpp>

#include <boost/test/unit_test.hpp>

#include <iomanip>
#include <limits>

///
/// BOOST TEST requires that the types must be streamable, here we go ...
///
namespace std {

std::ostream& operator<<(std::ostream& os, ibis::tool::event_trace::trace_value const& value)
{
    using ibis::util::overloaded;

    std::visit(  // --
        overloaded{
            [&os]([[maybe_unused]] std::monostate v) { os << "trace_value<std::monostate>"; },
            [&os]([[maybe_unused]] bool v) { os << "trace_value<bool>"; },
            [&os]([[maybe_unused]] uint64_t v) { os << "trace_value<uint64_t>"; },
            [&os]([[maybe_unused]] int64_t v) { os << "trace_value<int64_t>"; },
            [&os]([[maybe_unused]] double v) { os << "trace_value<double>"; },
            [&os]([[maybe_unused]] std::string_view sv) { os << "trace_value<std::string_view>"; },
            [&os]([[maybe_unused]] void const* ptr) { os << "trace_value<pointer>"; },
        },
        value.data());
    return os;
}

}  // namespace std

BOOST_AUTO_TEST_SUITE(common_instrumentation_utils)

//
// Check the correct behavior of trace_value
//
BOOST_AUTO_TEST_CASE(trace_value)
{
    using ibis::tool::event_trace::trace_value;

    // construct them

    {
        trace_value null{};
        BOOST_TEST(null.empty() == true);
    }
    {
        trace_value boolean(true);
        BOOST_TEST(!boolean.empty() == true);

        BOOST_REQUIRE_NO_THROW([[maybe_unused]] auto discard = std::get<bool>(boolean.data()));
        BOOST_TEST(std::get<bool>(boolean.data()) == true);
    }
    {
        trace_value char_(char{'X'});
        BOOST_TEST(!char_.empty() == true);

        using get_type = testsuite::util::promote_char_t<char>;

        BOOST_REQUIRE_NO_THROW([[maybe_unused]] auto discard = std::get<get_type>(char_.data()));
        BOOST_TEST(std::get<get_type>(char_.data()) == 'X');
    }
    {
        std::int16_t const val = 42;
        trace_value integer(val);
        BOOST_TEST(!integer.empty() == true);

        BOOST_REQUIRE_NO_THROW([[maybe_unused]] auto discard = std::get<int64_t>(integer.data()));
        BOOST_TEST(std::get<int64_t>(integer.data()) == val);
    }
    {
        trace_value integer(42);
        BOOST_TEST(!integer.empty() == true);

        BOOST_REQUIRE_NO_THROW([[maybe_unused]] auto discard = std::get<int64_t>(integer.data()));
        BOOST_TEST(std::get<int64_t>(integer.data()) == 42);
    }
    {
        trace_value integer(42UL);
        BOOST_TEST(!integer.empty() == true);

        BOOST_REQUIRE_NO_THROW([[maybe_unused]] auto discard = std::get<uint64_t>(integer.data()));
        BOOST_TEST(std::get<uint64_t>(integer.data()) == 42);
    }
    {
        trace_value integer(42ULL);
        BOOST_TEST(!integer.empty() == true);

        BOOST_REQUIRE_NO_THROW([[maybe_unused]] auto discard = std::get<uint64_t>(integer.data()));
        BOOST_TEST(std::get<uint64_t>(integer.data()) == 42);
    }
    {
        trace_value real(3.14);
        BOOST_TEST(!real.empty() == true);

        BOOST_REQUIRE_NO_THROW([[maybe_unused]] auto discard = std::get<double>(real.data()));
        BOOST_TEST(std::get<double>(real.data()) == 3.14);
    }
    {
        std::string_view const sv = "Hello World";

        trace_value string_view(sv);
        BOOST_TEST(!string_view.empty() == true);

        BOOST_REQUIRE_NO_THROW([[maybe_unused]] auto discard = std::get<char const*>(string_view.data()));
        BOOST_TEST(std::get<char const*>(string_view.data()) == sv);
    }
    {
        char const* cstr = "Hello World";

        trace_value cstring(cstr);
        BOOST_TEST(!cstring.empty() == true);

        BOOST_REQUIRE_NO_THROW([[maybe_unused]] auto discard = std::get<char const*>(cstring.data()));
        BOOST_TEST(std::get<char const*>(cstring.data()) == cstr);
    }
    {
        char const cstr[] = "Hello World";

        trace_value char_array(cstr);
        BOOST_TEST(!char_array.empty() == true);

        BOOST_REQUIRE_NO_THROW([[maybe_unused]] auto discard = std::get<char const*>(char_array.data()));
        BOOST_TEST(std::get<char const*>(char_array.data()) == cstr);
    }
    {
        int i = 42;
        void const* const ptr = &i;

        trace_value pointer(ptr);
        BOOST_TEST(!pointer.empty() == true);

        BOOST_REQUIRE_NO_THROW([[maybe_unused]] auto discard = std::get<void const*>(pointer.data()));
        BOOST_TEST(std::get<void const*>(pointer.data()) == ptr);
    }
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
BOOST_AUTO_TEST_SUITE_END()
