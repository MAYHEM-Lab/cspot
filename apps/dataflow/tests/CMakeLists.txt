add_executable(df-narity-test df-narity-test.c ../dfinterface.c)
target_link_libraries(df-narity-test PUBLIC woof uuid)
target_include_directories(df-narity-test PUBLIC "./include")

add_executable(df-stress-test df-stress-test.c ../dfinterface.c)
target_link_libraries(df-stress-test PUBLIC woof uuid)
target_include_directories(df-stress-test PUBLIC "./include")