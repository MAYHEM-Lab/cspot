add_library(dfoperation dfoperation.c)
target_link_libraries(dfoperation PUBLIC woof)
target_include_directories(dfoperation PUBLIC "./include")

add_executable(dfinit dfinit.c)
target_link_libraries(dfinit PUBLIC woof)
target_include_directories(dfinit PUBLIC "./include")

add_executable(narity-test narity-test.c dfinterface.c)
target_link_libraries(narity-test PUBLIC woof)
target_include_directories(narity-test PUBLIC "./include")

# TODO: fix wf->shared->filename
add_handler(dfhandler dfhandler.c)
target_link_libraries(dfhandler PUBLIC woof dfoperation)
target_include_directories(dfhandler PUBLIC "./include")
