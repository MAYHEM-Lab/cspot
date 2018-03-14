cmake_minimum_required(VERSION 3.6)
project(cspot_hw)

set(DEP "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
set(WOOFC "${CMAKE_CURRENT_SOURCE_DIR}/../..")

add_library(euca_utils INTERFACE IMPORTED)
set_target_properties(euca_utils PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${DEP}/euca-cutils"
    INTERFACE_LINK_LIBRARIES "${DEP}/euca-cutils/libutils.a"
)

add_library(mio INTERFACE IMPORTED)
set_target_properties(mio PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${DEP}/mio"
    INTERFACE_LINK_LIBRARIES "${DEP}/mio/mio.o;${DEP}/mio/mymalloc.o"
)

add_library(woof INTERFACE IMPORTED)
set_target_properties(woof PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${WOOFC}"
    INTERFACE_LINK_LIBRARIES "${WOOFC}/woofc.o;${WOOFC}/woofc-access.o;${WOOFC}/woofc-cache.o"
)

add_library(woof_host INTERFACE IMPORTED)
set_target_properties(woof_host PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${WOOFC}"
    INTERFACE_LINK_LIBRARIES "${WOOFC}/woofc-host.o"
)

add_library(woof_thread INTERFACE IMPORTED)
set_target_properties(woof_thread PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${WOOFC}"
    INTERFACE_LINK_LIBRARIES "${WOOFC}/woofc-thread.o"
)

add_library(woof_log INTERFACE IMPORTED)
set_target_properties(woof_log PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${WOOFC}"
    INTERFACE_LINK_LIBRARIES "${WOOFC}/log.o;${WOOFC}/host.o;${WOOFC}/event.o"
)

add_library(cspot INTERFACE IMPORTED)
set_target_properties(cspot PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${WOOFC}"
    INTERFACE_LINK_LIBRARIES "mio;woof;woof_log;woof_host;pthread;m;czmq;euca_utils;${WOOFC}/uriparser2/liburiparser2.a;${WOOFC}/lsema.o"
)

add_library(shepherd "${WOOFC}/woofc-shepherd.c")
target_link_libraries(shepherd PUBLIC cspot)
target_compile_definitions(shepherd PUBLIC WOOF_HANDLER_NAME=hw)

add_executable(hw hw.hpp hw.cpp)
target_link_libraries(hw PUBLIC cspot)
target_link_libraries(hw PUBLIC shepherd)
set_property(TARGET hw PROPERTY CXX_STANDARD 11)

add_executable(hw-start hw-start.cpp hw.hpp)
target_link_libraries(hw-start PUBLIC cspot)
set_property(TARGET hw-start PROPERTY CXX_STANDARD 11)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/cspot/)

add_custom_command(TARGET hw POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:hw> ${CMAKE_BINARY_DIR}/cspot/
    COMMAND ${CMAKE_COMMAND} -E copy ${WOOFC}/woofc-container ${CMAKE_BINARY_DIR}/cspot/
    COMMAND ${CMAKE_COMMAND} -E copy ${WOOFC}/woofc-namespace-platform ${CMAKE_BINARY_DIR}/cspot/
)

add_custom_command(TARGET hw-start POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:hw-start> ${CMAKE_BINARY_DIR}/cspot/
    COMMAND ${CMAKE_COMMAND} -E copy ${WOOFC}/woofc-container ${CMAKE_BINARY_DIR}/cspot/
    COMMAND ${CMAKE_COMMAND} -E copy ${WOOFC}/woofc-namespace-platform ${CMAKE_BINARY_DIR}/cspot/
)