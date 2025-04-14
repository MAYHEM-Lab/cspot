#pragma once

#include <optional>
#include <string>

namespace cspot::cmq {
void WooFProcessPut(unsigned char *fl, int sd, int no_cap);
void WooFProcessPutwithCAP(unsigned char *fl, int sd);

void WooFProcessGetElSize(unsigned char *fl, int sd, int no_cap);
void WooFProcessGetElSizewithCAP(unsigned char *fl, int sd);

void WooFProcessGetLatestSeqno(unsigned char *fl, int sd, int no_cap);
void WooFProcessGetLatestSeqnowithCAP(unsigned char *fl, int sd);

void WooFProcessGetTail(unsigned char *fl, int sd);

void WooFProcessGet(unsigned char *fl, int sd, int no_cap);
void WooFProcessGetwithCAP(unsigned char *fl, int sd);

std::optional<std::string> ip_from_woof(std::string_view woof_name);
std::optional<std::string> port_from_woof(std::string_view woof_name);
} // namespace cspot::cmq
