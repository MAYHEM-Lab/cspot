#include "backend.hpp"
#include "common.hpp"
#include "debug.h"

#include <woofc-access.h>
extern "C" {
#include <cmq-pkt.h>
}

namespace cspot::cmq {
namespace {
void WooFMsgThread(int sd) {
	int c_fd;
	int err;
	unsigned char *fl;
	unsigned char *f;
	unsigned char *r_fl;
	unsigned char *r_f;
	

    	DEBUG_LOG("WooFMsgThread: about to call accept");

	sd = cmq_pkt_accept(sd,WOOF_MSG_REQ_TIMEOUT);
	if(sd < 0) {
		DEBUG_WARN("WooFMsgThread: accept failed");
		perror("WooFMsgThread: accept failed");
		return;
	}

	err = cmq_pkt_recv_msg(sd,&fl);
	if(err < 0) {
		DEBUG_WARN("WooFMsgThread: recv failed");
                perror("WooFMsgThread: recv failed");
                return;
        }

    	while (err >= 0) {

        	DEBUG_LOG("WooFMsgThread: received");
		err = cmq_frame_pop(fl,&f);
		if(err < 0) {
			cmq_frame_list_destroy(fl);
        		DEBUG_WARN("WooFMsgThread: couldn't get tag");
			close(sd);
			close(c_fd);
			return;
		}

        /*
         * WooFMsg starts with a message tag for dispatch
         */
		tag = strtol(cmq_frame_payload(f),NULL,10);
        	DEBUG_LOG("WooFMsgThread: processing msg with tag: %lu\n", tag);
		cmq_frame_destroy(f);

		switch (tag) {
#if 0
		case WOOF_MSG_PUT:
		    WooFProcessPut(std::move(msg), receiver.get());
		    break;
#endif
		case WOOF_MSG_GET:
		    WooFProcessGet(fl,sd);
		    break;
#if 0
		case WOOF_MSG_GET_EL_SIZE:
		    WooFProcessGetElSize(std::move(msg), receiver.get());
		    break;
#endif
		case WOOF_MSG_GET_TAIL:
		    WooFProcessGetTail(fl,sd);
		    break;
#if 0
		case WOOF_MSG_GET_LATEST_SEQNO:
		    WooFProcessGetLatestSeqno(std::move(msg), receiver.get());
		    break;
#endif
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
		    break;
		}
		err = cmq_pkt_recv_msg(sd,&fl);
    	}
    	close(sd);
    	return;
}
} // namespace

bool backend::listen(std::string_view ns) {
    m_stop_called = false;
    int sd;

    std::string woof_namespace(ns);

    DEBUG_FATAL_IF(woof_namespace.empty(), "WooFMsgServer: couldn't find namespace");

    DEBUG_LOG("WooFMsgServer: started for namespace %s\n", woof_namespace.c_str());

    /*
     * set up the front end router socket
     */
    auto port = WooFPortHash(woof_namespace.c_str());

    sd = cmq_pkt_listen(port);
    
    /*
     * create a single thread for now.  multiple threads can call accept
     */
    for (auto& t : m_threads) {
        t = std::thread(WooFMsgThread, sd);
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
} // namespace cspot::zmq
