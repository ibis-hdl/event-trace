//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <ibis/event_trace/category.hpp>

#include <testsuite/namespace_alias.hpp>

#include <ibis/util/overloaded.hpp>

#include <boost/test/unit_test.hpp>
#include <boost/test/tools/output_test_stream.hpp>

#include <iomanip>
#include <limits>
#include <iostream>

///
/// BOOST TEST requires that the types must be streamable, here we go ...
///
namespace std {

std::ostream& operator<<(std::ostream& os, std::pair<bool, bool> pair)
{
    os << "std::pair(" << std::boolalpha << pair.first << ", " << pair.second << ")";
    return os;
}

}  // namespace std

BOOST_AUTO_TEST_SUITE(common_instrumentation_utils)

//
// The filter to select which category entries are allowed to log. The filter supports RE - for
// detailed RE check, use [Regex101](https://regex101.com/) Here we test only the basic concept
// since we trust in std::regex
//
BOOST_AUTO_TEST_CASE(category_filter)
{
    using ibis::tool::event_trace::category;

    category::filter filter("foo, -bar");

    BOOST_TEST(filter.count() == 2);

    BOOST_TEST(filter.result_of("foo") == std::pair(true, true));
    BOOST_TEST(filter.result_of("foot") == std::pair(true, true));
    BOOST_TEST(filter.result_of("barfooted") == std::pair(true, true));  // 1st match wins!
    BOOST_TEST(filter.result_of("bar") == std::pair(true, false));
    BOOST_TEST(filter.result_of("bart") == std::pair(true, false));
    BOOST_TEST(filter.result_of("centibar") == std::pair(true, false));
}

//
// Test the proxy of catalog's entry items.
//
BOOST_AUTO_TEST_CASE(category_proxied_entries)
{
    using ibis::tool::event_trace::category;

    // test helper to check proxy basic functionality by visual debugging
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

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
BOOST_AUTO_TEST_SUITE_END()
