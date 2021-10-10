add_subdirectory(red_black)
add_library(ks ks.c ks.h simple_input.c simple_input.h cdf.c cdf.h)
target_include_directories(rb2 PUBLIC red_black)
target_link_libraries(ks rb2)
