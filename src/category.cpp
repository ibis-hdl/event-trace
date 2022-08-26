//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <ibis/event_trace/category.hpp>

#include <ibis/util/tokenize.hpp>
#include <ibis/util/trim.hpp>

#include <algorithm>
#include <cassert>
#include <iostream>
#include <regex>
#include <string_view>
#include <vector>

namespace {
constexpr bool VERBOSE = false;
}

namespace ibis::tool::event_trace {

category::category() { my_categories.reserve(category::MAX_CATEGORIES); }

//
// ----------------------------------------------------------------------------
// category
// ----------------------------------------------------------------------------
//

void category::set_filter(category::filter const& filter_)
{
    category_filter_ = filter_;
    apply_filter();
}

void category::SetEnabled(std::string_view categories)
{
    category_filter_ = filter(categories);
    apply_filter();
}

void category::apply_filter()
{
    if constexpr (VERBOSE) {
        std::cout << "category: apply filter list '" << category_filter_.list_sv() << "'\n";
    }

    for (auto& entry : my_categories) {
        auto const [match, enable] = category_filter_.result_of(entry.category_name());

        if (match) {
            if constexpr (VERBOSE) {
                std::cout << "  apply result at category '" << entry.category_name()
                          << "': enable: " << std::boolalpha << enable << "\n\n";
            }
            entry.enable(enable);
        }
    }
}

category::proxy category::get_proxy(std::string_view category_name)
{
    static category::entry const categories_exhausted(
        { std::string_view("tracing categories exhausted."), false });

    std::scoped_lock scoped_lock(mutex);

    // Search for pre-existing category matching this name and return it ...
    auto const end = my_categories.cend();
    auto const iter = std::find_if(  // --
        my_categories.cbegin(), end, [&category_name](auto const& cat_entry) {
            return cat_entry.category_name() == category_name;
        });

    if (iter != end) {
        return proxy(*iter);
    }

    // ... otherwise create a new category, enable state depends on category filter
    if (my_categories.size() < MAX_CATEGORIES) {
        // Whether the filter matches the category name is not important here, since we are
        // creating them here - the activation is important. This allows the filtering of the
        // category activities even before creation at this point.
        auto const [match, category_enabled] = category_filter_.result_of(category_name);

        if constexpr (VERBOSE) {
            std::cout << "category: create category '" << category_name << "': "  // --
                      << std::boolalpha << " enable: " << category_enabled << "\n";
        }

        return my_categories.emplace_back(  // --
            std::make_pair(category_name, category_enabled));
    }

    // must increase category::MAX_CATEGORIES
    return proxy(categories_exhausted);
}

std::vector<std::string_view> category::GetKnownCategories() const
{
    std::vector<std::string_view> categories_;
    categories_.reserve(my_categories.size());

    std::scoped_lock scoped_lock(mutex);

    std::transform(my_categories.cbegin(), my_categories.cend(), std::back_inserter(categories_),
                   [](auto const& entry) { return entry.category_name(); });

    return categories_;
}

void category::append(std::initializer_list<value_type> categories)
{
    assert(categories.size() <= category::MAX_CATEGORIES  //--
           && "Initializing from list exceeds capacity!");

    my_categories.insert(my_categories.end(), categories.begin(), categories.end());
}

// static
void category::dump(std::ostream& os)
{
    // FixMe: add category filter
#if 0
    json_ostream json(os);
    json.objectBegin();
    json.attributeBegin("category");
    json.arrayBegin();
    json.object([&]() {
        for (auto const& cat : my_categories) {
            json.attribute(cat.category_name(), cat.enabled());
        }
    });
    json.arrayEnd();
    json.objectEnd();
#endif    
    os << "FixMe: Implement using {fmt} lib\n";
}

//
// ----------------------------------------------------------------------------
// category_filter
// ----------------------------------------------------------------------------
//
category_filter::category_filter(std::size_t size) { regex_list.reserve(size); }

category_filter::category_filter(std::string_view category_regex, std::size_t size)
    : categories(category_regex)
{
    using namespace ibis::util;

    regex_list.reserve(size);

    // helper to create regex objects
    auto const regex = [](std::string_view pattern) {
        switch (pattern.front()) {
            case '-':  // to disable 'marker'
                [[fallthrough]];
            case '+':  // convenience, even if redundant to enable
                pattern.remove_prefix(1);
            default:;  // nothing
        }
        return std::regex(pattern.begin(), pattern.end(), std::regex_constants::ECMAScript);
    };

    // Tokenize list of categories, delimited by ','.
    static constexpr char delimiter = ',';
    std::size_t start = 0;
    std::size_t end = 0;

    // Maybe wait for C++20, see [split_view](https://en.cppreference.com/w/cpp/ranges/split_view)
    // ```std::ranges::for_each(hello | std::ranges::views::split(' '), print);```
    // where print is a lambda. Even ranges-v3 has for_each, but seems to have different syntax!
    while ((start = category_regex.find_first_not_of(delimiter, end)) != std::string_view::npos) {
        end = category_regex.find(delimiter, start);
        auto const pattern = trim(category_regex.substr(start, end - start));

        try {
            regex_list.emplace_back(pattern, regex(pattern));
        }
        catch (std::regex_error const& e) {
            std::cerr << "category filter regex '" << pattern << "' error caught: "  // --
                      << e.what() << '\n';
            regex_list.pop_back();
            // FixMe: Maybe rethrow??
        }
    }
}

std::pair<bool, bool> category_filter::result_of(std::string_view category_name) const
{
    // default behavior on empty filter RE
    bool match = false;
    bool enable = true;

    if (regex_list.empty()) {
        if constexpr (VERBOSE) {
            std::cout << "  RE empty filter pattern applied on category '"  // --
                      << category_name << "' (" << std::boolalpha << "enable: " << enable << ")\n";
        }
        return { match, enable };
    }

    for (auto const& [regex_sv, regex] : regex_list) {
        match = std::regex_search(category_name.begin(), category_name.end(), regex);
        if (match) {
            // check to disable category
            if (regex_sv.front() == '-') {
                enable = false;
            }
            if constexpr (VERBOSE) {
                std::cout << "  RE filter pattern '" << regex_sv << "' matches on category '"
                          << category_name << "' " << std::boolalpha  //--
                          << "(enable: " << enable << ")\n";
            }
            // don't search further, first match wins
            break;
        }
    }

    return { match, enable };
}

}  // namespace ibis::tool::event_trace
