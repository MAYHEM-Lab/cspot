#pragma once

#include <functional>
#include <memory>
#include <stdint.h>
#include <string_view>

namespace cspot {
class network_backend {
public:
    virtual bool listen(std::string_view ns) = 0;
    virtual bool stop() = 0;

    virtual int32_t remote_get(std::string_view woof_name, void* elem, uint32_t elem_size, uint32_t seq_no) = 0;
    virtual int32_t
    remote_get_tail(std::string_view woof_name, void* elements, unsigned long el_size, int el_count) = 0;

    virtual int32_t
    remote_put(std::string_view woof_name, const char* handler_name, const void* elem, uint32_t elem_size) = 0;

    virtual int32_t remote_get_elem_size(std::string_view woof_name) = 0;
    virtual int32_t remote_get_latest_seq_no(std::string_view woof_name,
                                             const char* cause_woof_name,
                                             uint32_t cause_woof_latest_seq_no) = 0;

    virtual ~network_backend() = default;
};

std::unique_ptr<network_backend> get_backend_with_name(std::string_view backend_name);
void register_backend(std::string name, std::function<std::unique_ptr<network_backend>()> factory);

network_backend* get_active_backend();
void set_active_backend(std::unique_ptr<network_backend> backend);
} // namespace cspot