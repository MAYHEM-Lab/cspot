cmake_minimum_required(VERSION 3.6)
project(cspot-hw-cpp CXX C)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/Cspot.cmake)

add_executable(hw hw.hpp hw.cpp)
target_link_libraries(hw PUBLIC cspot)
set_property(TARGET hw PROPERTY CXX_STANDARD 11)
cspot_add_handler(hw)

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