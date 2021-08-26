include_directories("./includes")

add_executable(micro micro.c)
target_link_libraries(micro PUBLIC woof)

add_executable(micro-start micro-start.c)
target_link_libraries(micro-start PUBLIC woof)

add_executable(micro-create micro-create.c)
target_link_libraries(micro-create PUBLIC woof)