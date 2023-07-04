cmake_minimum_required(VERSION 3.15)

include_directories(
    ${MONITOR_LIB_DIR}
    ${MONITOR_TESTRIG_INCLUDE_DIR}
    ${BOOST_SML_INCLUDE_DIR}
)

link_libraries(
    nand_monitor
)

add_executable(main_nand_monitor_msgq
    main_nand_monitor_msgq.cc
    ${MONITOR_LIB_DIR}/msgq_common.c
    ${MONITOR_LIB_DIR}/rm_parse_interface_msgq.c)

target_compile_definitions(
    main_nand_monitor_msgq
    PRIVATE
    MONITOR_INTERFACE_MSGQ
)

target_link_libraries(main_nand_monitor_msgq
    rt
    nand_monitor)

add_custom_command(POST_BUILD TARGET main_nand_monitor_msgq
    COMMAND ${CMAKE_COMMAND} -E rename
    $<TARGET_FILE:main_nand_monitor_msgq>
    ${MONITOR_BUILD_DIR}/main_nand_monitor_msgq)

# Unit Tests
FetchContent_Declare(
    googletest
    URL "https://github.com/google/googletest/archive/refs/tags/v1.13.0.tar.gz"
    URL_HASH SHA256=ad7fdba11ea011c1d925b3289cf4af2c66a352e18d4c7264392fead75e919363
    DOWNLOAD_EXTRACT_TIMESTAMP true
)

set(BUILD_GMOCK ON)
set(INSTALL_GTEST OFF)
message(STATUS "Fetching googletest ...")
FetchContent_MakeAvailable(googletest)

enable_testing()
include(GoogleTest)

include_directories(
    ${googletest_SOURCE_DIR}/googletest/include
    ${googletest_SOURCE_DIR}/googlemock/include
)

link_libraries(
    gtest
    gmock
    gtest_main
)

add_executable(unit_nand_monitor
    unit_nand_monitor.cc
)
gtest_discover_tests(unit_nand_monitor)
##


# Benchmark
if (MONITOR_DEBUG_MSG)
    message(WARNING "Benchmarking with debug messages enabled. Try to turn it
    off with `cmake -DMONITOR_DEBUG_MSG=n ..`")
endif()

add_compile_options(-O2 -march=native -mtune=native)

include_directories(
    ${BENCHMARK_INC_DIR}
)
link_libraries(
    benchmark
    pthread)


add_executable(bench_nand_monitor
    bench_nand_monitor.cc)

##
