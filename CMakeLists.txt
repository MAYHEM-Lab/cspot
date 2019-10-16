cmake_minimum_required(VERSION 2.8)

project (cspot)

include(cmake/Other.cmake)

add_subdirectory(mio)
add_subdirectory(uriparser2)
add_subdirectory(libemsha)
add_subdirectory(caps)
add_subdirectory(net)

add_library(woof 
  src/log.c 
  include/log.h
  src/woofc.c
  include/woofc.h 
  src/woofc-access.c
  include/woofc-access.h 
  src/woofc-cache.c
  include/woofc-cache.h
  src/event.c
  include/event.h
  src/host.c
  include/host.h
  src/sema.c
  include/sema.h
  src/lsema.c
  include/lsema.h
  src/msgpack-ep.cpp
  cwpack/cwpack.c
  src/flat_ep.cpp
)

target_include_directories(woof PUBLIC include)
target_include_directories(woof PUBLIC cwpack)
target_compile_options(woof PUBLIC -g)
target_link_libraries(woof PUBLIC mio euca_utils czmq m pthread uriparser2 caps cspot_net)
target_compile_features(woof PUBLIC cxx_std_17)

add_library(woof-thread 
  src/woofc-thread.c 
  include/woofc-thread.h
)
target_include_directories(woof-thread PUBLIC include)
target_link_libraries(woof-thread PUBLIC mio euca_utils)

add_executable(woofc-namespace-platform src/woofc-host.c include/woofc-host.h)
target_include_directories(woofc-namespace-platform PUBLIC include)
target_link_libraries(woofc-namespace-platform PRIVATE woof)
target_compile_definitions(woofc-namespace-platform PRIVATE IS_PLATFORM)
