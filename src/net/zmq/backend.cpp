#include "backend.hpp"

#include <debug.h>
#include <woofc-access.h>

#undef WOOF_MSG_REQ_TIMEOUT
#define WOOF_MSG_REQ_TIMEOUT (10000)

namespace cspot::zmq {
per_endpoint_data* backend::get_local_socket_for(const std::string& endpoint) {
    auto& map_for_thread = m_per_thread_socks[std::this_thread::get_id()];

    auto it = map_for_thread.find(endpoint);
    if (it == map_for_thread.end()) {
        // Socket does not exist
        auto ep_data = cspot::zmq::per_endpoint_data::create(endpoint);
        if (!ep_data) {
            return nullptr;
        }
        auto [i, ins] = map_for_thread.emplace(endpoint, std::move(*ep_data));
        it = i;
    }

    return &it->second;
}

ZMsgPtr backend::ServerRequest(const char* endpoint, ZMsgPtr msg) {
    auto ep_data = get_local_socket_for(endpoint);
    if (!ep_data) {
        return nullptr;
    }

    auto sent = Send(std::move(msg), *ep_data->server);

    if (!sent) {
        DEBUG_WARN("ServerRequest: msg send to %s failed\n", endpoint);
        return nullptr;
    }

    auto server_resp = static_cast<zsock_t*>(zpoller_wait(ep_data->resp_poll.get(), WOOF_MSG_REQ_TIMEOUT));
    if (!server_resp) {
        if (zpoller_expired(ep_data->resp_poll.get())) {
            DEBUG_WARN("ServerRequest: msg recv timeout from %s after %d msec\n", endpoint, WOOF_MSG_REQ_TIMEOUT);
        } else if (zpoller_terminated(ep_data->resp_poll.get())) {
            DEBUG_WARN("ServerRequest: msg recv interrupted from %s\n", endpoint);
        } else {
            DEBUG_WARN("ServerRequest: msg recv failed from %s\n", endpoint);
        }
        return nullptr;
    }

    auto resp = Receive(*ep_data->server);

    if (!resp) {
        DEBUG_WARN("ServerRequest: msg recv from %s failed\n", endpoint);
    }

    DEBUG_LOG("ServerRequest: completed successfully");
    return resp;
}

void backend_register() {
    zsys_init();
    register_backend("zmq", [] { return std::make_unique<backend>(); });
}
} // namespace cspot::zmq
