#include <mutex>
#include "backend.hpp"

#include <debug.h>
#include <woofc-access.h>
#include "cmq-pkt.h"
#include <signal.h>

namespace cspot::cmq {

void backend_shutdown(int sig) {
	cmq_pkt_shutdown();
	return;
}

void backend_register() {
    signal(SIGINT,backend_shutdown);
    signal(SIGTERM,backend_shutdown);
    atexit(cmq_pkt_shutdown); // for mqtt xport
    register_backend("cmq", [] { return std::make_unique<backend>(); });
}
} // namespace cspot::cmq
