#include <mutex>
#include <pthread.h>
#include "backend.hpp"

#include <debug.h>
#include <woofc-access.h>
#include "cmq-pkt.h"
#include <signal.h>

namespace cspot::cmq {

pthread_mutex_t shutdown_lock;

void backend_shutdown(int sig) {
	pthread_mutex_lock(&shutdown_lock);
	cmq_pkt_shutdown();
	pthread_mutex_unlock(&shutdown_lock);
	return;
}

void backend_register() {
    pthread_mutex_init(&shutdown_lock,NULL);
    signal(SIGINT,backend_shutdown);
    signal(SIGTERM,backend_shutdown);
    atexit(cmq_pkt_shutdown); // for mqtt xport
    register_backend("cmq", [] { return std::make_unique<backend>(); });
}
} // namespace cspot::cmq
