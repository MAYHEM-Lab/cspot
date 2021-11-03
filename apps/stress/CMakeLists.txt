include_directories("./include")

add_executable(hw_create_stress hw_create_stress.c)
target_link_libraries(hw_create_stress PUBLIC woof)

add_executable(hw_put_stress hw_put_stress.c)
target_link_libraries(hw_put_stress PUBLIC woof)

add_executable(hw_get_stress hw_get_stress.c)
target_link_libraries(hw_get_stress PUBLIC woof)

add_handler(hw_stress hw_stress.c)
target_link_libraries(hw_stress PUBLIC woof)
