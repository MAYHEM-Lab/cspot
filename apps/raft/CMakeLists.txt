
# build rafts
add_library(raft raft.c raft_utils.c raft_client.c)
target_link_libraries(raft PUBLIC woof)
target_include_directories(raft PUBLIC "${CMAKE_SOURCE_DIR}/apps/raft/include")


# handlers
add_handler(raft_s_create_woofs raft_s_create_woofs.c)
target_link_libraries(raft_s_create_woofs PUBLIC raft)

add_handler(s_start_server s_start_server.c )
target_link_libraries(s_start_server PUBLIC raft)


add_handler(h_append_entries h_append_entries.c )
target_link_libraries(h_append_entries PUBLIC raft)

add_handler(h_client_put h_client_put.c)
target_link_libraries(h_client_put PUBLIC raft)

add_handler(h_config_change h_config_change.c)
target_link_libraries(h_config_change PUBLIC raft)

add_handler(h_count_vote h_count_vote.c)
target_link_libraries(h_count_vote PUBLIC raft)

add_handler(h_forward_put_result h_forward_put_result.c)
target_link_libraries(h_forward_put_result PUBLIC raft)

add_handler(h_replicate_entries h_replicate_entries.c)
target_link_libraries(h_replicate_entries PUBLIC raft)

add_handler(h_request_vote h_request_vote.c)
target_link_libraries(h_request_vote PUBLIC raft)

add_handler(h_timeout_checker h_timeout_checker.c)
target_link_libraries(h_timeout_checker PUBLIC raft)


# clients

add_executable(client_get client_get.c)
target_link_libraries(client_get PUBLIC woof raft)

add_executable(raft_client_node_info raft_client_node_info.c)
target_link_libraries(raft_client_node_info PUBLIC woof raft)

add_executable(client_observe client_observe.c)
target_link_libraries(client_observe PUBLIC woof raft)

add_executable(client_put_result client_put_result.c)
target_link_libraries(client_put_result PUBLIC woof raft)

add_executable(client_put client_put.c)
target_link_libraries(client_put PUBLIC woof raft)

add_executable(client_reconfig client_reconfig.c)
target_link_libraries(client_reconfig PUBLIC woof raft)

# tests
# test: test_append_handler test_client test_config test_raft_handler test_throughput test_throughput_handler
add_executable(test_append_handler test_append_handler.c)
target_link_libraries(test_append_handler PUBLIC woof raft)

add_executable(test_client test_client.c)
target_link_libraries(test_client PUBLIC woof raft)

add_executable(test_config test_config.c)
target_link_libraries(test_config PUBLIC woof raft)

add_executable(test_raft_handler test_raft_handler.c)
target_link_libraries(test_raft_handler PUBLIC woof raft)

add_executable(test_throughput test_throughput.c)
target_link_libraries(test_throughput PUBLIC woof raft)

add_executable(test_throughput_handler test_throughput_handler.c)
target_link_libraries(test_throughput_handler PUBLIC woof raft)