if(APPLE)
cmake_minimum_required(VERSION 3.13...4.0)
else()
cmake_minimum_required(VERSION 3.13)
endif()
project(cspot VERSION 2.0.0)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64" OR CMAKE_OSX_ARCHITECTURES MATCHES "arm64")
    message(STATUS "Configuring for Apple Silicon (ARM64)")

    set(CMAKE_OSX_ARCHITECTURES arm64)

    # Manually specify Homebrew ARM64 paths
    set(OPENSSL_ROOT_DIR "/opt/homebrew/opt/openssl@3")
    set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/include")
    set(OPENSSL_LIBRARIES "${OPENSSL_ROOT_DIR}/lib")

    include_directories(/opt/homebrew/include)
    link_directories(/opt/homebrew/lib)

#    set(CMAKE_PREFIX_PATH "/opt/homebrew/opt/libyaml")
endif()

find_package(OpenSSL REQUIRED)

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
    set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++ -Wl,-Bstatic -lpthread")
    set(CMAKE_C_FLAGS "-I ../src/include -ffunction-sections -fdata-sections")
    set(CMAKE_CXX_FLAGS "-std=gnu++17 -ffunction-sections -fdata-sections -Wl,-Bstatic")

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

add_library(woof-mqtt
	include/woofc-mqtt.h
	src/woofc-mqtt.cpp
)


if(APPLE1)
target_include_directories(woof PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/include>
#        ${OPENSSL_INCLUDE_DIR}
        )
else()
target_include_directories(woof PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/include>
        )
endif()

if(APPLE1)
target_include_directories(woof PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
#        ${OPENSSL_INCLUDE_DIR}
)
else()
target_include_directories(woof PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
endif()

target_include_directories(woof-mqtt PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/include>
        )

target_include_directories(woof-mqtt PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

find_package(Threads REQUIRED)

if(APPLE1)
target_link_libraries(woof PUBLIC mio euca_utils m uriparser2 fmt::fmt woof_cmq_net woof_caplets Threads::Threads)
else()
target_link_libraries(woof PUBLIC mio euca_utils m uriparser2 fmt::fmt woof_zmq_net woof_cmq_net woof_caplets Threads::Threads)
endif()
target_compile_features(woof PUBLIC cxx_std_17)

target_link_libraries(woof-mqtt PUBLIC euca_utils m uriparser2)
target_compile_features(woof-mqtt PUBLIC cxx_std_17)

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

add_executable(woofc-forker-helper src/woofc-forker-helper.cpp)
target_link_libraries(woofc-forker-helper PRIVATE woof)
target_include_directories(woofc-forker-helper PRIVATE src/include)

add_library(woofc-shepherd src/woofc-shepherd.cpp)
target_link_libraries(woofc-shepherd PUBLIC woof)
target_include_directories(woofc-shepherd PRIVATE src/include)

#add_executable(woofc-mqtt-gateway src/woofc-mqtt-gateway.cpp)
if(NOT APPLE)
add_executable(woofc-mqtt-gateway src/woofc-mqtt-gateway.c)
target_link_libraries(woofc-mqtt-gateway woof woof-mqtt)
target_include_directories(woofc-mqtt-gateway PRIVATE src/include)
endif()

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
    add_executable(${name} ${ARGV})
    target_link_libraries(${name} PRIVATE woofc-shepherd)
    target_sources(${name} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/${name}_handler_fwd.cpp")
endfunction()

add_subdirectory(apps)

set(CMAKE_EXPORT_PACKAGE_REGISTRY ON)
if(APPLE)
install(
    TARGETS woof mio euca_utils uriparser2 fmt woof_cmq_net woof_zmq_net woof_caplets
    EXPORT woof
    DESTINATION lib
)
install(
    TARGETS woofc-shepherd woofc-container woofc-namespace-platform
    EXPORT woof
    DESTINATION bin
)
else()
install(
    TARGETS woof mio euca_utils uriparser2 fmt woof_zmq_net woof_cmq_net woof_caplets
    EXPORT woof
    DESTINATION lib
)
install(
    TARGETS woofc-shepherd woofc-container woofc-namespace-platform woofc-mqtt-gateway
    EXPORT woof
    DESTINATION bin
)
endif()
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
