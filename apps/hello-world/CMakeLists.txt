add_handler(hw hw.c)

add_executable(hw-client hw-client.c)
target_link_libraries(hw-client PRIVATE woof woofc-host)