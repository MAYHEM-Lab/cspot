include_directories("./include" "../../src/include")

add_library(senspot senspot.c)
target_link_libraries(senspot PUBLIC woof)

add_executable(senspot-get senspot-get.c)
target_link_libraries(senspot-get PUBLIC woof senspot)

add_executable(senspot-put senspot-put.c)
target_link_libraries(senspot-put PUBLIC woof senspot)

add_executable(senspot-init senspot-init.c)
target_link_libraries(senspot-init PUBLIC woof senspot)

add_executable(senspot-forward-init senspot-forward-init.c)
target_link_libraries(senspot-forward-init PUBLIC woof senspot)

add_executable(device-ping-init device-ping-init.c)
target_link_libraries(device-ping-init PUBLIC woof)

add_executable(device-ping-timing device-ping-timing.c)
target_link_libraries(device-ping-timing PUBLIC woof)

add_executable(device-platform-ping device-platform-ping.c)
target_link_libraries(device-platform-ping PUBLIC woof)

add_handler(PingHandler PingHandler.c)
target_link_libraries(PingHandler PUBLIC woof)

# TODO: fix wf->shared->filename
add_handler(senspot_log senspot_log.c)
target_link_libraries(senspot_log PUBLIC woof senspot)

# TODO: fix wf->shared->filename
add_handler(senspot_forward senspot_forward.c)
target_link_libraries(senspot_forward PUBLIC woof senspot)
