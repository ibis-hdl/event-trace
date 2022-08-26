//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <ibis/event_trace/detail/platform.hpp>

#include <cassert>
#include <vector>
#include <string_view>
#include <mutex>
#include <tuple>
#include <vector>
#include <memory>
#include <regex>
#include <initializer_list>

namespace ibis::tool::event_trace {

class category_filter {
private:
    using value_type = std::pair<std::string_view, std::regex>;

    static constexpr std::size_t RESERVE_SZ = 10;

public:
    ///
    /// Construct a new filter object.
    ///
    /// @param category_regex A string(view) of a comma separated list with regular expressions to
    /// activate the categories. Using a leading '-' disables the matching category, otherwise it's
    /// enabled. For convenience an optional leading '+' activates the category too (which is of
    /// course redundant). It's using the ECMAScript syntax for RE.
    /// The regular expression can be expressed to match several times the same category in the
    /// comma separated list, but the first match wins, discarding the later one.
    ///
    category_filter(std::string_view category_regex, std::size_t size = RESERVE_SZ);

    ~category_filter() = default;
    category_filter(std::size_t size = RESERVE_SZ);
    category_filter(category_filter const&) = default;
    category_filter& operator=(category_filter const&) = default;
    category_filter(category_filter&&) = default;
    category_filter& operator=(category_filter&&) = default;

public:
    /// return the original categories RE expression string(view)
    std::string_view list_sv() const { return categories; }

    // the the number of RE filter expressions.
    std::size_t count() const { return regex_list.size(); }

    /// check the @a category_name against the regular expression, if it shall enabled or disabled.
    /// If default constructed, no given category_regex at construction time, it returns true to
    /// enable all given @category_name.
    ///
    /// @note The first match in the regex list wins. This means that the first match in the regex
    /// pattern list within the category activates/deactivates it, regardless of further entries
    /// that might give different results for that category.
    ///
    /// @param category_name Name of the category to test against the RE stored.
    /// @return std::pair<bool, bool> Pair of <match, enable>, if pattern matches 1st boolean is
    /// true and if enabled the 2nd is also true. otherwise false.
    ///
    std::pair<bool, bool> result_of(std::string_view category_name) const;

    /// Syntactical sugar for matches.
    ///
    /// @param category_name Name of the category to test against the RE stored.
    /// @return true If the RE matches and category is enabled.
    /// @return false otherwise.
    ///
    bool operator()(std::string_view category_name) const
    {
        auto const [match, enable] = result_of(category_name);
        return match && enable;
    }

private:
    std::string_view categories;
    std::vector<value_type> regex_list;
};

// new concept/API https://coliru.stacked-crooked.com/a/a164f90675e3cd1c
class category {
private:
    category();
    ~category() = default;

public:
    using value_type = std::pair<std::string_view, bool>;
    class proxy;
    class entry;
    using filter = category_filter;

public:
    static category& instance()
    {
        static category static_instance;
        return static_instance;
    };

public:
    /// Initialize and append a given @a categories list to the internal category list. The
    /// initializer list is of pairs of category name and boolean enable state.
    void append(std::initializer_list<value_type> categories);

public:
    /// Get the proxy object for the @a category_name. If the @a category_name exist in the registry
    /// the proxy object is returned from this. Otherwise it's append to the internal category list.
    /// Herby the @a category_name is check against the filter RE object from set_filter to set the
    /// enabled state.
    static category::proxy get(std::string_view category_name);

public:
    /// Get set of known categories. This can change as new code paths are reached.
    /// @todo [C++20] This is a use case [iterator_facade in C++20](
    ///  https://vector-of-bool.github.io/2020/06/13/cpp20-iter-facade.html)
    std::vector<std::string_view> GetKnownCategories() const;

    std::size_t GetKnownCategoriesCount() const { return my_categories.size(); }

    /// FixMe: description obsolete; Enable tracing for provided list of category. If tracing is
    /// already enabled, this method does nothing -- changing categories during trace is not
    /// supported. Regular expressions are supported.
    ///
    /// @param categories is a comma-delimited list of category wildcards.
    void SetEnabled(std::string_view categories);

    void set_filter(category::filter const& filter);

public:
    // dump the internals for debugging purpose.
    static void dump(std::ostream& os);

private:
    category::proxy get_proxy(std::string_view name);

    void apply_filter();

private:
    static constexpr std::size_t MAX_CATEGORIES = 100;

private:
    /// FixMe: std::recursive_mutex required???
    std::mutex mutable mutex;

    static inline std::vector<category::entry> my_categories;

    category::filter category_filter_;

    std::vector<category::entry> category_pattern;
};

class category::entry {
    friend category::proxy;

public:
    using value_type = category::value_type;

public:
    entry(value_type pair)
        : data(std::move(pair))
    {
    }

    ~entry() = default;
    entry(entry&&) = default;
    entry& operator=(entry&&) = default;

    entry() = delete;
    entry(const entry&) = default;
    entry& operator=(const entry&) = delete;

public:
    /// get the category name.
    std::string_view category_name() const { return data.first; }

    /// get category name's active state.
    explicit operator bool() const { return data.second; }

    /// get category name's active state.
    bool enabled() const { return data.second; }

    /// enable the category entry
    void enable(bool enabled) { data.second = enabled; }

private:
    value_type data;
};

// Concept: https://coliru.stacked-crooked.com/a/131880fa10af40c1
class category::proxy {
public:
    using value_type = std::pair<std::string_view, bool const&>;

public:
    proxy(category::entry const& entry)
        : data(entry.data.first, entry.data.second)
    {
    }

    ~proxy() = default;

    proxy() = delete;
    proxy(const proxy&) = default;
    proxy& operator=(const proxy&) = delete;

    proxy(proxy&&) = default;
    proxy& operator=(proxy&&) = delete;

public:
    /// get the category name.
    std::string_view category_name() const { return data.first; }

    /// get category name's active state.
    explicit operator bool() const { return data.second; }

    /// get category name's active state.
    bool enabled() const { return data.second; }

private:
    value_type const data;
};

inline category::proxy category::get(std::string_view name)
{
    return category::instance().get_proxy(name);
}

}  // namespace ibis::tool::event_trace
