#include "common.hpp"

#include "debug.h"

#include <fmt/format.h>
#include <woofc-access.h>

namespace cspot::cmq {
std::optional<std::string> ip_from_woof(std::string_view woof_name_v) {
    std::string woof_name(woof_name_v);

    auto woof_namespace = ns_from_uri(woof_name.c_str());
    if (!woof_namespace) {
        DEBUG_WARN("WooFMsgGetElSize: woof: %s no name space\n", woof_name.c_str());
        return {};
    }

    auto ip = ip_from_uri(woof_name.c_str());
    if (!ip) {
        /*
         * try local addr
         */
        ip = local_ip();
        if (!ip) {
            fprintf(stderr, "ip_from_woof: woof: %s invalid IP address\n", woof_name.c_str());
            return {};
        }
    }

    return ip;
}

std::optional<std::string> port_from_woof(std::string_view woof_name_v) {
    std::string woof_name(woof_name_v);

    auto woof_namespace = ns_from_uri(woof_name.c_str());
    if (!woof_namespace) {
        DEBUG_WARN("port_from_woof: woof: %s no name space\n", woof_name.c_str());
        return {};
    }
    auto port = port_from_uri(woof_name.c_str());

    if (!port) {
        port = WooFPortHash(woof_namespace->c_str());
    }

    return std::to_string(port.value());
}

} // namespace cspot::cmq
