#pragma once

#include "zmq_wrap.hpp"

#include <czmq.h>
#include <optional>
#include <string>

namespace cspot::zmq {
void WooFProcessPut(ZMsgPtr req_msg, zsock_t* receiver);

void WooFProcessGetElSize(ZMsgPtr req_msg, zsock_t* receiver);

void WooFProcessGetLatestSeqno(ZMsgPtr req_msg, zsock_t* receiver);

void WooFProcessGetTail(ZMsgPtr req_msg, zsock_t* receiver);

void WooFProcessGet(ZMsgPtr req_msg, zsock_t* receiver);

std::optional<std::string> endpoint_from_woof(std::string_view woof_name);
} // namespace cspot::zmq