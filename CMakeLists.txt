################################################################################
## IBIS/event_trace project
##
## file: source/event_trace/CMakeLists.txt
################################################################################

project(ibis_event_trace LANGUAGES CXX)


add_library(${PROJECT_NAME})
add_library(ibis::event_trace ALIAS ${PROJECT_NAME})


target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ibis::util
        Boost::headers
    PRIVATE
        fmt::fmt
        range-v3::range-v3
)


target_sources(${PROJECT_NAME}
    PRIVATE
        src/category.cpp
        src/trace_event.cpp
        src/trace_log.cpp
        src/event_trace.cpp
)


if(${PROJECT_NAME}_PCH)
    # override ibis_pch helper
    target_precompile_headers(${PROJECT_NAME}
        PRIVATE
            $<$<BOOL:${IBIS_ENABLE_PCH_DEFAULT}>:${PROJECT_SOURCE_DIR}/include/ibis/event_trace/detail/pch_default.hpp>
            $<$<BOOL:${IBIS_ENABLE_PCH_IBIS}>:${PROJECT_SOURCE_DIR}/include/ibis/event_trace/detail/pch_ibis.hpp>
    )
elseif(IBIS_ENABLE_PCH_DEFAULT OR IBIS_ENABLE_PCH_IBIS)
    target_precompile_headers(${PROJECT_NAME} REUSE_FROM ibis_pch)
endif()


target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:include>
)


if (IBIS_BUILD_TESTS)
    add_subdirectory(test)
endif()
