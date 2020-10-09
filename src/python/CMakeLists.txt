find_package(Python3)

if (NOT Python3_FOUND)
    return()
endif()

pybind11_add_module(pycspot bind.cpp)
target_link_libraries(pycspot PUBLIC woof)
