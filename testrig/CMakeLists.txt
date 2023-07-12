cmake_minimum_required(VERSION 3.15)

add_compile_options(-Wall -Wextra -Wpedantic -Werror)
add_compile_options(-Wno-type-limits)
add_compile_options(-Wno-unused-parameter)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g)
    add_compile_options(-fsanitize=address,undefined)
    add_link_options(-fsanitize=address,undefined)
endif()

set (MONITOR_ORIGIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Origins)
set (MONITOR_SCRIPTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Scripts)
set (MONITOR_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Include)
set (MONITOR_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Lib)

add_custom_command(OUTPUT ${MONITOR_LIB_DIR}/libns.a
    COMMAND make
    DEPENDS testrig-inc
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/NANDStub
    VERBATIM
)
add_custom_target(testrig-libns DEPENDS ${MONITOR_LIB_DIR}/libns.a)

add_library(nandstub STATIC IMPORTED)
set_target_properties(nandstub PROPERTIES IMPORTED_LOCATION ${MONITOR_LIB_DIR}/libns.a)
add_dependencies(nandstub testrig-libns)

add_custom_command(OUTPUT ${MONITOR_LIB_DIR}/libde.a
    COMMAND make
    DEPENDS testrig-inc
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/DeviceEmulator
    VERBATIM
    )
add_custom_target(testrig-libde DEPENDS ${MONITOR_LIB_DIR}/libde.a)

add_library(devemu STATIC IMPORTED)
set_target_properties(devemu PROPERTIES IMPORTED_LOCATION ${MONITOR_LIB_DIR}/libde.a)
add_dependencies(devemu testrig-libde)


add_custom_command(OUTPUT ${MONITOR_LIB_DIR}/librm.a
    COMMAND make
    DEPENDS testrig-inc testrig-patch
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/RuntimeMonitor/rm_parse.c
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/RuntimeMonitor/rm_state.c
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/RuntimeMonitor
    VERBATIM
    )
add_custom_target(testrig-librm DEPENDS ${MONITOR_LIB_DIR}/librm.a)

add_library(rmon STATIC IMPORTED)
set_target_properties(rmon PROPERTIES IMPORTED_LOCATION ${MONITOR_LIB_DIR}/librm.a)
add_dependencies(rmon testrig-librm)


add_custom_command(OUTPUT ${MONITOR_LIB_DIR}/libtest.a
    COMMAND make
    DEPENDS testrig-inc
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/TestLib
    VERBATIM
    )
add_custom_target(testrig-libtest DEPENDS ${MONITOR_LIB_DIR}/libtest.a)

add_library(test STATIC IMPORTED)
set_target_properties(test PROPERTIES IMPORTED_LOCATION ${MONITOR_LIB_DIR}/libtest.a)
add_dependencies(test testrig-libtest)


# Binaries

include_directories(
    ${MONITOR_TESTRIG_INCLUDE_DIR}
    ${MONITOR_LIB_DIR}
    )

link_libraries(
    rmon
    nandstub
    test
    devemu
    nand_monitor
    )

if (MONITOR_INTERFACE STREQUAL "msgq")
    link_libraries(rt)
endif()

add_executable(test_example
    TestExample/test_example.c
    )
add_custom_command(POST_BUILD TARGET test_example
    COMMAND ${CMAKE_COMMAND} -E rename
        ${CMAKE_CURRENT_BINARY_DIR}/test_example
        ${MONITOR_BUILD_DIR}/test_example
    )

add_executable(test_ns
    TestNS/test_ns.c
    )
add_custom_command(POST_BUILD TARGET test_ns
    COMMAND ${CMAKE_COMMAND} -E rename
        ${CMAKE_CURRENT_BINARY_DIR}/test_ns
        ${MONITOR_BUILD_DIR}/test_ns
    )

add_executable(test_rm
    TestRM/test_rm.c
    )
add_custom_command(POST_BUILD TARGET test_rm
    COMMAND ${CMAKE_COMMAND} -E rename
        ${CMAKE_CURRENT_BINARY_DIR}/test_rm
        ${MONITOR_BUILD_DIR}/test_rm
    )

add_executable(test_system
    TestSystem/st_goto.c
    TestSystem/st_oracle.c
    TestSystem/st_print.c
    TestSystem/test_system.c
    )
add_custom_command(POST_BUILD TARGET test_system
    COMMAND ${CMAKE_COMMAND} -E rename
        ${CMAKE_CURRENT_BINARY_DIR}/test_system
        ${MONITOR_BUILD_DIR}/test_system
    )

