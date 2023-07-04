cmake_minimum_required(VERSION 3.15.0 FATAL_ERROR)

project(nand-runtime-monitor)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)


# Options
set (MONITOR_INTERFACE "func" CACHE STRING "Monitor interface type")
set_property(CACHE MONITOR_INTERFACE PROPERTY STRINGS
    func # Call the monitor function directly
    msgq # Send a message to the monitor
    thinros # Use the thinros monitor interface
    )
message(STATUS "Monitor interface: ${MONITOR_INTERFACE}")

option (MONITOR_DEBUG_MSG "Print debug messages" OFF)
message(STATUS "Debug messages: ${MONITOR_DEBUG_MSG}")

option (MONITOR_BUILD_BENCHMARKS "Build benchmarks" OFF)
message(STATUS "Build benchmarks: ${MONITOR_BUILD_BENCHMARKS}")
##


# Process options
set (MONITOR_C_DEFINITIONS)

if (MONITOR_INTERFACE STREQUAL "func")
    list(APPEND MONITOR_C_DEFINITIONS MONITOR_INTERFACE_FUNC)
elseif (MONITOR_INTERFACE STREQUAL "msgq")
    list(APPEND MONITOR_C_DEFINITIONS MONITOR_INTERFACE_MSGQ)
elseif (MONITOR_INTERFACE STREQUAL "thinros")
    list(APPEND MONITOR_C_DEFINITIONS MONITOR_INTERFACE_THINROS)
else()
    message(FATAL_ERROR "Unknown monitor interface: ${MONITOR_INTERFACE}")
endif()

if (MONITOR_DEBUG_MSG)
    list(APPEND MONITOR_C_DEFINITIONS DEBUG)
endif()

