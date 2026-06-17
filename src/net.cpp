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

// maps are not thread safe -> each thread needs a backend
thread_local int registered = 0;
thread_local std::unordered_map<std::string, std::function<std::unique_ptr<network_backend>()>> backend_factories;
thread_local std::unique_ptr<network_backend> active_backend;

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
	registered = 1;
#else
       	zmq::backend_register();
	if((CMQ_use_mqtt == 1) && cmq_pkt_init()) {
		cmq::backend_register();
	}
	registered = 1;
#endif

//        atexit([]{
//            active_backend.reset();
//        });
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
#if 0
    if(CMQ_use_mqtt == 1) {
    	DEBUG_LOG("Registering backend %s and mqtt backend\n",name.c_str());
    } else {
    	DEBUG_LOG("Registering backend %s\n",name.c_str());
    }
    fflush(stdout);
#endif
    backend_factories.emplace(std::move(name), std::move(factory));
}

void check_backends()
{
	if(registered == 0) { // thread local
		zmq::backend_register();
		cmq::backend_register();
		registered = 1;
	}
	return;
}

network_backend* get_active_backend() {
    if (!active_backend) {
        cspot::set_active_backend(cspot::get_backend_with_name(BACKEND));
//        DEBUG_WARN("No active network backend, using %s",BACKEND);
    }
    return active_backend.get();
}

void set_active_backend(std::unique_ptr<network_backend> backend) {
    active_backend = std::move(backend);
}

} // namespace cspot

extern "C" {
extern unsigned int CMQ_use_mqtt;

int backend_from_woof(const char *woof_name)
{
	if((strncmp(woof_name,"woof://",strlen("woof://")) == 0) ||
           (strncmp(woof_name,"zmq://",strlen("zmq://")) == 0)){
		return(1);
	} else if(strncmp(woof_name,"cmq://",strlen("cmq://")) == 0) {
		return(2);
	} else if(strncmp(woof_name,"mqtt://",strlen("mqtt://")) == 0) {
		return(3);
	} else {
		return(-1);
	}
}

cspot::network_backend *adjust_active_backend(const char *woof_name) 
{
	int be;
	cspot::network_backend *nbe = NULL;

	be = backend_from_woof(woof_name);
	if(be != -1) {
		if(be == 1) {
			cspot::set_active_backend(cspot::get_backend_with_name("zmq"));
		} else if(be == 2) {
			CMQ_use_mqtt = 0;
			cspot::set_active_backend(cspot::get_backend_with_name("cmq"));
		} else if(be == 3) {
			CMQ_use_mqtt = 1;
			cspot::set_active_backend(cspot::get_backend_with_name("cmq"));
		}
		nbe = cspot::get_active_backend();
	} else {
	}
	if(be == -1) {
		return nullptr;
	} else {
		return(nbe);
	}
}

unsigned long WooFMsgPut(const char* woof_name, const char* hand_name, const void* element, unsigned long el_size) {
	cspot::network_backend *be;
	cspot::check_backends();
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
	cspot::check_backends();
	be = adjust_active_backend(woof_name);
	if(be != NULL) {
		return(be->remote_get(woof_name, element, el_size, seq_no));
	} else {
		return(-1);
	}
//	return cspot::get_active_backend()->remote_get(woof_name, element, el_size, seq_no);
}

int WooFMsgGetRange(const char* woof_name, void* elements, 
			unsigned long el_size, unsigned long seq_no,
			unsigned int count) {
	cspot::network_backend *be;
	cspot::check_backends();
	be = adjust_active_backend(woof_name);
	if(be != NULL) {
		int ret = be->remote_get_range(woof_name, elements, el_size, seq_no, count);
		return(ret);
	} else {
		return(-1);
	}
//	return cspot::get_active_backend()->remote_get(woof_name, element, el_size, seq_no);
	return(1);
}

int WooFMsgCreate(const char* woof_name, unsigned long el_size, unsigned long history_size) {
	cspot::network_backend *be;
	cspot::check_backends();
	be = adjust_active_backend(woof_name);
	if(be != NULL) {
		return(be->remote_create(woof_name, el_size, history_size));
	} else {
		return(-1);
	}
//	return cspot::get_active_backend()->remote_get(woof_name, element, el_size, seq_no);
}

unsigned long WooFMsgGetElSize(const char* woof_name) {
	thread_local cspot::network_backend *be;
	unsigned long el_size;
	cspot::check_backends();
	be = adjust_active_backend(woof_name);
	if(be != NULL) {
		return(be->remote_get_elem_size(woof_name));
	} else {
		return(-1);
	}
//	return cspot::get_active_backend()->remote_get_elem_size(woof_name);
}

unsigned long WooFMsgGetEarliestSeqno(const char* woof_name)
{
	thread_local cspot::network_backend *be;
	unsigned long el_size;
	cspot::check_backends();
	be = adjust_active_backend(woof_name);
	if(be != NULL) {
		return(be->remote_get_earliest_seq_no(woof_name));
	} else {
		return(-1);
	}
//	return cspot::get_active_backend()->remote_get_elem_size(woof_name);
}
unsigned long
WooFMsgGetLatestSeqno(const char* woof_name, const char* cause_woof_name, unsigned long cause_woof_latest_seq_no) {
	cspot::network_backend *be;
	cspot::check_backends();
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
