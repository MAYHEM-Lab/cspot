#include "pybind11/pybind11.h"
#include "woofc.h"

#include <debug.h>
#include <global.h>
#include <pybind11/stl.h>
#include <net.h>

namespace py = pybind11;

namespace bind {
int WooFCreate(const std::string& name, int elem_size, int hist_size) {
    DEBUG_LOG("Create Called");
    return ::WooFCreate(const_cast<char*>(name.c_str()), elem_size, hist_size);
}

unsigned long WooFPut(const std::string& name, const std::string& handler_name, py::bytes data) {
    DEBUG_LOG("Put Called");
    std::string raw_data = data;
    return ::WooFPut(
        const_cast<char*>(name.c_str()), const_cast<char*>(handler_name.c_str()), const_cast<char*>(raw_data.data()));
}

py::bytes WooFGet(const std::string& name, uint32_t elem_size, uint32_t seq_no) {
    DEBUG_LOG("Get Called");
    std::vector<uint8_t> raw_data(elem_size);
    auto res = ::WooFGet(const_cast<char*>(name.c_str()), raw_data.data(), seq_no);
    if (res < 0) {
        return py::none{};
    }
    return py::bytes(reinterpret_cast<const char*>(raw_data.data()), raw_data.size());
}
} // namespace bind

namespace {
std::unordered_map<std::string, std::pair<char*, size_t>> confs{
    {"namespace", {WooF_namespace, std::size(WooF_namespace)}},
    {"woof_dir", {WooF_dir, std::size(WooF_dir)}},
    {"host_ip", {Host_ip, std::size(Host_ip)}},
    {"log_dir", {WooF_namelog_dir, std::size(WooF_namelog_dir)}},
    {"log_name", {Namelog_name, std::size(Namelog_name)}},
};
}

PYBIND11_MODULE(pycspot, m) {
    cspot::set_active_backend(cspot::get_backend_with_name("zmq"));

    m.def("Create", bind::WooFCreate);
    m.def("Put", bind::WooFPut);
    m.def("Get", bind::WooFGet);
    m.def("GetLatestSeqno", ::WooFGetLatestSeqno);
    m.def("Config", [](const py::dict& vals) {
        for (auto& [key, arr] : confs) {
            if (vals.contains(key)) {
                auto str = vals[key.c_str()].cast<std::string>();
                strncpy(arr.first, str.c_str(), arr.second);
            }
        }
    });
}
