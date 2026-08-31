add_executable(cspot-app-example-client cspot-app-example-client.c)
target_link_libraries(cspot-app-example-client PUBLIC woof)
target_include_directories(cspot-app-example-client PUBLIC "../include")

add_executable(cspot-app-example-init cspot-app-example-init.c)
target_link_libraries(cspot-app-example-init PUBLIC woof)
target_include_directories(cspot-app-example-init PUBLIC "../include")

add_handler(cspot_app_example_handler cspot_app_example_handler.c)
target_link_libraries(cspot_app_example_handler PUBLIC woof)
target_include_directories(cspot_app_example_handler PUBLIC "../include")

set_target_properties(cspot-app-example-client PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(cspot-app-example-init PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(cspot_app_example_handler PROPERTIES LINKER_LANGUAGE CXX)
