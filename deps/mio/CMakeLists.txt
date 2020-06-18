project(mio)

add_library(mio mio.c mio.h mymalloc.c mymalloc.h)
target_include_directories(mio PUBLIC .)
