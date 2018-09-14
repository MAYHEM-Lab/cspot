cmake_minimum_required(VERSION 2.8)

project (cspot)

include(cmake/Other.cmake)

add_library(woof 
  log.c 
  log.h
  woofc.c
  woofc.h 
  woofc-access.c
  woofc-access.h 
  woofc-cache.c
  woofc-cache.h
  event.c
  event.h
  host.c
  host.h
  sema.c
  sema.h
  lsema.c
  lsema.h
)

target_include_directories(woof PUBLIC .)
target_compile_options(woof PUBLIC -g)
target_link_libraries(woof PUBLIC mio euca_utils czmq m pthread)

add_library(woof-thread 
  woofc-thread.c 
  woofc-thread.h
)
target_include_directories(woof-thread PUBLIC .)
target_link_libraries(woof-thread PUBLIC mio euca_utils)

add_executable(woofc-namespace-platform woofc-host.c woofc-host.h)
target_link_libraries(woofc-namespace-platform PRIVATE woof)
target_compile_definitions(woofc-namespace-platform PRIVATE IS_PLATFORM)