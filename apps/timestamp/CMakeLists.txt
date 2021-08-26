include_directories("./include")

add_executable(ts-start ts-start.c)
target_link_libraries(ts-start PUBLIC woof)

# TODO: log.h
# add_executable(ts-repair ts-repair.c)
# target_link_libraries(ts-repair PUBLIC woof)

add_executable(ts-create ts-create.c)
target_link_libraries(ts-create PUBLIC woof)

# TODO: wf->shared->filename
# add_handler(ts ts.c)
# target_link_libraries(ts PUBLIC woof)
