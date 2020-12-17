project(mio)

add_library(mio mio.c mio.h mymalloc.c mymalloc.h)
target_link_libraries(mio PUBLIC euca_utils)

file(GLOB MIO_HEADERS "*.h")
install(FILES ${MIO_HEADERS} DESTINATION include)

target_include_directories(mio PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
        )