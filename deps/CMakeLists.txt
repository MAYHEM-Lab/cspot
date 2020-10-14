add_subdirectory(uriparser2)
add_subdirectory(mio)
add_subdirectory(euca-cutils)
add_subdirectory(fmt)

if (ENABLE_PYCSPOT)
    add_subdirectory(pybind11)
endif()
