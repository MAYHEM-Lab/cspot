add_handler(cjkHhw cjkHhw.c)
target_link_libraries(cjkHhw PUBLIC woof)
add_handler(cjkHlw cjkHlw.c)
target_link_libraries(cjkHlw PUBLIC woof)
add_handler(cjkHrw cjkHrw.c)
target_link_libraries(cjkHrw PUBLIC woof)

add_executable(cjk-hw cjk-hw.c)
target_link_libraries(cjk-hw woof)
add_executable(cjk-hw-init cjk-hw-init.c)
target_link_libraries(cjk-hw-init woof)
add_executable(cjk-ping cjk-ping.c)
target_link_libraries(cjk-ping woof)

