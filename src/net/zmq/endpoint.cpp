#include "backend.hpp"
#include "debug.h"

namespace cspot::zmq {
std::optional<per_endpoint_data> per_endpoint_data::create(const std::string& ep) {
    per_endpoint_data res;
    res.server = ZServerPtr(zsock_new_req(ep.c_str()));
    if (!res.server) {
        DEBUG_WARN("ServerRequest: no server connection to %s\n", ep.c_str());
        return {};
    }

    res.resp_poll = ZPollerPtr(zpoller_new(res.server.get(), nullptr));
    if (!res.resp_poll) {
        DEBUG_WARN("ServerRequest: no poller for reply from %s\n", ep.c_str());
        return {};
    }

    return res;
}
} // namespace cspot::zmq