include_directories("./includes")

add_executable(create create.c)
target_link_libraries(create PUBLIC woof)

add_executable(api-recv api-recv.c)
target_link_libraries(api-recv PUBLIC woof)

add_executable(api-send api-send.c)
target_link_libraries(api-send PUBLIC woof)

add_executable(api-direct-send api-direct-send.c)
target_link_libraries(api-direct-send PUBLIC woof)

add_executable(api-direct-send-nohandler api-direct-send-nohandler.c)
target_link_libraries(api-direct-send-nohandler PUBLIC woof)

add_handler(cause_recv cause_recv.c)
target_link_libraries(cause_recv PUBLIC woof)

add_handler(cause_send cause_send.c)
target_link_libraries(cause_send PUBLIC woof)
