#include <mutex>
#include "backend.hpp"

#include <debug.h>
#include <woofc-access.h>
#include <signal.h>

namespace cspot::cmq {

extern int CMQ_use_mqtt;
void backend_register() {
    CMQ_use_mqtt = CMQMQTTXPORT; // defined in woodc-access.h
    register_backend("cmq", [] { return std::make_unique<backend>(); });
}
} // namespace cspot::cmq
