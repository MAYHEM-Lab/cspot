include_directories("./include")
add_handler(caplets_handler caplets_handler.c)

add_executable(cspot_caplets cspot_caplets.cpp)
target_link_libraries(cspot_caplets woof caplets)

add_executable(read_log read_log.cpp)
target_link_libraries(read_log woof)