if (MONITOR_BUILD_BENCHMARKS)
    FetchContent_Declare(
        googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG        v1.8.1
    )
    set (BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set (BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    set (BENCHMARK_ENABLE_ASSEMBLY_TESTS OFF CACHE BOOL "" FORCE)
    set (BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

    message(STATUS "Fetching google-benchmark ...")
    FetchContent_MakeAvailable(googlebenchmark)

    set (BENCHMARK_INC_DIR ${googlebenchmark_SOURCE_DIR}/include)
endif()



add_compile_definitions(${MONITOR_C_DEFINITIONS})
##


# Global variables
set (MONITOR_TOP_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (MONITOR_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})
set (MONITOR_TESTRIG_DIR ${MONITOR_TOP_DIR}/testrig)
set (MONITOR_TESTRIG_BUILD_DIR ${MONITOR_BUILD_DIR}/testrig)
set (MONITOR_LIB_DIR ${MONITOR_TOP_DIR}/lib)
set (MONITOR_LIB_BUILD_DIR ${MONITOR_BUILD_DIR}/lib)
##

# Set up testrig
find_program(MAKE
    NAMES make
    REQUIRED
    )

find_file(MONITOR_TESTRIG_MAKEFILE
    NAMES Makefile
    PATHS ${MONITOR_TESTRIG_DIR}
    NO_DEFAULT_PATH
    )
if (NOT MONITOR_TESTRIG_MAKEFILE)
    message(FATAL_ERROR "testrig is missing! Please copy `TestRig-RMStub` to "
        "the directory `testrig` in the root of the project.")
endif()

add_custom_command(
    OUTPUT testrig.clean.tstamp
    DEPENDS ${MONITOR_TESTRIG_MAKEFILE}
    COMMAND ${MAKE} clean
    WORKING_DIRECTORY ${MONITOR_TESTRIG_DIR}
    VERBATIM
    )

add_custom_target(testrig-clean
    COMMAND ${CMAKE_COMMAND} -E remove
        testrig.include.gen.tstamp
        testrig.replace.monitor.state.tstamp
        testrig.replace.monitor.parse.tstamp
    DEPENDS testrig.clean.tstamp
    )

add_custom_target(clean-all)

add_dependencies(clean-all
    testrig-clean
    )
##

# Patch testrig's nand-monitor stub
file(COPY_FILE
    ${MONITOR_LIB_DIR}/rm_parse_stub.c
    ${MONITOR_TESTRIG_DIR}/RuntimeMonitor/rm_parse.c
    ONLY_IF_DIFFERENT
    )

add_custom_command(
    OUTPUT testrig.replace.monitor.parse.tstamp
    BYPRODUCTS ${MONITOR_TESTRIG_DIR}/RuntimeMonitor/rm_parse.c
    DEPENDS ${MONITOR_LIB_DIR}/rm_parse_stub.c
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${MONITOR_LIB_DIR}/rm_parse_stub.c
        ${MONITOR_TESTRIG_DIR}/RuntimeMonitor/rm_parse.c
    VERBATIM
)

file(COPY_FILE
    ${MONITOR_LIB_DIR}/rm_state_stub.c
    ${MONITOR_TESTRIG_DIR}/RuntimeMonitor/rm_state.c
    ONLY_IF_DIFFERENT
    )

add_custom_command(
    OUTPUT testrig.replace.monitor.state.tstamp
    BYPRODUCTS ${MONITOR_TESTRIG_DIR}/RuntimeMonitor/rm_state.c
    DEPENDS ${MONITOR_LIB_DIR}/rm_parse_stub.c
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${MONITOR_LIB_DIR}/rm_state_stub.c
        ${MONITOR_TESTRIG_DIR}/RuntimeMonitor/rm_state.c
    VERBATIM
)

add_custom_target(testrig-patch
    DEPENDS testrig.replace.monitor.parse.tstamp
            testrig.replace.monitor.state.tstamp
    )
##

# Configure testrig
find_file(MONITOR_TESTRIG_INCLUDE_MAKEFILE
    Makefile
    PATHS ${MONITOR_TESTRIG_DIR}/Include
    NO_DEFAULT_PATH
    REQUIRED
    )

add_custom_command(
    OUTPUT testrig.include.gen.tstamp
    DEPENDS ${MONITOR_TESTRIG_INCLUDE_MAKEFILE}
        ${MONITOR_TESTRIG_DIR}/Origins/v16-device_emu.c
        ${MONITOR_TESTRIG_DIR}/Origins/v16-device_emu.h
        ${MONITOR_TESTRIG_DIR}/Scripts/slice-constants.sed
        ${MONITOR_TESTRIG_DIR}/Scripts/transform-constants.sed
        ${MONITOR_TESTRIG_DIR}/Scripts/transform-ns-to-de.sed
        ${MONITOR_TESTRIG_DIR}/Scripts/transform-ns-to-rm.sed
    BYPRODUCTS ${MONITOR_TESTRIG_DIR}/Include/nand_constants.h
        ${MONITOR_TESTRIG_DIR}/Include/de.h
        ${MONITOR_TESTRIG_DIR}/Include/rm.h
    COMMAND ${MAKE}
    COMMAND ${CMAKE_COMMAND} -E touch testrig.include.gen.tstamp
    WORKING_DIRECTORY ${MONITOR_TESTRIG_DIR}/Include
    VERBATIM
    )

add_custom_target(testrig-inc
    DEPENDS testrig.include.gen.tstamp
    )

set (MONITOR_TESTRIG_INCLUDE_DIR ${MONITOR_TESTRIG_DIR}/Include)
##

# Set up Boost.SML
FetchContent_Declare(
    boost_sml
    GIT_REPOSITORY https://github.com/boost-ext/sml.git
    GIT_TAG master
)

message(STATUS "Fetching Boost.SML ...")
FetchContent_MakeAvailable(boost_sml)

set (BOOST_SML_INCLUDE_DIR
    ${boost_sml_SOURCE_DIR}/include)
##


# Build nand-monitor
add_subdirectory(${MONITOR_LIB_DIR} ${MONITOR_LIB_BUILD_DIR})
##

# Build testrig
add_subdirectory(${MONITOR_TESTRIG_DIR} ${MONITOR_TESTRIG_BUILD_DIR})
##

# Build unit tests
add_subdirectory(${MONITOR_TOP_DIR}/tests)
##
