#include <mutex>
#include "backend.hpp"

#include <debug.h>
#include <woofc-access.h>

std::mutex EPMutex;

namespace cspot::cmq {

void backend_register() {
    register_backend("cmq", [] { return std::make_unique<backend>(); });
}
} // namespace cspot::cmq
