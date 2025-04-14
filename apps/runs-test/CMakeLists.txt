add_subdirectory(ks)
add_library(normal normal.c normal.h)
add_library(c-twist c-twist.c c-twist.h)
add_library(c-runstest-obj c-runstest.c c-runstest.h)
target_include_directories(c-runstest-obj PUBLIC ks)

target_include_directories(ks PUBLIC ks)

add_handler(RHandler RHandler.c)
add_handler(SHandler SHandler.c)
add_handler(KHandler KHandler.c)

target_link_libraries(RHandler PUBLIC c-twist pthread)
target_link_libraries(KHandler PUBLIC ks normal pthread)
target_link_libraries(SHandler PUBLIC c-runstest-obj pthread)

target_include_directories(RHandler PUBLIC ks)
target_include_directories(SHandler PUBLIC ks)
target_include_directories(KHandler PUBLIC ks normal)


add_executable(c-runstest c-runstest.c c-runstest.h)
target_compile_definitions(c-runstest PRIVATE -DSTANDALONE)
target_include_directories(c-runstest PUBLIC ks)
target_link_libraries(c-runstest c-twist normal ks m)

add_executable(cspot-runstat cspot-runstat.c c-runstest.h)
target_compile_definitions(cspot-runstat PRIVATE -DSTANDALONE)
target_include_directories(cspot-runstat PUBLIC ks)
target_link_libraries(cspot-runstat woof pthread c-twist normal ks m pthread)

add_executable(cspot-runstat-multi-ns cspot-runstat-multi-ns.c c-runstest.h)
target_compile_definitions(cspot-runstat-multi-ns PRIVATE -DSTANDALONE)
target_include_directories(cspot-runstat-multi-ns PUBLIC ks)
target_link_libraries(cspot-runstat-multi-ns woof pthread c-twist normal ks m pthread)

add_executable(cspot-runstat-message cspot-runstat-message.c c-runstest.h)
target_compile_definitions(cspot-runstat-message PRIVATE -DSTANDALONE)
target_include_directories(cspot-runstat-message PUBLIC ks)
target_link_libraries(cspot-runstat-message woof c-twist normal ks m pthread)

add_handler(MM_mul_handler MM_mul_handler.c)
add_handler(MM_ctrl_handler MM_ctrl_handler.c)
add_handler(MM_add_handler MM_add_handler.c)
add_executable(mm_init mm_init.c mm_matrix.h)
target_link_libraries(mm_init woof)
add_executable(mm_print_result mm_print_result.c mm_matrix.h)
target_link_libraries(mm_print_result woof)
