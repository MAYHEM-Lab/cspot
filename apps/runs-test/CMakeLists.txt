add_subdirectory(ks)
add_library(normal normal.c normal.h)
add_library(c-twist c-twist.c c-twist.h)
add_library(c-runstest-obj c-runstest.c c-runstest.h)
target_include_directories(c-runstest-obj PUBLIC ks)

target_include_directories(ks PUBLIC ks)

add_handler(RHandler RHandler.c)
add_handler(SHandler SHandler.c)
add_handler(KHandler KHandler.c)

target_link_libraries(RHandler PUBLIC c-twist)
target_link_libraries(KHandler PUBLIC ks normal)
target_link_libraries(SHandler PUBLIC c-runstest-obj)

target_include_directories(RHandler PUBLIC ks)
target_include_directories(SHandler PUBLIC ks)
target_include_directories(KHandler PUBLIC ks normal)


add_executable(c-runstest c-runstest.c c-runstest.h)
target_compile_definitions(c-runstest PRIVATE -DSTANDALONE)
target_include_directories(c-runstest PUBLIC ks)
target_link_libraries(c-runstest c-twist normal ks m)
