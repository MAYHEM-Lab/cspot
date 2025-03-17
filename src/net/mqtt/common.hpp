#pragma once

#include "zmq_wrap.hpp"

#include <optional>
#include <string>

namespace cspot::mqtt {
extern int WooFGatewayIP(char *ip_str, int ip_size);

nt WooFGetMQTT(const char *wf_name, const void* element, const unsigned long seqno, int no_cap);
} // namespace cspot::zmq
