add_executable(log-dump log-dump.c)
target_link_libraries(log-dump PRIVATE woof)
target_include_directories(log-dump PRIVATE "include")

add_executable(log-test log-test.c)
target_link_libraries(log-test PRIVATE woof)
target_include_directories(log-test PRIVATE "include")

add_executable(log-test-thread log-test-thread.c)
target_link_libraries(log-test-thread PRIVATE woof)
target_include_directories(log-test-thread PRIVATE "include")

if (ENABLE_FEATURE_REPAIR)
    add_executable(woof-repair woof-repair.c)
    target_link_libraries(woof-repair PRIVATE woof)
endif()

if (ENABLE_PYCSPOT)
    add_subdirectory(python)
endif()

add_subdirectory(tests)
