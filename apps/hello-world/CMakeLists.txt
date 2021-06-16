add_handler(hw hw.c)

add_executable(hw-client hw-client.c)
target_link_libraries(hw-client woof)

add_executable(hw-start hw-start.c)
target_link_libraries(hw-start woof)