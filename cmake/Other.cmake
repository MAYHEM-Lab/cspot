cmake_minimum_required(VERSION 2.8)

set(DIR_OF_CSPOT_CMAKE ${CMAKE_CURRENT_LIST_DIR})  

set(DEP "${DIR_OF_CSPOT_CMAKE}/../..")
set(WOOFC "${DIR_OF_CSPOT_CMAKE}/..")

add_library(euca_utils INTERFACE IMPORTED)
set_target_properties(euca_utils PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${DEP}/euca-cutils"
    INTERFACE_LINK_LIBRARIES "${DEP}/euca-cutils/libutils.a"
)
