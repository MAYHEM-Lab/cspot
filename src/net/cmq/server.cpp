#include "backend.hpp"
#include "common.hpp"
#include "debug.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <woofc-access.h>
#include "cmq-frame.h"
#include "cmq-pkt.h"
#include <pthread.h>

extern void cmq_mqtt_shutdown();

namespace cspot::cmq {
namespace {

void *WooFMsgThread(void *arg) {
	int sd;
	int c_sd;
	int err;
	unsigned char *fl;
	unsigned char *f;

	sd = *((int *)arg);
	free(arg);

	

	while(1) { // loop until something fails
		DEBUG_LOG("WooFMsgThread: cmq about to call accept");

		c_sd = cmq_pkt_accept(sd,WOOF_MSG_REQ_TIMEOUT); // timeout is set for c_sd
		if(c_sd < 0) {
			DEBUG_WARN("WooFMsgThread: accept failed");
			perror("WooFMsgThread: accept failed");
			return(NULL);
		}

		err = cmq_pkt_recv_msg(c_sd,&fl);
		if(err < 0) {
			DEBUG_WARN("WooFMsgThread: recv failed");
			perror("WooFMsgThread: recv failed");
			return(NULL);
		}

		while (err >= 0) {

			DEBUG_LOG("WooFMsgThread: received");
			err = cmq_frame_pop(fl,&f);
			if(err < 0) {
				cmq_frame_list_destroy(fl);
				DEBUG_WARN("WooFMsgThread: couldn't get tag");
				cmq_pkt_close(c_sd);
				return(NULL);
			}

		/*
		 * WooFMsg starts with a message tag for dispatch
		 */
			long tag = strtol((char *)cmq_frame_payload(f),NULL,10);
			DEBUG_LOG("WooFMsgThread: processing msg with tag: %lu\n", tag);
			cmq_frame_destroy(f);

			// process routines destroy fl
			switch (tag) {
				case WOOF_MSG_PUT:
				    WooFProcessPut(fl,c_sd);
				    break;
				case WOOF_MSG_GET:
				    WooFProcessGet(fl,c_sd);
				    break;
				case WOOF_MSG_GET_EL_SIZE:
				    WooFProcessGetElSize(fl,c_sd,1);
				    break;
				case WOOF_MSG_GET_EL_SIZE_CAP:
				    WooFProcessGetElSizewithCAP(fl,c_sd);
				    break;
				case WOOF_MSG_GET_TAIL:
				    WooFProcessGetTail(fl,c_sd);
				    break;
				case WOOF_MSG_GET_LATEST_SEQNO:
				    WooFProcessGetLatestSeqno(fl,c_sd);
				    break;
	// these need to be  converte from zmq
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
			err = cmq_pkt_recv_msg(c_sd,&fl);
		}
		cmq_pkt_close(c_sd);
	}
    	return(NULL);
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

    listen_sd = cmq_pkt_listen(port);
    if(listen_sd < 0) {
	DEBUG_WARN("WooFMsgServer: could not create listen socket on %d\n",
			(int)port);
	return(false);
    }

printf("cmq: listen: %s, port: %d sd: %d\n",woof_namespace.c_str(),port,listen_sd);
fflush(stdout);
    
    /*
     * create a single thread for now.  multiple threads can call accept
     */
    /*
    for (auto& t : m_threads) {
        t = std::thread(WooFMsgThread, listen_sd);
    }
    */
    int t;
    for(t=0; t < WOOF_MSG_THREADS; t++) {
	    int *sp = (int *)malloc(sizeof(int));
	    if(sp == NULL) {
		   return false;
	    } 
	    *sp = listen_sd;
	    int perr;
	    perr = pthread_create(&tids[t],NULL,WooFMsgThread,(void *)sp);
	    if(perr != 0) {
		DEBUG_WARN("WooFMsgThread: could not create WooFMsgThread %d\n",t);
		return false;
	    }
    }

    return true;
}

bool backend::stop() {
printf("cmq::backend stop called\n");
    m_stop_called = true;
    /*
     * right now, there is no way for these threads to exit so the msg server will block
     * indefinitely in this join
     */
    /*
    for (auto& t : m_threads) {
        t.join();
    }
    */

    int t;
    for(t=0; t < WOOF_MSG_THREADS; t++) {
	    pthread_join(tids[t],NULL);
    }

    //m_proxy.reset();
    cmq_pkt_shutdown();
    return true;
}
} // namespace cspot::cmq
