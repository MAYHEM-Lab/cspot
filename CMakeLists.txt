cmake_minimum_required(VERSION 3.10)
project(cspot)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(deps)
add_subdirectory(src)

add_library(woof 
  src/log.c 
  include/log.h
  src/woofc.cpp
  include/woofc.h
  src/woofc-access.cpp
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
)

find_package(Threads REQUIRED)
find_package(czmq REQUIRED)

target_include_directories(woof PUBLIC include)
target_link_libraries(woof PUBLIC mio euca_utils czmq-static m Threads::Threads uriparser2 fmt::fmt)
target_compile_features(woof PUBLIC cxx_std_17)

if (${CMAKE_BUILD_TYPE} MATCHES Debug)
    target_compile_definitions(woof PUBLIC DEBUG=1)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(woof PUBLIC -Wall -Wextra)
endif()

add_library(woof-thread 
  src/woofc-thread.c 
  include/woofc-thread.h
)
target_include_directories(woof-thread PUBLIC include)
target_link_libraries(woof-thread PUBLIC mio euca_utils)

add_executable(woofc-namespace-platform src/woofc-host.cpp include/woofc-host.h)
target_link_libraries(woofc-namespace-platform PRIVATE woof)
target_compile_definitions(woofc-namespace-platform PRIVATE IS_PLATFORM)

add_executable(woofc-container src/woofc-container.cpp)
target_link_libraries(woofc-container PRIVATE woof)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_link_options(woofc-container PRIVATE -static-libgcc)
    target_link_options(woofc-container PRIVATE -static-libstdc++)
endif()