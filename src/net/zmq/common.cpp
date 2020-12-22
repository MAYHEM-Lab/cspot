#include "common.hpp"

#include "debug.h"

#include <fmt/format.h>
#include <woofc-access.h>

namespace cspot::zmq {
std::optional<std::string> endpoint_from_woof(std::string_view woof_name_v) {
    std::string woof_name(woof_name_v);

    char woof_namespace[2048] = {};
    auto err = WooFNameSpaceFromURI(woof_name.c_str(), woof_namespace, sizeof(woof_namespace));
    if (err < 0) {
        DEBUG_WARN("WooFMsgGetElSize: woof: %s no name space\n", woof_name.c_str());
        return {};
    }

    char ip_str[25] = {};
    err = WooFIPAddrFromURI(woof_name.c_str(), ip_str, sizeof(ip_str));
    if (err < 0) {
        /*
         * try local addr
         */
        err = WooFLocalIP(ip_str, sizeof(ip_str));
        if (err < 0) {
            fprintf(stderr, "WooFMsgGetElSize: woof: %s invalid IP address\n", woof_name.c_str());
            return {};
        }
    }

    int port;
    if (WooFPortFromURI(woof_name.c_str(), &port) < 0) {
        port = WooFPortHash(woof_namespace);
    }

    return fmt::format(">tcp://{}:{}", ip_str, port);
}
} // namespace cspot::zmq
