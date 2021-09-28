add_executable(put-test put-test.c)
target_link_libraries(put-test PUBLIC woof)
target_include_directories(put-test PUBLIC "./include")

add_executable(bw-test bw-test.c)
target_link_libraries(bw-test PUBLIC woof)
target_include_directories(bw-test PUBLIC "./include")

add_executable(invoke-test invoke-test.c)
target_link_libraries(invoke-test PUBLIC woof)

add_executable(stress-test stress-test.c)
target_link_libraries(stress-test PUBLIC woof)
target_include_directories(stress-test PUBLIC "./include")

add_executable(recv-init recv-init.c)
target_link_libraries(recv-init PUBLIC woof)
target_include_directories(recv-init PUBLIC "./include")

add_executable(stress-init stress-init.c)
target_link_libraries(stress-init PUBLIC woof)
target_include_directories(stress-init PUBLIC "./include")

add_handler(recv_start recv_start.c)
target_link_libraries(recv_start PUBLIC woof)
target_include_directories(recv_start PUBLIC "./include")

# TODO: check for wf->shared->seq_no
# add_handler(recv recv.c)
# target_link_libraries(recv PUBLIC woof)
# target_include_directories(recv PUBLIC "./include")

add_handler(null_handler null_handler.c)
target_link_libraries(null_handler PUBLIC woof)
target_include_directories(null_handler PUBLIC "./include")

add_handler(invoke_handler invoke_handler.c)
target_link_libraries(invoke_handler PUBLIC woof)
target_include_directories(invoke_handler PUBLIC "./include")

add_handler(stress_handler stress_handler.c)
target_link_libraries(stress_handler PUBLIC woof)
target_include_directories(stress_handler PUBLIC "./include")
