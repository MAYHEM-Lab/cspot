cmake_minimum_required(VERSION 3.13)
project(cspot VERSION 2.0.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(BUILD_SHARED_LIBS OFF)
option(ENABLE_PYCSPOT "Enable cspot python bindings" OFF)
option(ENABLE_FEATURE_REPAIR "Enable repair" OFF)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    message(STATUS "Emitting static binaries")
    set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++ -Wl,--gc-sections")
    set(CMAKE_C_FLAGS "-ffunction-sections -fdata-sections")
    set(CMAKE_CXX_FLAGS "-ffunction-sections -fdata-sections")

#    if (CMAKE_BUILD_TYPE MATCHES "Release")
#        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
#        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto")
#        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
#    endif()
endif ()

add_subdirectory(deps)
add_subdirectory(src)

add_library(woof
        src/log.cpp
        src/include/log.h
        src/woofc.cpp
        include/woofc.h
        src/woofc-access.cpp
        src/include/woofc-access.h
        src/event.cpp
        src/include/event.h
        src/host.c
        src/include/host.h
        src/lsema.c
        src/include/lsema.h
        src/debug.cpp
        src/include/debug.h
        src/include/global.h
        src/global.cpp
        src/include/woofc-priv.h
        src/net.cpp
        src/woofc-host.cpp
)

target_include_directories(woof PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/include>
        )

target_include_directories(woof PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

find_package(Threads REQUIRED)

target_link_libraries(woof PUBLIC mio euca_utils m Threads::Threads uriparser2 fmt::fmt woof_zmq_net)
target_compile_features(woof PUBLIC cxx_std_17)

if (${CMAKE_BUILD_TYPE} MATCHES Debug)
    target_compile_definitions(woof PUBLIC DEBUG=1)
endif ()

if (ENABLE_FEATURE_REPAIR)
    target_compile_definitions(woof PUBLIC REPAIR=1)
    target_sources(woof PRIVATE src/repair.c)
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(woof PUBLIC -Wall -Wextra)
endif ()

add_executable(woofc-namespace-platform src/woofc-namespace-platform.cpp)
target_link_libraries(woofc-namespace-platform woof)
target_include_directories(woofc-namespace-platform PRIVATE src/include)

add_executable(woofc-container src/woofc-container.cpp)
target_link_libraries(woofc-container PRIVATE woof)
target_include_directories(woofc-container PRIVATE src/include)

add_library(woofc-shepherd src/woofc-shepherd.cpp)
target_link_libraries(woofc-shepherd PUBLIC woof)
target_include_directories(woofc-shepherd PRIVATE src/include)

function (add_handler name)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${name}_handler_fwd.cpp
"#include \"woofc.h\"\n \
\
extern \"C\" { \
int ${name}(WOOF* wf, unsigned long seq_no, void* ptr); \
 \
int handler(WOOF* wf, unsigned long seq_no, void* ptr) { \
    return ${name}(wf, seq_no, ptr); \
} \
} \
")
    add_executable(${name} ${ARGV1})
    target_link_libraries(${name} PRIVATE woofc-shepherd)
    target_sources(${name} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/${name}_handler_fwd.cpp")
endfunction()

add_subdirectory(apps)

set(CMAKE_EXPORT_PACKAGE_REGISTRY ON)
install(
    TARGETS woof mio euca_utils uriparser2 fmt woof_zmq_net
    EXPORT woof
    DESTINATION lib
)
install(
    TARGETS woofc-shepherd woofc-container woofc-namespace-platform
    EXPORT woof
    DESTINATION bin
)
#install(TARGETS czmq-static EXPORT woof DESTINATION lib)

install(EXPORT woof DESTINATION lib/cmake/woof)
#install(EXPORT czmq-targets DESTINATION lib)
#install(EXPORT ZeroMQ-targets DESTINATION lib)

install(FILES include/woofc.h DESTINATION include)
install(FILES woof-config.cmake DESTINATION lib/cmake/woof)

set(CPACK_GENERATOR "DEB;RPM;TGZ;STGZ")
set(CPACK_PACKAGE_VENDOR "Mayhem Lab")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Cspot")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Fatih")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/Readme.adoc")
include(CPack)