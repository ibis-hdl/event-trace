//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <ibis/event_trace/trace_log.hpp>
#include <ibis/event_trace/trace_event.hpp>

#include <testsuite/namespace_alias.hpp>

#include <boost/test/unit_test.hpp>
#include <boost/test/tools/output_test_stream.hpp>

namespace testsuite::mock {

struct TraceEvent {
    using storage_ptr = ibis::tool::event_trace::TraceEvent::storage_ptr;
};

struct TraceLog {
    using copy = ibis::tool::event_trace::TraceLog::copy;

    template <typename T>
    static std::size_t accumulate_size(std::size_t& alloc_size, T str) {
        return ibis::tool::event_trace::TraceLog::accumulate_size(alloc_size, str);
    }
    
    static TraceEvent::storage_ptr allocate(std::size_t alloc_size)
    {
        return ibis::tool::event_trace::TraceLog::allocate(alloc_size);
    }

    template <typename T>
    static void deep_copy(TraceEvent::storage_ptr& ptr, std::size_t& offset, T& str)
    {
        ibis::tool::event_trace::TraceLog::deep_copy(ptr, offset, str);
    }    
};

} // namespace testsuite::mock

BOOST_AUTO_TEST_SUITE(common_instrumentation_utils)

//
// Check the correct behavior of TraceLog::copy() capabilities. Since the relevant members are
// private, a mock class is used. The code below mimics the behaviour of
// TraceLog::AddTraceEvent().
//
BOOST_AUTO_TEST_CASE(copy_marker)
{
    namespace mock = testsuite::mock;

    using namespace std::literals;

    char const cstr[] = "(Hello World cstr)";
    std::string_view const sv = "[Hello World sv]";
    std::string const str = "{Hello World str}";

    // small helper to get char type pointer type pruned as void address pointer to
    // compare/check
    auto const as_void_ptr = [](auto const* str) { return static_cast<void const* const>(str); };

    void const* const cstr_ptr = as_void_ptr(cstr);
    void const* const sv_ptr = as_void_ptr(sv.data());
    void const* const str_ptr = as_void_ptr(str.data());

    // ------------------------------------------------------------------------
    // test if there is no TraceLog::copy, shall be left as is
    // ------------------------------------------------------------------------
    {
        std::size_t alloc_size = 0;
        std::size_t offset = 0;

        mock::TraceLog::accumulate_size(alloc_size, cstr);
        mock::TraceLog::accumulate_size(alloc_size, sv);
        mock::TraceLog::accumulate_size(alloc_size, str);

        mock::TraceEvent::storage_ptr ptr = mock::TraceLog::allocate(alloc_size);

        mock::TraceLog::deep_copy(ptr, offset, cstr);
        mock::TraceLog::deep_copy(ptr, offset, sv);
        mock::TraceLog::deep_copy(ptr, offset, str);

        BOOST_TEST(alloc_size == 0);
        BOOST_TEST(offset == 0);
        BOOST_TEST(!ptr);  // nullptr

        // still point to origin
        BOOST_TEST(as_void_ptr(cstr) == cstr_ptr);
        BOOST_TEST(as_void_ptr(sv.data()) == sv_ptr);
        BOOST_TEST(as_void_ptr(str.data()) == str_ptr);
    }

    // ------------------------------------------------------------------------
    // now with TraceLog::copy
    // ------------------------------------------------------------------------
    {
        mock::TraceLog::copy cstr_cpy(cstr);
        mock::TraceLog::copy sv_cpy(sv);
        mock::TraceLog::copy str_cpy(str);

        std::size_t alloc_size = 0;
        std::size_t offset = 0;

        mock::TraceLog::accumulate_size(alloc_size, cstr_cpy);
        mock::TraceLog::accumulate_size(alloc_size, sv_cpy);
        mock::TraceLog::accumulate_size(alloc_size, str_cpy);

        mock::TraceEvent::storage_ptr ptr = mock::TraceLog::allocate(alloc_size);

        mock::TraceLog::deep_copy(ptr, offset, cstr_cpy);
        mock::TraceLog::deep_copy(ptr, offset, sv_cpy);
        mock::TraceLog::deep_copy(ptr, offset, str_cpy);

        BOOST_TEST(alloc_size == (strlen(cstr) + sv.size() + str.size() + 3));  //  3x '\0'
        BOOST_TEST(ptr.get() != nullptr);  // something is allocated

        // string size shall be the same ...
        BOOST_TEST(cstr_cpy.get_sv().size() == strlen(cstr));
        BOOST_TEST(sv_cpy.get_sv().size() == sv.size());
        BOOST_TEST(str_cpy.get_sv().size() == str.size());

        // also string contents ...
        BOOST_TEST(cstr_cpy.get_sv() == cstr);
        BOOST_TEST(sv_cpy.get_sv() == sv);
        BOOST_TEST(str_cpy.get_sv() == str);

        // .. but point to different storage/pointers
        BOOST_TEST(as_void_ptr(cstr_cpy.get_sv().data()) != cstr_ptr);
        BOOST_TEST(as_void_ptr(sv_cpy.get_sv().data()) != sv_ptr);
        BOOST_TEST(as_void_ptr(str_cpy.get_sv().data()) != str_ptr);
    }
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
BOOST_AUTO_TEST_SUITE_END()
