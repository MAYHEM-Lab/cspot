#include <mutex>
#include "backend.hpp"

#include <debug.h>
#include <woofc-access.h>

std::mutex EPMutex;

namespace cspot::zmq {
per_endpoint_data* backend::get_local_socket_for(const std::string& endpoint) {

//    const std::lock_guard<std::mutex> lock(EPMutex);
    auto& map_for_thread = m_per_thread_socks[std::this_thread::get_id()];
    auto it = map_for_thread.find(endpoint);
    if (it == map_for_thread.end()) {
        // Socket does not exist
//printf("creating socket for %s\n",endpoint.c_str());
//fflush(stdout);
        auto ep_data = cspot::zmq::per_endpoint_data::create(endpoint);
        if (!ep_data) {
            return nullptr;
        }
        auto [i, ins] = map_for_thread.emplace(endpoint, std::move(*ep_data));
        it = i;
    } else {
#if 0 // doesn't work with cspot 2.0 but does with 1.0
	map_for_thread.erase(endpoint); // should call destructor on server and poller
	auto ep_data = cspot::zmq::per_endpoint_data::create(endpoint);
        if (!ep_data) {
            return nullptr;
        }
        auto [i, ins] = map_for_thread.emplace(endpoint, std::move(*ep_data));
        it = i;
#endif
//printf("found socket for %s\n",endpoint.c_str());
//fflush(stdout);
    	return &it->second;
    }

    return &it->second;
}

per_endpoint_data* backend::reset_local_socket_for(const std::string& endpoint) {

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
        return &it->second;
    } else {
 // doesn't work with cspot 2.0 but does with 1.0
	map_for_thread.erase(endpoint); // should call destructor on server and poller
	auto ep_data = cspot::zmq::per_endpoint_data::create(endpoint);
        if (!ep_data) {
            return nullptr;
        }
        auto [i, ins] = map_for_thread.emplace(endpoint, std::move(*ep_data));
        it = i;
    	return &it->second;
    }

}

ZMsgPtr backend::ServerRequest(const char* endpoint, ZMsgPtr msg) {
    auto ep_data = get_local_socket_for(endpoint);
    if (!ep_data) {
	printf("ServerRequest: failed to create endpoint for %s\n",endpoint);
	fflush(stdout);
        return nullptr;
    }

    zmsg_t *dmsg = msg.get();
    zmsg_t *dup_msg = zmsg_dup(dmsg);
    auto sent = Send(std::move(msg), *ep_data->server);

    if (!sent) {
        DEBUG_WARN("ServerRequest: msg send to %s failed\n", endpoint);
        printf("ServerRequest: msg send to %s failed\n", endpoint);
	fflush(stdout);
	ep_data = reset_local_socket_for(endpoint);
	if(!ep_data) {
		printf("ServerRequest: failed to reset endpoint for %s\n",endpoint);
       	        return nullptr;
	} else {
		printf("ServerRequest: attempting send with reset endpoint for %s\n",endpoint);
    		//sent = Send(std::move(msg), *ep_data->server);
    		int rc = zmsg_send(&dup_msg, ep_data->server.get());
		if(rc != 0) {
			printf("ServerRequest: send with reset failed to endpoint for %s\n",endpoint);
			return nullptr;
		}
	}
    } else {
	    zmsg_destroy(&dup_msg);
    }

    auto server_resp = static_cast<zsock_t*>(zpoller_wait(ep_data->resp_poll.get(), WOOF_MSG_REQ_TIMEOUT));
    if (!server_resp) {
        if (zpoller_expired(ep_data->resp_poll.get())) {
            DEBUG_WARN("ServerRequest: msg recv timeout from %s after %d msec\n", endpoint, WOOF_MSG_REQ_TIMEOUT);
	    printf("ServerRequest: msg recv timeout from %s after %d msec\n", endpoint, WOOF_MSG_REQ_TIMEOUT);
	    fflush(stdout);
        } else if (zpoller_terminated(ep_data->resp_poll.get())) {
            DEBUG_WARN("ServerRequest: msg recv interrupted from %s\n", endpoint);
            printf("ServerRequest: msg recv interrupted from %s\n", endpoint);
	    fflush(stdout);
        } else {
            DEBUG_WARN("ServerRequest: msg recv failed from %s\n", endpoint);
            printf("ServerRequest: msg recv interrupted from %s\n", endpoint);
	    fflush(stdout);
        }
        return nullptr;
    }

    auto resp = Receive(*ep_data->server);

    if (!resp) {
        DEBUG_WARN("ServerRequest: msg recv from %s failed\n", endpoint);
        printf("ServerRequest: msg recv from %s failed\n", endpoint);
	fflush(stdout);
    }

    DEBUG_LOG("ServerRequest: completed successfully");
    return resp;
}

void backend_register() {
    zsys_init();
    register_backend("zmq", [] { return std::make_unique<backend>(); });
}
} // namespace cspot::zmq
