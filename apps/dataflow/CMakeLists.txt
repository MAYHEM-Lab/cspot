add_library(dfoperation dfoperation.c)
target_link_libraries(dfoperation PUBLIC woof)
target_include_directories(dfoperation PUBLIC "./include")

add_executable(dfinit dfinit.c)
target_link_libraries(dfinit PUBLIC woof)
target_include_directories(dfinit PUBLIC "./include")

add_executable(dfaddnode dfaddnode.c)
target_link_libraries(dfaddnode PUBLIC woof)
target_include_directories(dfaddnode PUBLIC "./include")

add_executable(dfaddoperand dfaddoperand.c)
target_link_libraries(dfaddoperand PUBLIC woof)
target_include_directories(dfaddoperand PUBLIC "./include")

add_executable(dfgetoperand dfgetoperand.c)
target_link_libraries(dfgetoperand PUBLIC woof)
target_include_directories(dfgetoperand PUBLIC "./include")

# TODO: fix wf->shared->filename
add_handler(dfhandler dfhandler.c)
target_link_libraries(dfhandler PUBLIC woof dfoperation)
target_include_directories(dfhandler PUBLIC "./include")
