#pragma once

#include <optional>
#include <string>

namespace cspot::cmq {
void WooFProcessPut(int receiver, unsigned char *req_msg);

void WooFProcessGetElSize(int receiver, unsigned char *req_msg);

void WooFProcessGetLatestSeqno(int receiver, unsigned char *req_msg);

void WooFProcessGetTail(int receiver, unsigned char *req_msg);

void WooFProcessGet(int receiver, unsigned char *req_msg);

std::optional<std::string> ip_from_woof(std::string_view woof_name);
std::optional<std::string> port_from_woof(std::string_view woof_name);
} // namespace cspot::cmq
