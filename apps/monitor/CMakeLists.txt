
add_library(monitor monitor.c)
target_link_libraries(monitor PUBLIC woof )
target_include_directories(monitor PUBLIC "include")

add_handler(monitor_invoker monitor_invoker.c)
target_link_libraries(monitor_invoker PUBLIC woof monitor)

add_executable(test_monitor_ test_monitor.c)
target_include_directories(test_monitor_ PUBLIC "include")
target_link_libraries(test_monitor_ PUBLIC woof monitor)

add_handler(test_handler_concurrent test_handler_concurrent.c)
target_link_libraries(test_handler_concurrent PUBLIC woof monitor)

add_handler(test_handler_monitored test_handler_monitored.c)
target_link_libraries(test_handler_monitored PUBLIC woof monitor)

add_handler(test_handler_idempotent test_handler_idempotent.c)
target_link_libraries(test_handler_idempotent PUBLIC woof monitor)
