cmake_minimum_required(VERSION 3.6)
project(cspot-hw-cpp CXX)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/Cspot.cmake)

add_library(hw hw.hpp hw.cpp)
target_link_libraries(hw PUBLIC cspot)
set_property(TARGET hw PROPERTY CXX_STANDARD 11)
cspot_add_handler(hw)

add_executable(hw-start hw-start.cpp hw.hpp)
target_link_libraries(hw-start PUBLIC cspot)
set_property(TARGET hw-start PROPERTY CXX_STANDARD 11)

add_custom_command(TARGET hw-start POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:hw-start> ${CMAKE_BINARY_DIR}/cspot/
)