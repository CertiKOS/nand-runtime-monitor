cmake_minimum_required(VERSION 3.15)

include_directories(
    ${MONITOR_TESTRIG_INCLUDE_DIR}
    ${BOOST_SML_INCLUDE_DIR}
)

set (NAND_MONITOR_SRC
    nand_monitor.cc)

set (NAND_MONITOR_EXTRA)

if (MONITOR_INTERFACE STREQUAL "func")
    list(APPEND NAND_MONITOR_EXTRA
        rm_parse_interface_func.c)
elseif(MONITOR_INTERFACE STREQUAL "msgq")
    list(APPEND NAND_MONITOR_EXTRA
        rm_parse_interface_msgq.c)
else()
    message(FATAL_ERROR "Monitor interface `${MONITOR_INTERFACE}` is not supported!")
endif()

add_library(nand_monitor
    STATIC
    ${NAND_MONITOR_SRC}
    ${NAND_MONITOR_EXTRA})

add_dependencies(nand_monitor
    testrig-inc)
