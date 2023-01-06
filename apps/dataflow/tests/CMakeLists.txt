add_executable(narity-test narity-test.c ../dfinterface.c)
target_link_libraries(narity-test PUBLIC woof)
target_include_directories(narity-test PUBLIC "./include")