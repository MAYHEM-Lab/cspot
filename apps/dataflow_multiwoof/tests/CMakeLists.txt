add_executable(multinode_regression multinode_regression.cpp ../dfinterface.cpp)
target_link_libraries(multinode_regression PUBLIC woof)
target_include_directories(multinode_regression PUBLIC "./include")

add_executable(distributed_multinode_regression distributed_multinode_regression.cpp ../dfinterface.cpp)
target_link_libraries(distributed_multinode_regression PUBLIC woof)
target_include_directories(distributed_multinode_regression PUBLIC "./include")

add_executable(tests tests.cpp ../dfinterface.cpp
    arithmetic_tests.cpp
    conditional_tests.cpp
    namespace_tests.cpp
    loop_tests.cpp
)
target_link_libraries(tests PUBLIC woof)
target_include_directories(tests PUBLIC "./include")

add_executable(baseline-regression baseline-regression.cpp)