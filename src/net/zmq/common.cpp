#include "common.hpp"

#include "debug.h"

#include <fmt/format.h>
#include <woofc-access.h>

namespace cspot::zmq {
std::optional<std::string> endpoint_from_woof(std::string_view woof_name_v) {
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
            fprintf(stderr, "WooFMsgGetElSize: woof: %s invalid IP address\n", woof_name.c_str());
            return {};
        }
    }

    auto port = port_from_uri(woof_name.c_str());

    if (!port) {
        port = WooFPortHash(woof_namespace->c_str());
    }

    return fmt::format(">tcp://{}:{}", *ip, *port);
}
} // namespace cspot::zmq
