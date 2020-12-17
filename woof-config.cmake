find_package(ZeroMQ REQUIRED)
find_package(czmq REQUIRED)
find_package(Threads REQUIRED)

get_filename_component(SELF_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
include(${SELF_DIR}/woof.cmake)

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