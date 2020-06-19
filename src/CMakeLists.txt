add_executable(log-dump log-dump.c)
target_link_libraries(log-dump PRIVATE woof)

add_executable(woof-repair woof-repair.c)
target_link_libraries(woof-repair PRIVATE woof)

add_executable(log-test log-test.c)
target_link_libraries(log-test PRIVATE woof)

add_executable(log-test-thread log-test-thread.c)
target_link_libraries(log-test-thread PRIVATE woof)