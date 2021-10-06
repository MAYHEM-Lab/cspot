include_directories("./Includes")

add_executable(master-slave-init master-slave-init.c)
target_link_libraries(master-slave-init PUBLIC woof)

add_executable(master-slave-pulse master-slave-pulse.c)
target_link_libraries(master-slave-pulse PUBLIC woof)

#To be checked with Rich
# TODO: check woofRead is declared in woof-priv and unaccessable
# add_handler(MSPulseHandler MSPulseHandler.c)
# target_link_libraries(MSPulseHandler PUBLIC woof)

add_handler(MSPingPongHandler MSPingPongHandler.c)
target_link_libraries(MSPingPongHandler PUBLIC woof)
