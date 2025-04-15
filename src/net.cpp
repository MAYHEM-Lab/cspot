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
	extern int cmq_pkt_init();
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
pthread_mutex_t ELock;
struct registerer {
    registerer() {
	pthread_mutex_init(&ELock,NULL);
#ifdef USE_CMQ
	if(CMQ_use_mqtt == 1) {
		if(cmq_pkt_init()) {
			cmq::backend_register();
		} 
	} else {
		cmq::backend_register();
	}
#else
       	zmq::backend_register();
	if((CMQ_use_mqtt == 1) && cmq_pkt_init()) {
		cmq::backend_register();
	}
#endif

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
    if(CMQ_use_mqtt == 1) {
    	printf("Registering backend %s and mqtt backend\n",name.c_str());
    } else {
    	printf("Registering backend %s\n",name.c_str());
    }
    fflush(stdout);
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
cspot::network_backend *adjust_active_backend(const char *woof_name) {
	const char *be;
	cspot::network_backend *nbe = NULL;
	pthread_mutex_lock(&cspot::ELock);
	be = backend_from_woof(woof_name);
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
		nbe = cspot::get_active_backend();
	}
	pthread_mutex_unlock(&cspot::ELock);
	return(nbe);
}
unsigned long WooFMsgPut(const char* woof_name, const char* hand_name, const void* element, unsigned long el_size) {
	cspot::network_backend *be;
	be = adjust_active_backend(woof_name);
	if(be != NULL) {
		return(be->remote_put(woof_name, hand_name, element, el_size));
	} else {
		return(-1);
	}
//	return cspot::get_active_backend()->remote_put(woof_name, hand_name, element, el_size);
}

int WooFMsgGet(const char* woof_name, void* element, unsigned long el_size, unsigned long seq_no) {
	cspot::network_backend *be;
	be = adjust_active_backend(woof_name);
	if(be != NULL) {
		return(be->remote_get(woof_name, element, el_size, seq_no));
	} else {
		return(-1);
	}
//	return cspot::get_active_backend()->remote_get(woof_name, element, el_size, seq_no);
}

unsigned long WooFMsgGetElSize(const char* woof_name) {
	cspot::network_backend *be;
	be = adjust_active_backend(woof_name);
	if(be != NULL) {
		return(be->remote_get_elem_size(woof_name));
	} else {
		return(-1);
	}
//	return cspot::get_active_backend()->remote_get_elem_size(woof_name);
}

unsigned long
WooFMsgGetLatestSeqno(const char* woof_name, const char* cause_woof_name, unsigned long cause_woof_latest_seq_no) {
	cspot::network_backend *be;
	be = adjust_active_backend(woof_name);
	if(be != NULL) {
		return(be->remote_get_latest_seq_no(woof_name, cause_woof_name, cause_woof_latest_seq_no));
	} else {
		return(-1);
	}
//    	return cspot::get_active_backend()->remote_get_latest_seq_no(woof_name, cause_woof_name, cause_woof_latest_seq_no);
}

unsigned long WooFMsgGetTail(const char* woof_name, void* elements, unsigned long el_size, int el_count) {
    return -1;
}


//
// this is complicated
//
// zmq and cmq can only both be configured wher cmq is operating in MQTT mode
// (otherwise they have a port conflict for the listen port)
//
// Also, there is a race condition between the set backend and the get which is why there is a lock
// FIX: this should work with "get_backend_with_name" but the rest of the platform crashes
int WooFMsgServer(const char* woof_namespace) {
	cspot::network_backend *be;

#ifdef USE_CMQ
	if(CMQ_use_mqtt == 1) {
		pthread_mutex_lock(&cspot::ELock);
		cspot::set_active_backend(cspot::get_backend_with_name("cmq"));
		be = cspot::get_active_backend();
		pthread_mutex_unlock(&cspot::ELock);

		if (be != NULL) {
			be->listen(woof_namespace);
//			CMQ_use_mqtt = 0;
//			be->listen(woof_namespace);
//			CMQ_use_mqtt = 1;
		} else {
			return -1;
		}
	} else {
		pthread_mutex_lock(&cspot::ELock);
		cspot::set_active_backend(cspot::get_backend_with_name("cmq"));
		be = cspot::get_active_backend();
		pthread_mutex_unlock(&cspot::ELock);
		if(be != NULL) {
			be->listen(woof_namespace);
		}
	}

#else // use zmq
	if(CMQ_use_mqtt == 1) {
		pthread_mutex_lock(&cspot::ELock);
		cspot::set_active_backend(cspot::get_backend_with_name("cmq"));
		be = cspot::get_active_backend();
		pthread_mutex_unlock(&cspot::ELock);
		if(be != NULL) {
			be->listen(woof_namespace);
		} else {
        		return -1;
		}
	}

	pthread_mutex_lock(&cspot::ELock);
	cspot::set_active_backend(cspot::get_backend_with_name("zmq"));
	be = cspot::get_active_backend();
	pthread_mutex_unlock(&cspot::ELock);

	if (be != NULL) {
		be->listen(woof_namespace);
	} else {
        	return -1;
	}
	// cmq and cmq+mqtt can co-exist if is zmq is not enabled
#endif
	return cspot::get_active_backend()->stop() ? 0 : -1;
}
}
