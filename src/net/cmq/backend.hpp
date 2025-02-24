#pragma once

#include "cmq_wrap.hpp"

#include <net.h>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <woofc-access.h>
#include <atomic>
#include <pthread.h>

namespace cspot::cmq {

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
    //ZMsgPtr ServerRequest(const char* endpoint, ZMsgPtr msg_arg);
    //std::vector<std::thread> m_threads{WOOF_MSG_THREADS};
    pthread_t tids[WOOF_MSG_THREADS];
    std::atomic<bool> m_stop_called = false;
    int listen_sd;
};
}
