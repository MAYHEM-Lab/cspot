#pragma once

#include "zmq_wrap.hpp"

#include <czmq.h>
#include <net.h>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <woofc-access.h>
#include <atomic>

namespace cspot::zmq {
class per_endpoint_data {
public:
    ZServerPtr server;
    ZPollerPtr resp_poll;

    static std::optional<per_endpoint_data> create(const std::string& ep);
};

class backend : public network_backend {
public:
    bool listen(std::string_view ns) override;
    bool stop() override;

    int32_t remote_get(std::string_view woof_name, void* elem, uint32_t elem_size, uint32_t seq_no) override;
    int32_t remote_get_tail(std::string_view woof_name, void* elements, unsigned long el_size, int el_count) override;
    int32_t remote_put(std::string_view woof_name, const char* handler_name, const void* elem, uint32_t elem_size) override;
    int32_t remote_get_elem_size(std::string_view woof_name) override;
    int32_t remote_get_latest_seq_no(std::string_view woof_name,
                                     const char* cause_woof_name,
                                     uint32_t cause_woof_latest_seq_no) override;

private:
    ZMsgPtr ServerRequest(const char* endpoint, ZMsgPtr msg_arg);
    per_endpoint_data* get_local_socket_for(const std::string& endpoint);

    using AddrSockMapT = std::unordered_map<std::string, per_endpoint_data>;

    using ThreadMapT = std::unordered_map<std::thread::id, AddrSockMapT>;

    ThreadMapT m_per_thread_socks;

    std::vector<std::thread> m_threads{WOOF_MSG_THREADS};

    ZActorPtr m_proxy;
    std::atomic<bool> m_stop_called = false;
};
}