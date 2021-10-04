add_library(senspot senspot.c senspot.h)

add_executable(senspot-init senspot-init.c)
target_link_libraries(senspot-init woof mio euca_utils senspot)

add_executable(senspot-get senspot-get.c)
target_link_libraries(senspot-get woof mio euca_utils senspot)

add_executable(senspot-put senspot-put.c)
target_link_libraries(senspot-put woof mio euca_utils senspot)

