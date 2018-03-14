cmake_minimum_required(VERSION 3.6)

set(DIR_OF_CSPOT_CMAKE ${CMAKE_CURRENT_LIST_DIR})  

set(DEP "${DIR_OF_CSPOT_CMAKE}/../..")
set(WOOFC "${DIR_OF_CSPOT_CMAKE}/..")

add_library(euca_utils INTERFACE IMPORTED)
set_target_properties(euca_utils PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${DEP}/euca-cutils"
    INTERFACE_LINK_LIBRARIES "${DEP}/euca-cutils/libutils.a"
)

add_library(mio INTERFACE IMPORTED)
set_target_properties(mio PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${DEP}/mio"
    INTERFACE_LINK_LIBRARIES "${DEP}/mio/mio.o;${DEP}/mio/mymalloc.o"
)

add_library(woof INTERFACE IMPORTED)
set_target_properties(woof PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${WOOFC}"
    INTERFACE_LINK_LIBRARIES "${WOOFC}/woofc.o;${WOOFC}/woofc-access.o;${WOOFC}/woofc-cache.o"
)

add_library(woof_host INTERFACE IMPORTED)
set_target_properties(woof_host PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${WOOFC}"
    INTERFACE_LINK_LIBRARIES "${WOOFC}/woofc-host.o"
)

add_library(woof_thread INTERFACE IMPORTED)
set_target_properties(woof_thread PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${WOOFC}"
    INTERFACE_LINK_LIBRARIES "${WOOFC}/woofc-thread.o"
)

add_library(woof_log INTERFACE IMPORTED)
set_target_properties(woof_log PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${WOOFC}"
    INTERFACE_LINK_LIBRARIES "${WOOFC}/log.o;${WOOFC}/host.o;${WOOFC}/event.o"
)

add_library(cspot INTERFACE IMPORTED)
set_target_properties(cspot PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${WOOFC}"
    INTERFACE_LINK_LIBRARIES "mio;woof;woof_log;woof_host;pthread;m;czmq;euca_utils;${WOOFC}/uriparser2/liburiparser2.a;${WOOFC}/lsema.o"
)

function(cspot_add_handler handler_name)
    add_library(${handler_name}_shepherd "${WOOFC}/woofc-shepherd.c")
    target_link_libraries(${handler_name}_shepherd PUBLIC cspot)
    target_compile_definitions(${handler_name}_shepherd PUBLIC WOOF_HANDLER_NAME=${handler_name})
    target_link_libraries(${handler_name} PUBLIC ${handler_name}_shepherd)
endfunction()