#include <iostream>
#include <string>
#include <limits>
#include <optional>
#include <thread>
#include <chrono>

#include "backend.hpp"
#include "common.hpp"
#include "debug.h"

#define DEBUG

#include <fmt/format.h>
#include <woofc-access.h>


namespace cspot::zmq {
namespace {
#if 0
void* m_zmq_ctx = nullptr;
void* m_frontend = nullptr;
void* m_backend = nullptr;
std::thread m_proxy_thread;
#endif

int safe_stoul_to_int(const std::string& str) {
    try {
        size_t pos;
        unsigned long value = std::stoul(str, &pos);

        // Ensure no extra characters exist
//        if (pos != str.size()) {
//            throw std::invalid_argument("Invalid characters in input");
//        }

        // Check for overflow
        if (value > static_cast<unsigned long>(std::numeric_limits<int>::max())) {
            throw std::out_of_range("Value out of int range");
        }

        return static_cast<int>(value); // Safe conversion
    } catch (const std::exception& e) {
        std::cerr << "Conversion error: " << e.what() << std::endl;
        return -1; // Use an error indicator
    }
}

#define DEBUG
void WooFMsgThread() {
	Msg_id = 0;
	Resp_id = 0;
    /*
     * right now, we use REQ-REP pattern from ZeroMQ.  need a way to timeout, however, as
     * this pattern blocks indefinitely on network partition
     */

    /*
     * create a reply zsock and connect it to the back end of the proxy in the msg server
     */
    auto receiver = ZServerPtr(zsock_new_rep(">inproc://workers"));
    if (!receiver) {
        perror("WooFMsgThread: couldn't open receiver");
        return;
    }

    DEBUG_LOG("WooFMsgThread: about to call receive");

    auto msg = Receive(*receiver);
    if(msg == NULL) {
            DEBUG_WARN("WooFMsgThread: NULL msg\n");
    }
    Msg_id++;
    while (msg) {
        DEBUG_LOG("WooFMsgThread: received");

	//while(Msg_id > (Resp_id + 1)) {
	//	printf("sleeping: %d %d\n",Msg_id,Resp_id);
	//	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	//}

        /*
         * WooFMsg starts with a message tag for dispatch
         */
        auto frame = PopFront(*msg);
        DEBUG_FATAL_IF(!frame, "WooFMsgThread: couldn't get tag");

        /*
         * tag in the first frame
         */
        auto str = FromFrame<std::string>(*frame);
        if (!str) {
            DEBUG_WARN("WooFMsgThread: Could not parse the message!");
            return;
        }

        auto tag = safe_stoul_to_int(*str);
	if(tag == -1) {
		DEBUG_LOG("WooFMsgThread: processing msg with bad tag\n");
		printf("WooFMsgThread: processing msg with bad tag\n");
        	msg = Receive(*receiver);
		continue;
	}
        DEBUG_LOG("WooFMsgThread: processing msg with tag: %lu\n", tag);

        switch (tag) {
        case WOOF_MSG_PUT:
            WooFProcessPut(std::move(msg), receiver.get(),1);
	    if(Msg_id != Resp_id) {
		    printf("no resp from put\n");
	    }
            break;
	case WOOF_MSG_PUT_CAP:
	    WooFProcessPutwithCAP(std::move(msg), receiver.get());
	    if(Msg_id != Resp_id) {
		    printf("no resp from put with cap\n");
	    }
	    break;
        case WOOF_MSG_GET:
            WooFProcessGet(std::move(msg), receiver.get(),1);
	    if(Msg_id != Resp_id) {
		    printf("no resp from get\n");
	    }
            break;
        case WOOF_MSG_GET_CAP:
            WooFProcessGetwithCAP(std::move(msg), receiver.get());
	    if(Msg_id != Resp_id) {
		    printf("no resp from get with cap\n");
	    }
            break;
        case WOOF_MSG_GET_RANGE:
            WooFProcessGetRange(std::move(msg), receiver.get(),1);
	    if(Msg_id != Resp_id) {
		    printf("no resp from get range\n");
	    }
            break;
        case WOOF_MSG_GET_RANGE_CAP:
            WooFProcessGetRangewithCAP(std::move(msg), receiver.get());
	    if(Msg_id != Resp_id) {
		    printf("no resp from get range with cap\n");
	    }
            break;
        case WOOF_MSG_GET_EL_SIZE:
            WooFProcessGetElSize(std::move(msg), receiver.get(),1);
	    if(Msg_id != Resp_id) {
		    printf("no resp from get el size\n");
	    }
            break;
        case WOOF_MSG_GET_EL_SIZE_CAP:
            WooFProcessGetElSizewithCAP(std::move(msg), receiver.get());
	    if(Msg_id != Resp_id) {
		    printf("no resp from get el size with cap\n");
	    }
            break;
        case WOOF_MSG_GET_EARLIEST_SEQNO:
            WooFProcessGetEarliestSeqno(std::move(msg), receiver.get(),1);
	    if(Msg_id != Resp_id) {
		    printf("no resp from eraliest\n");
	    }
            break;
        case WOOF_MSG_GET_EARLIEST_SEQNO_CAP:
            WooFProcessGetEarliestSeqnowithCAP(std::move(msg), receiver.get());
	    if(Msg_id != Resp_id) {
		    printf("no resp from eraliest with cap\n");
	    }
            break;
        case WOOF_MSG_GET_TAIL:
            WooFProcessGetTail(std::move(msg), receiver.get(),1);
            break;
        case WOOF_MSG_GET_TAIL_CAP:
            WooFProcessGetTailwithCAP(std::move(msg), receiver.get());
            break;
        case WOOF_MSG_GET_LATEST_SEQNO:
            WooFProcessGetLatestSeqno(std::move(msg), receiver.get(),1);
	    if(Msg_id != Resp_id) {
		    printf("no resp from latest\n");
	    }
            break;
        case WOOF_MSG_GET_LATEST_SEQNO_CAP:
            WooFProcessGetLatestSeqnowithCAP(std::move(msg), receiver.get());
	    if(Msg_id != Resp_id) {
		    printf("no resp from latest with cap\n");
	    }
            break;
	case WOOF_MSG_CREATE_CAP:
            WooFProcessCreatewithCAP(std::move(msg), receiver.get());
            break;
#ifdef DONEFLAG
        case WOOF_MSG_GET_DONE:
            WooFProcessGetDone(msg, receiver);
            break;
#endif
#ifdef REPAIR
        case WOOF_MSG_REPAIR:
            WooFProcessRepair(msg, receiver);
            break;
        case WOOF_MSG_REPAIR_PROGRESS:
            WooFProcessRepairProgress(msg, receiver);
            break;
        case LOG_GET_REMOTE_SIZE:
            LogProcessGetSize(msg, receiver);
            break;
        case LOG_GET_REMOTE:
            LogProcessGet(msg, receiver);
            break;
#endif
        default:
            DEBUG_WARN("WooFMsgThread: unknown tag %d\n", int(tag));
            printf("WooFMsgThread: unknown tag %d\n", int(tag));
            break;
        }

	fflush(stdout);
        /*
         * wait for next request
         */
        msg = Receive(*receiver);
	if(!msg) {
		// if something went wrong, next recive will fail
//		zsock_destroy(receiver);
printf("zmq: server failed to receive msg, creating new receiver\n");
    		receiver = ZServerPtr(zsock_new_rep(">inproc://workers"));
        	msg = Receive(*receiver);
                Msg_id++;
	} else {
		Msg_id++;
	}
    }
    printf("zmq msg server thread is exiting\n");
#undef DEBUG
    return; // will cause thread to exit
}
} // namespace

bool backend::listen(std::string_view ns) {
    m_stop_called = false;

    std::string woof_namespace(ns);

    DEBUG_FATAL_IF(woof_namespace.empty(), "WooFMsgServer: couldn't find namespace");

    DEBUG_LOG("WooFMsgServer: started for namespace %s\n", woof_namespace.c_str());

    /*
     * set up the front end router socket
     */
    auto port = WooFPortHash(woof_namespace.c_str());

printf("zmq configured for namespace %s on port %d\n",woof_namespace.c_str(), port);
fflush(stdout);

    /*
     * listen to any tcp address on port hash of namespace
     */
    auto endpoint = fmt::format("tcp://*:{}", port);

    DEBUG_LOG("WooFMsgServer: frontend at %s\n", endpoint.c_str());

    /*
     * create zproxy actor
     */
    m_proxy = ZActorPtr(zactor_new(zproxy, nullptr));
    if (!m_proxy) {
        perror("WooFMsgServer: couldn't create proxy");
        return false;
    }

    /*
     * create and bind endpoint with port has to frontend zsock
     */
    zstr_sendx(m_proxy.get(), "FRONTEND", "ROUTER", endpoint.c_str(), NULL);
    zsock_wait(m_proxy.get());

    /*
     * inproc:// is a process internal enpoint for ZeroMQ
     *
     * if backend isn't in this process, this endpoint will need to be
     * some kind of IPC endpoit.  For now, assume it is within the process
     * and handled by threads
     */
    zstr_sendx(m_proxy.get(), "BACKEND", "DEALER", "inproc://workers", NULL);
    zsock_wait(m_proxy.get());

    /*
     * create a single thread for now.  The DEALER pattern can handle multiple threads,
     * however so this can be increased if need be
    for (auto& t : m_threads) {
     */
    for (auto& t : m_threads) {
        t = std::thread(WooFMsgThread);
    }

    return true;
}

bool backend::stop() {
    m_stop_called = true;
    /*
     * right now, there is no way for these threads to exit so the msg server will block
     * indefinitely in this join
     */
    for (auto& t : m_threads) {
        t.join();
    }

    m_proxy.reset();
    return true;
}

#if 0
bool backend::listen(std::string_view ns) {
    m_stop_called = false;

    std::string woof_namespace(ns);
    DEBUG_FATAL_IF(woof_namespace.empty(), "WooFMsgServer: couldn't find namespace");

    auto port = WooFPortHash(woof_namespace.c_str());
    auto endpoint = fmt::format("tcp://*:{}", port);

    printf("zmq configured for namespace %s on port %d\n",
           woof_namespace.c_str(), port);
    fflush(stdout);

    DEBUG_LOG("WooFMsgServer: frontend at %s\n", endpoint.c_str());

    m_zmq_ctx = zmq_ctx_new();
    if (!m_zmq_ctx) {
        perror("zmq_ctx_new");
        return false;
    }

    m_frontend = zmq_socket(m_zmq_ctx, ZMQ_ROUTER);
    if (!m_frontend) {
        perror("zmq_socket ROUTER");
        zmq_ctx_term(m_zmq_ctx);
        m_zmq_ctx = nullptr;
        return false;
    }

    m_backend = zmq_socket(m_zmq_ctx, ZMQ_DEALER);
    if (!m_backend) {
        perror("zmq_socket DEALER");
        zmq_close(m_frontend);
        zmq_ctx_term(m_zmq_ctx);
        m_frontend = nullptr;
        m_zmq_ctx = nullptr;
        return false;
    }

    if (zmq_bind(m_frontend, endpoint.c_str()) != 0) {
        perror("zmq_bind frontend");
        zmq_close(m_backend);
        zmq_close(m_frontend);
        zmq_ctx_term(m_zmq_ctx);
        m_backend = nullptr;
        m_frontend = nullptr;
        m_zmq_ctx = nullptr;
        return false;
    }

    if (zmq_bind(m_backend, "inproc://workers") != 0) {
        perror("zmq_bind backend");
        zmq_close(m_backend);
        zmq_close(m_frontend);
        zmq_ctx_term(m_zmq_ctx);
        m_backend = nullptr;
        m_frontend = nullptr;
        m_zmq_ctx = nullptr;
        return false;
    }

    m_proxy_thread = std::thread([this]() {
        int rc = zmq_proxy(m_frontend, m_backend, nullptr);

        if (rc != 0 && !m_stop_called) {
            perror("zmq_proxy");
        }
    });

    for (auto& t : m_threads) {
        t = std::thread(WooFMsgThread);
    }

    return true;
}
#endif
} // namespace cspot::zmq
