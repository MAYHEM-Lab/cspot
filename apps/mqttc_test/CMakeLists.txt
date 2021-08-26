include_directories("./includes")

add_executable(mqttc_create mqttc_create.c)
target_link_libraries(mqttc_create PUBLIC woof)

# TODO add fix for #include "MQTTAsync.h"
# add_executable(mqttc_pub mqttc_pub.c)
# target_link_libraries(mqttc_pub PUBLIC woof)

# add_executable(mqttc_sub_trigger mqttc_sub_trigger.c)
# target_link_libraries(mqttc_sub_trigger PUBLIC woof)

# add_executable(mqttc_sub_woof mqttc_sub_woof.c)
# target_link_libraries(mqttc_sub_woof PUBLIC woof)

# add_executable(mqttc_sub mqttc_sub.c)
# target_link_libraries(mqttc_sub PUBLIC woof)

# TODO: How to get CLOCK_REALTIME from deps/libzmq/src/clock.hpp
# add_handler(mqttc_test mqttc_test.c)
# target_link_libraries(mqttc_test PUBLIC woof zmq)
