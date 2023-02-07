add_subdirectory(tests)
add_subdirectory(debug)

add_library(dfoperation dfoperation.c)
target_link_libraries(dfoperation PUBLIC woof)
target_include_directories(dfoperation PUBLIC "./include")

add_executable(dfinit dfinit.c)
target_link_libraries(dfinit PUBLIC woof)
target_include_directories(dfinit PUBLIC "./include")

add_library(dfdebug dflogger.c dflogger.h)

# TODO: fix wf->shared->filename
add_handler(dfhandler dfhandler.c)
target_link_libraries(dfhandler PUBLIC woof dfoperation dfdebug)
target_include_directories(dfhandler PUBLIC "./include")

add_handler(dfnodehandler dfnodehandler.c)
target_link_libraries(dfnodehandler PUBLIC woof dfdebug)
target_include_directories(dfnodehandler PUBLIC "./include")
