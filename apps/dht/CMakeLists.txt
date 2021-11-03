
set(OPENSSL_USE_STATIC_LIBS TRUE)
set(OPENSSL_MSVC_STATIC_RT TRUE)

find_package(OpenSSL REQUIRED)
pkg_search_module(OPENSSL REQUIRED openssl)


add_library(dht dht.c dht_utils.c dht_client.c)
target_link_libraries(dht PUBLIC woof raft monitor OpenSSL::SSL OpenSSL::Crypto)
target_include_directories(dht PUBLIC "${CMAKE_SOURCE_DIR}/apps/dht/include")

# Handlers 

add_handler(d_check_predecessor d_check_predecessor.c)
target_link_libraries(d_check_predecessor PUBLIC dht)

add_handler(d_daemon d_daemon.c)
target_link_libraries(d_daemon PUBLIC dht monitor)

add_handler(d_fix_finger d_fix_finger.c)
target_link_libraries(d_fix_finger PUBLIC dht monitor)

add_handler(d_stabilize d_stabilize.c)
target_link_libraries(d_stabilize PUBLIC dht monitor raft)

add_handler(h_find_successor h_find_successor.c)
target_link_libraries(h_find_successor PUBLIC dht monitor raft)

add_handler(h_fix_finger_callback h_fix_finger_callback.c)
target_link_libraries(h_fix_finger_callback PUBLIC dht monitor woof)

add_handler(h_get_predecessor h_get_predecessor.c)
target_link_libraries(h_get_predecessor PUBLIC dht monitor woof)

add_handler(h_invalidate_cache h_invalidate_cache.c)
target_link_libraries(h_invalidate_cache PUBLIC dht monitor woof)

add_handler(h_invalidate_fingers h_invalidate_fingers.c)
target_link_libraries(h_invalidate_fingers PUBLIC dht monitor woof)

add_handler(h_join_callback h_join_callback.c)
target_link_libraries(h_join_callback PUBLIC dht monitor woof raft)

add_handler(h_notify h_notify.c)
target_link_libraries(h_notify PUBLIC dht monitor woof)

add_handler(h_shift_successor h_shift_successor.c)
target_link_libraries(h_shift_successor PUBLIC dht monitor raft)

add_handler(h_notify_callback h_notify_callback.c)
target_link_libraries(h_notify_callback PUBLIC dht monitor raft)

add_handler(h_stabilize_callback h_stabilize_callback.c)
target_link_libraries(h_stabilize_callback PUBLIC dht monitor raft woof)

add_handler(h_subscribe h_subscribe.c)
target_link_libraries(h_subscribe PUBLIC dht monitor raft)

add_handler(r_create_index_map r_create_index_map.c)
target_link_libraries(r_create_index_map PUBLIC dht monitor)

add_handler(r_register_topic r_register_topic.c)
target_link_libraries(r_register_topic PUBLIC dht monitor woof)

add_handler(r_set_successor r_set_successor.c)
target_link_libraries(r_set_successor PUBLIC dht monitor woof)

add_handler(r_subscribe r_subscribe.c)
target_link_libraries(r_subscribe PUBLIC dht monitor woof)

add_handler(r_try_replicas r_try_replicas.c)
target_link_libraries(r_try_replicas PUBLIC dht monitor woof raft)

add_handler(server_publish_data server_publish_data.c)
target_link_libraries(server_publish_data PUBLIC dht monitor raft woof)

add_handler(server_publish_trigger server_publish_trigger.c)
target_link_libraries(server_publish_trigger PUBLIC dht raft woof monitor)

# clients:    \
# 	     client_publish client_subscribe

add_executable(s_create_cluster s_create_cluster.c)
target_link_libraries(s_create_cluster PUBLIC woof dht)

add_executable(dht_s_create_woofs dht_s_create_woofs.c)
target_link_libraries(dht_s_create_woofs PUBLIC woof dht)

add_executable(s_join_cluster s_join_cluster.c)
target_link_libraries(s_join_cluster PUBLIC woof dht raft monitor)

add_executable(client_check_leader client_check_leader.c)
target_link_libraries(client_check_leader PUBLIC woof dht raft)

add_executable(client_find_node client_find_node.c)
target_link_libraries(client_find_node PUBLIC woof dht raft)

add_executable(client_init_topic client_init_topic.c)
target_link_libraries(client_init_topic PUBLIC woof dht raft)

add_executable(dht_client_node_info dht_client_node_info.c)
target_link_libraries(dht_client_node_info PUBLIC woof dht raft)

add_subdirectory(apps)