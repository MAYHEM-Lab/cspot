add_executable(hw hw.c)
target_link_libraries(hw PRIVATE woof woofc-shepherd)

add_executable(hw-client hw-client.c)
target_link_libraries(hw-client PRIVATE woof woofc-host)