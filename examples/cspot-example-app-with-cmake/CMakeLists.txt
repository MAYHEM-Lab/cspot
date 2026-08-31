if(APPLE)
cmake_minimum_required(VERSION 3.13...4.0)
else()
cmake_minimum_required(VERSION 3.13)
endif()
project(senspot-build C CXX)

if(EXISTS "/opt/libyaml-musl/include/yaml.h" AND EXISTS "/opt/openssl-musl/include/openssl/ssl.h")
    message(STATUS "Using musl dependency stack")

    include_directories(
        /opt/libyaml-musl/include
        /opt/openssl-musl/include
        ${PROJECT_SOURCE_DIR}/deps/fmt/include
    )

    link_directories(
        /opt/libyaml-musl/lib
        /opt/openssl-musl/lib
        /opt/openssl-musl/lib64
    )
endif()

if(APPLE)
    message(STATUS "Configuring for Apple Silicon (ARM64)")

    set(CMAKE_OSX_ARCHITECTURES arm64)
    set(CMAKE_PREFIX_PATH "/opt/homebrew;/opt/homebrew/opt/openssl@3")
    set(OPENSSL_ROOT_DIR "/opt/homebrew/opt/openssl@3")


    # Manually specify Homebrew ARM64 paths
    set(OPENSSL_ROOT_DIR "/opt/homebrew/opt/openssl@3")
    set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/include")
    set(OPENSSL_LIBRARIES "${OPENSSL_ROOT_DIR}/lib")

#    include_directories(/opt/homebrew/include)
#    link_directories(/opt/homebrew/lib)
    find_package(OpenSSL REQUIRED)
    find_library(YAML_LIBRARY yaml HINTS /opt/homebrew/lib REQUIRED)
    find_path(YAML_INCLUDE_DIR yaml.h HINTS /opt/homebrew/include REQUIRED)


#    set(CMAKE_PREFIX_PATH "/opt/homebrew/opt/libyaml")
#else()
#    include_directories("/opt/openssl-musl/include")
#    link_directories("/opt/openssl-musl/lib" "/opt/libyaml-musl/lib")

else()
        set(USE_MUSL_DEPS OFF)
                if(EXISTS "/opt/musl-cross/bin/x86_64-linux-musl-gcc")
                set(USE_MUSL_DEPS ON)
#               add_compile_options(-g -O0 -fno-omit-frame-pointer)
        endif()
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

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


set(WOOF_PREFIX "${CMAKE_SOURCE_DIR}")
set(WOOF_LIBDIR "${WOOF_PREFIX}/lib")
set(WOOF_INCLUDEDIR "${WOOF_PREFIX}/include")

add_library(woof STATIC IMPORTED)
set_target_properties(woof PROPERTIES
    IMPORTED_LOCATION "${WOOF_LIBDIR}/libwoof.a"
    INTERFACE_INCLUDE_DIRECTORIES "${WOOF_INCLUDEDIR}"
    INTERFACE_LINK_LIBRARIES
        "woof_zmq_net;woof_cmq_net;woof_caplets;mio;euca_utils;uriparser2;fmt;czmq;zmq;yaml;crypto;pthread;m;dl"
)

foreach(lib woof_zmq_net woof_cmq_net woof_caplets mio euca_utils uriparser2 czmq zmq)
    add_library(${lib} STATIC IMPORTED)
    set_target_properties(${lib} PROPERTIES
        IMPORTED_LOCATION "${WOOF_LIBDIR}/lib${lib}.a"
    )
endforeach()

add_library(woofc-shepherd STATIC IMPORTED)

set_target_properties(woofc-shepherd PROPERTIES
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/bin/libwoofc-shepherd.a"
)

add_library(fmt STATIC IMPORTED)
set_target_properties(fmt PROPERTIES
    IMPORTED_LOCATION "${WOOF_LIBDIR}/libfmtd.a"
)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

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


add_subdirectory(cspot-app-example)
