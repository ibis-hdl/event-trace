################################################################################
## IBIS/testsuite event trace project
##
## file: testsuite/event_trace/CMakeLists.txt
################################################################################

project(testrunner_event_trace LANGUAGES CXX)


add_executable(${PROJECT_NAME})


add_test(NAME test_event_trace
    COMMAND
        ${PROJECT_NAME} --log_level=error
)


set_property(TEST test_event_trace
    PROPERTY
        ENVIRONMENT LD_PRELOAD=${CMAKE_BINARY_PATH}/lib/libmock.so
)


target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ibis::util
        ibis::event_trace
        Boost::unit_test_framework
        mock
)
#set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-v")


target_sources(${PROJECT_NAME}
    PRIVATE
        src/test_event_trace.cpp
        src/test/category_test.cpp
        src/test/trace_value_test.cpp
        src/test/trace_log_test.cpp
        src/test/clock_test.cpp
        src/test/simple_test.cpp
        #src/test/basic_test.cpp
)


# instrumentation depends on mock; no Win32 since DLL Injection isn't here yet.
add_dependencies(${PROJECT_NAME} mock)


#set_source_files_properties(src/test_event_trace.cpp
#    APPEND PROPERTIES
#        # Failed to link Boost UTF with Windows/Clang, Linux works
#        COMPILE_DEFINITIONS "$<$<NOT:$<PLATFORM_ID:Windows>>:BOOST_TEST_DYN_LINK>"
#)
# [Boost.Test linker error by use with precompiled headers (PCH)](
#  https://stackoverflow.com/questions/66535719/boost-test-linker-error-by-use-with-precompiled-headers-pch)
set_source_files_properties(src/test_event_trace.cpp
    PROPERTIES
        SKIP_PRECOMPILE_HEADERS ON
)


if (IBIS_ENABLE_PCH)
    target_precompile_headers(${PROJECT_NAME}
        PRIVATE

    )
endif()


target_include_directories(${PROJECT_NAME}
    PRIVATE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)


target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        # don't allow automatically select which libraries to link against
        $<$<CXX_COMPILER_ID:MSVC>:BOOST_ALL_NO_LIB>
)


################################################################################
# Mock std::steady_clock(), getpid() etc.
#
# FixMe: Windows Support, see
# [What happens to global and static variables in a shared library when it is dynamically linked?](
#  https://stackoverflow.com/questions/19373061/what-happens-to-global-and-static-variables-in-a-shared-library-when-it-is-dynam)
################################################################################
add_library(mock STATIC
    src/mock_dummy.cpp
    #src/mock_std_clock.cpp
    #src/mock_unistd.cpp
    #src/mock_pthread.cpp
)


target_link_libraries(mock
    PRIVATE
        ibis::util
        ibis::event_trace
)


target_include_directories(mock
    PRIVATE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)
