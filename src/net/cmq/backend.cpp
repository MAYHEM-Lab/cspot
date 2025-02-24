#include <mutex>
#include "backend.hpp"

#include <czmq.h>
#include <debug.h>
#include <woofc-access.h>
#include <signal.h>

namespace cspot::cmq {

void backend_register() {
    register_backend("cmq", [] { return std::make_unique<backend>(); });
}
} // namespace cspot::cmq
