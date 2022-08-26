// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <ibis/event_trace/detail/trace_value.hpp>
#include <ibis/event_trace/trace_event.hpp>
#include <ibis/event_trace/trace_id.hpp>

#include <ibis/util/overloaded.hpp>

#include <fmt/core.h>
#include <range/v3/view/zip.hpp>

namespace ibis::tool::event_trace {

///
/// Small helper class to ensure quoted JSON strings.
///
/// @see
/// - [ECMA](
///    https://www.ecma-international.org/publications-and-standards/standards/ecma-404/)
/// - [IETF](https://datatracker.ietf.org/doc/html/rfc7159)
///
/// Notice the missing quoted '/', see [JSON: why are forward slashes escaped?](
///  https://stackoverflow.com/questions/1580647/json-why-are-forward-slashes-escaped)
///
/// @see Concept: [godbolt.org](https://godbolt.org/z/717eEzTdT)
///
struct jstring {
    jstring(std::string_view str) : contents{ str } {}
    std::string_view const contents;
};

}  // namespace ibis::tool::event_trace

namespace fmt {

namespace event_trace = ibis::tool::event_trace;

template<>
struct formatter<event_trace::jstring> {
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(event_trace::jstring str, FormatContext& ctx) {

        auto const is_control = [](char chr) { return 0x00 <= chr && chr <= 0x1F; };

        // format-clang off
        for (auto const chr : str.contents) {
            switch (chr) {
                case '"':  fmt::format_to(ctx.out(), R"(\")"); break;
                case '\\': fmt::format_to(ctx.out(), R"(\\)"); break;
                case '\b': fmt::format_to(ctx.out(), R"(\b)"); break;
                case '\f': fmt::format_to(ctx.out(), R"(\f)"); break;
                case '\n': fmt::format_to(ctx.out(), R"(\n)"); break;
                case '\r': fmt::format_to(ctx.out(), R"(\r)"); break;
                case '\t': fmt::format_to(ctx.out(), R"(\t)"); break;
                default:
                    if (is_control(chr)) { // UNICODE
                        fmt::format_to(ctx.out(), R"(\u{:0{}X})", static_cast<std::uint64_t>(chr), 4U);
                    }
                    fmt::format_to(ctx.out(), "{}", chr);
            }
        }
        // format-clang on
        return ctx.out();
    }
};

template<>
struct formatter<event_trace::trace_value> {
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(event_trace::trace_value const& value, FormatContext& ctx) {

        using ibis::util::overloaded;
        using event_trace::jstring;

        auto const null = [&]() { return fmt::format_to(ctx.out(), "null"); };

        // format-clang off
        return std::visit(overloaded{
            [&](auto val) { // captures bool, uint64_t and int64_t
                return fmt::format_to(ctx.out(), "{}", val);
            },
            [&](double val) {
                return fmt::format_to(ctx.out(), "{:<.{}}", val, std::numeric_limits<double>::max_digits10);
            },
            [&](char const* cstr) {
                if (cstr == nullptr) { return null(); }
                return fmt::format_to(ctx.out(), R"("{}")", jstring(cstr));
            },
            [&](void const* ptr) {
                if (ptr == nullptr) { return null(); }
                // JSON only supports double and 64-bit integers numbers. So output as a hex string.
                // << hex(static_cast<std::uint64_t>(reinterpret_cast<std::intptr_t>(ptr)), 8)
                // mimics fmt::ptr() to avoid to include <fmt/format.h>
                return fmt::format_to(ctx.out(), R"("0x{}")", std::bit_cast<const void*>(ptr));
            },
            [&]([[maybe_unused]] std::monostate) {
                return null();
            }
        }, value.data());
        // format-clang on
    }
};

template<>
struct formatter<event_trace::TraceID> {
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(event_trace::TraceID const& id, FormatContext& ctx) {

        return fmt::format_to(ctx.out(), R"("0x{:0{}X}")", id.value(), 8U);
    }
};

template<>
struct formatter<event_trace::TraceEvent> {
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format([[maybe_unused]] event_trace::TraceEvent const& event, FormatContext& ctx) {

        // get more complicated as intended due to access to internals, see 
        // TraceEvent::AppendAsJSON()
#if 0
		fmt::format_to(ctx.out(), R"({{ "{}" : "{}")", "key X", event_name);
		if(arg_names[0] != nullptr) { // one or more args
			fmt::format_to(ctx.out(), R"(, "args" : {{ )");
			auto comma = "";
			for(std::tuple<char const*, trace_value> arg : ranges::views::zip(arg_names, arg_values)) {
				if(std::get<0>(arg) == nullptr) { break; }
                fmt::format_to(ctx.out(), R"({}"{}" : {})", comma, std::get<0>(arg), std::get<1>(arg));
				comma = ", ";
			}
			fmt::format_to(ctx.out(), R"(}})");
		}
		if(the_flag > 42) {
			fmt::format_to(ctx.out(), R"(, "{}" : {})", "uuid", uuid);
		}
		fmt::format_to(ctx.out(), " }}");
		if(newline) { fmt::format_to(ctx.out(), "\n"); }
#endif

        return ctx.out();
    }
};

} // namespace fmt
