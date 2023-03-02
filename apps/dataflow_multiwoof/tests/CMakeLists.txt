add_executable(multiwoof-test multiwoof-test.cpp ../dfinterface.cpp)
target_link_libraries(multiwoof-test PUBLIC woof)
target_include_directories(multiwoof-test PUBLIC "./include")

add_executable(multinode_regression multinode_regression.cpp ../dfinterface.cpp)
target_link_libraries(multinode_regression PUBLIC woof)
target_include_directories(multinode_regression PUBLIC "./include")

add_executable(baseline-regression baseline-regression.cpp)