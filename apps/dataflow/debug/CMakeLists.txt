add_executable(dfstackdebug dfstackdebug.c)
target_link_libraries(dfstackdebug PUBLIC woof dfdebug)
target_include_directories(dfstackdebug PUBLIC "./include")