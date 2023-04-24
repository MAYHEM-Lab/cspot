add_executable(stress-test stress-test.c)
target_link_libraries(stress-test PUBLIC woof)
target_include_directories(stress-test PUBLIC "./include")

add_executable(stress-init stress-init.c)
target_link_libraries(stress-init PUBLIC woof)
target_include_directories(stress-init PUBLIC "./include")

add_handler(stress_handler stress_handler.c)
target_link_libraries(stress_handler PUBLIC woof)
target_include_directories(stress_handler PUBLIC "./include")
