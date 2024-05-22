#include <mutex>
#include <thread>
#include "net.h"
#include <pthread.h>

#include <debug.h>
#include <functional>
#include <unordered_map>
#include <woofc-access.h>

namespace cspot {
namespace zmq {
void backend_register();
} // namespace zmq
std::unordered_map<std::string, std::function<std::unique_ptr<network_backend>()>> backend_factories;
std::unique_ptr<network_backend> active_backend;
namespace {
struct registerer {
    registerer() {
        zmq::backend_register();

        atexit([]{
            active_backend.reset();
        });
    }
};

} // namespace

std::unique_ptr<network_backend> get_backend_with_name(std::string_view backend_name) {
    static registerer reg;

    std::string backend_name_str(backend_name);
    auto it = backend_factories.find(backend_name_str);
    if (it == backend_factories.end()) {
        DEBUG_WARN("No such backend: %s", backend_name_str.c_str());
        return nullptr;
    }
    return it->second();
}

void register_backend(std::string name, std::function<std::unique_ptr<network_backend>()> factory) {
    DEBUG_LOG("Registering backend %s", name.c_str());
    backend_factories.emplace(std::move(name), std::move(factory));
}

std::mutex RegMutex;
//pthread_mutex_t RLock;
network_backend* get_active_backend() {
//    const std::lock_guard<std::mutex> lock(RegMutex);
//    pthread_mutex_lock(&RLock);
    if (!active_backend) {
        cspot::set_active_backend(cspot::get_backend_with_name("zmq"));
        DEBUG_WARN("No active network backend, using zmq");
    }
//    pthread_mutex_unlock(&RLock);
    return active_backend.get();
}

void set_active_backend(std::unique_ptr<network_backend> backend) {
    active_backend = std::move(backend);
}
} // namespace cspot

extern "C" {
unsigned long WooFMsgPut(const char* woof_name, const char* hand_name, const void* element, unsigned long el_size) {
    return cspot::get_active_backend()->remote_put(woof_name, hand_name, element, el_size);
}

int WooFMsgGet(const char* woof_name, void* element, unsigned long el_size, unsigned long seq_no) {
    return cspot::get_active_backend()->remote_get(woof_name, element, el_size, seq_no);
}

unsigned long WooFMsgGetElSize(const char* woof_name) {
    return cspot::get_active_backend()->remote_get_elem_size(woof_name);
}

unsigned long
WooFMsgGetLatestSeqno(const char* woof_name, const char* cause_woof_name, unsigned long cause_woof_latest_seq_no) {
    return cspot::get_active_backend()->remote_get_latest_seq_no(woof_name, cause_woof_name, cause_woof_latest_seq_no);
}

unsigned long WooFMsgGetTail(const char* woof_name, void* elements, unsigned long el_size, int el_count) {
    return -1;
}

int WooFMsgServer(const char* woof_namespace) {
    if (cspot::get_active_backend()->listen(woof_namespace)) {
        return cspot::get_active_backend()->stop() ? 0 : -1;
    } else {
        return -1;
    }
}
}
