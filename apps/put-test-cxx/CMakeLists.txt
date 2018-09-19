cmake_minimum_required(VERSION 3.2)
project(cspot-put-cpp CXX)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/Cspot.cmake)

add_library(put-common common.hpp stub.cpp)
target_link_libraries(put-common PUBLIC cspot)
target_compile_features(put-common PUBLIC cxx_std_11)

add_executable(put-start start.cpp)
target_link_libraries(put-start PUBLIC put-common)

add_custom_command(TARGET put-start POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:put-start> ${CMAKE_BINARY_DIR}/cspot/
)

add_executable(mcu_emu mcu.cpp)
target_link_libraries(mcu_emu PUBLIC put-common)

add_custom_command(TARGET mcu_emu POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:mcu_emu> ${CMAKE_BINARY_DIR}/cspot/
)

add_library(pi_data pi.cpp)
add_library(nuc_data nuc.cpp)
add_library(aristotle_data aristotle.cpp)
add_library(aws_data aws.cpp)

add_library(fwd_base fwd_handler.cpp)
target_link_libraries(fwd_base PUBLIC put-common)

add_library(fwd1 stub.cpp)
add_library(fwd2 stub.cpp)
add_library(fwd3 stub.cpp)
add_library(fwd4 stub.cpp)

target_link_libraries(fwd1 PUBLIC fwd_base pi_data)
target_link_libraries(fwd2 PUBLIC fwd_base nuc_data)
target_link_libraries(fwd3 PUBLIC fwd_base aristotle_data)
target_link_libraries(fwd4 PUBLIC fwd_base aws_data)

cspot_add_handler(fwd1 fwd)
cspot_add_handler(fwd2 fwd)
cspot_add_handler(fwd3 fwd)
cspot_add_handler(fwd4 fwd)