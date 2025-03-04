#include <iostream>
#include <string>
#include <limits>
#include <optional>

#include "backend.hpp"
#include "common.hpp"
#include "debug.h"

#include <fmt/format.h>
#include <woofc-access.h>

namespace cspot::zmq {
namespace {
int safe_stoul_to_int(const std::string& str) {
    try {
        size_t pos;
        unsigned long value = std::stoul(str, &pos);

        // Ensure no extra characters exist
        if (pos != str.size()) {
            throw std::invalid_argument("Invalid characters in input");
        }

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

void WooFMsgThread() {
	int ltag;
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
    while (msg) {
        DEBUG_LOG("WooFMsgThread: received");

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
		return;
	}
        DEBUG_LOG("WooFMsgThread: processing msg with tag: %lu\n", tag);

        switch (tag) {
        case WOOF_MSG_PUT:
            WooFProcessPut(std::move(msg), receiver.get());
            break;
        case WOOF_MSG_GET:
            WooFProcessGet(std::move(msg), receiver.get());
            break;
        case WOOF_MSG_GET_EL_SIZE:
            WooFProcessGetElSize(std::move(msg), receiver.get());
            break;
        case WOOF_MSG_GET_TAIL:
            WooFProcessGetTail(std::move(msg), receiver.get());
            break;
        case WOOF_MSG_GET_LATEST_SEQNO:
            WooFProcessGetLatestSeqno(std::move(msg), receiver.get());
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
            break;
        }

        /*
         * wait for next request
         */
        msg = Receive(*receiver);
    }
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
} // namespace cspot::zmq
