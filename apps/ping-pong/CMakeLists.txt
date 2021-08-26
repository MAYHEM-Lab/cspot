include_directories( "./include")
link_libraries(woof)

add_executable(ping-pong-start ping-pong-start.c)

add_executable(ping-start ping-start.c)

add_executable(pong-start pong-start.c)

# TODO: check for wf->shared->filename
# add_handler(ping ping.c)
# add_handler(pong pong.c)