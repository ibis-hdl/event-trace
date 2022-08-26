// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <type_traits>

namespace testsuite::util {

namespace detail {

// concept see: https://coliru.stacked-crooked.com/a/939a32029ef1a98f

template<typename CharT>
concept SignedChar = std::is_same_v<CharT, char> && std::is_signed_v<CharT>;

template<typename CharT>
concept UnSignedChar = std::is_same_v<CharT, char> && !std::is_signed_v<CharT>;

template <class... T>
constexpr bool always_false = false;

template<typename T>
struct promote_char {
    static_assert(always_false<T>, "T not supported");
};

template<UnSignedChar CharT>
struct promote_char<CharT> {
    using type = std::uint64_t;  
};


template<SignedChar CharT>
struct promote_char<CharT> {
    using type = std::int64_t;  
};

} // namespace detail

template<typename T>
using promote_char_t = typename detail::promote_char<T>::type;

} // namespace testsuite::util
