include_directories("./includes")

# TODO: check for log.h and repair.h
# add_executable(merge-init merge-init.c)
# target_link_libraries(merge-init PUBLIC woof mio)

add_executable(merge-remote merge-remote.c)
target_link_libraries(merge-remote PUBLIC woof)

# TODO: check for wf->shared->filename
# add_executable(merge merge.c)
# target_link_libraries(merge PUBLIC woof)