#include <mutex>
#include "backend.hpp"

#include <debug.h>
#include <woofc-access.h>
#include "cmq-pkt.h"
#include <signal.h>

namespace cspot::cmq {

void backend_register() {
    atexit(cmq_pkt_shutdown); // for mqtt xport
    register_backend("cmq", [] { return std::make_unique<backend>(); });
}
} // namespace cspot::cmq
