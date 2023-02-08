add_executable(dfnodestackdebug dfnodestackdebug.c)
target_link_libraries(dfnodestackdebug PUBLIC woof dfdebug)
target_include_directories(dfnodestackdebug PUBLIC "./include")

add_executable(dfoperandstackdebug dfoperandstackdebug.c)
target_link_libraries(dfoperandstackdebug PUBLIC woof dfdebug)
target_include_directories(dfoperandstackdebug PUBLIC "./include")