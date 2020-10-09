#include "pybind11/pybind11.h"
#include "woofc.h"
#include <pybind11/stl.h>

#include <iostream>

extern "C" {
LOG* Name_log;
unsigned long Name_id;
char WooF_namespace[2048];
char WooF_dir[2048];
char Host_ip[25];
char WooF_namelog_dir[2048];
char Namelog_name[2048];
}

namespace py = pybind11;

namespace bind {
int WooFCreate(const std::string& name, int elem_size, int hist_size) {
    strcpy(WooF_dir, "/tmp");
    std::cerr << "WooFCreate called\n";
    return ::WooFCreate(const_cast<char*>(name.c_str()), elem_size, hist_size);
}

unsigned long WooFPut(const std::string& name, const std::string& handler_name, py::bytes data) {
    strcpy(WooF_dir, "/tmp");
    std::cerr << "WooFPut called\n";
    std::string raw_data = data;
    return ::WooFPut(
        const_cast<char*>(name.c_str()), const_cast<char*>(handler_name.c_str()), const_cast<char*>(raw_data.data()));
}
} // namespace bind

PYBIND11_MODULE(pycspot, m) {
    m.def("WooFCreate", bind::WooFCreate);
    m.def("WooFPut", bind::WooFPut);
}