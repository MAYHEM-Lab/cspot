#include <mutex>
#include <thread>
#include "net.h"
#include <pthread.h>
#include <string.h>

#include <debug.h>
#include <functional>
#include <unordered_map>
#include <woofc-access.h>

extern "C" {
	extern unsigned int CMQ_use_mqtt;
}

namespace cspot {
namespace zmq {
void backend_register();
} // namespace zmq
namespace cmq {
	void backend_register();
} // namespace cmq
std::unordered_map<std::string, std::function<std::unique_ptr<network_backend>()>> backend_factories;
std::unique_ptr<network_backend> active_backend;
namespace {
struct registerer {
    registerer() {
	if(CMQ_use_mqtt == 1) {
        	zmq::backend_register();
		cmq::backend_register();
	} else {
#ifdef USE_CMQ
		cmq::backend_register();
#else
        	zmq::backend_register();
#endif
	}

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
//    DEBUG_LOG("Registering backend %s\n", name.c_str());
// not sure why but with DEBUG on, this log call causes the 33rd process spawn to hang (on ubuntu, at least)
#ifdef DEBUG
    printf("Registering backend %s\n",name.c_str());
    fflush(stdout);
#endif
    backend_factories.emplace(std::move(name), std::move(factory));
}

std::mutex RegMutex;
//pthread_mutex_t RLock;
network_backend* get_active_backend() {
//    const std::lock_guard<std::mutex> lock(RegMutex);
//    pthread_mutex_lock(&RLock);
    if (!active_backend) {
        cspot::set_active_backend(cspot::get_backend_with_name(BACKEND));
//        DEBUG_WARN("No active network backend, using %s",BACKEND);
    }
//    pthread_mutex_unlock(&RLock);
    return active_backend.get();
}

void set_active_backend(std::unique_ptr<network_backend> backend) {
    active_backend = std::move(backend);
}
} // namespace cspot

extern "C" {
extern unsigned int CMQ_use_mqtt;
const char *backend_from_woof(const char *woof_name)
{
	if((strstr(woof_name,"woof://") != NULL) ||
           (strstr(woof_name,"zmq://") != NULL)){
		return("zmq");
	} else if(strstr(woof_name,"cmq://") != NULL) {
		return("cmq");
	} else if(strstr(woof_name,"mqtt://") != NULL) {
		return("mqtt");
	} else {
		return(NULL);
	}
}
void adjust_active_backend(const char *woof_name) {
	const char *be = backend_from_woof(woof_name);
	if(be != NULL) {
		if(strcmp(be,"zmq") == 0) {
			cspot::set_active_backend(cspot::get_backend_with_name("zmq"));
		} else if(strcmp(be,"cmq") == 0) {
			CMQ_use_mqtt = 0;
			cspot::set_active_backend(cspot::get_backend_with_name("cmq"));
		} else if(strcmp(be,"mqtt") == 0) {
			CMQ_use_mqtt = 1;
			cspot::set_active_backend(cspot::get_backend_with_name("cmq"));
		}
	}
	return;
}
unsigned long WooFMsgPut(const char* woof_name, const char* hand_name, const void* element, unsigned long el_size) {
	adjust_active_backend(woof_name);
	return cspot::get_active_backend()->remote_put(woof_name, hand_name, element, el_size);
}

int WooFMsgGet(const char* woof_name, void* element, unsigned long el_size, unsigned long seq_no) {
	adjust_active_backend(woof_name);
	return cspot::get_active_backend()->remote_get(woof_name, element, el_size, seq_no);
}

unsigned long WooFMsgGetElSize(const char* woof_name) {
	adjust_active_backend(woof_name);
	return cspot::get_active_backend()->remote_get_elem_size(woof_name);
}

unsigned long
WooFMsgGetLatestSeqno(const char* woof_name, const char* cause_woof_name, unsigned long cause_woof_latest_seq_no) {
	adjust_active_backend(woof_name);
    	return cspot::get_active_backend()->remote_get_latest_seq_no(woof_name, cause_woof_name, cause_woof_latest_seq_no);
}

unsigned long WooFMsgGetTail(const char* woof_name, void* elements, unsigned long el_size, int el_count) {
    return -1;
}

int WooFMsgServer(const char* woof_namespace) {
    cspot::set_active_backend(cspot::get_backend_with_name("cmq"));
    if (!cspot::get_active_backend()->listen(woof_namespace)) {
        return -1;
    }
    cspot::set_active_backend(cspot::get_backend_with_name("zmq"));
    if (!cspot::get_active_backend()->listen(woof_namespace)) {
        return -1;
    }
    return cspot::get_active_backend()->stop() ? 0 : -1;
}
}
