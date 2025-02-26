#include "backend.hpp"
#include "common.hpp"
#include "debug.h"
#include "net.h"
#include "zmq_wrap.hpp"

#include <czmq.h>
#include <fmt/format.h>
#include <global.h>
#include <thread>
#include <unordered_map>
#include <woofc-access.h>
#include <woofc-priv.h>
#include <woofc.h>

namespace cspot::zmq {
void WooFProcessGetElSize(ZMsgPtr req_msg, zsock_t* resp_sock) {
    auto res = ExtractMessage<std::string>(*req_msg);

    if (!res) {
        DEBUG_WARN("WooFProcessGetElSize Bad message");
        return;
    }

    auto& [woof_name] = *res;

    char local_name[1024] = {};
    auto err = WooFLocalName(woof_name.c_str(), local_name, sizeof(local_name));

    WOOF* wf;
    if (err < 0) {
        wf = WooFOpen(local_name);
    } else {
        wf = WooFOpen(woof_name.c_str());
    }

    unsigned long el_size = -1;
    if (!wf) {
        DEBUG_LOG("WooFProcessGetElSize: couldn't open %s (%s)\n", local_name, woof_name.c_str());
    } else {
        el_size = wf->shared->element_size;
        WooFDrop(wf);
    }

    auto resp = CreateMessage(std::to_string(el_size));
    if (!res) {
        DEBUG_WARN("WooFProcessGetElSize: Could not allocate message");
        return;
    }

    if (!Send(std::move(resp), *resp_sock)) {
        DEBUG_WARN("WooFProcessGetElSize: Could not send response");
        return;
    }
}

void WooFProcessPut(ZMsgPtr req_msg, zsock_t* resp_sock) {
    auto res = ExtractMessage<std::string, std::string, /*std::string, std::string,*/ std::vector<uint8_t>>(*req_msg);

    if (!res) {
        DEBUG_WARN("WooFProcessPut Bad message");
        return;
    }

    auto& [woof_name, hand_name, /*name_id_str, log_seq_no_str,*/ elem] = *res;
    // auto cause_host = std::stoul(name_id_str);
    // auto cause_seq_no = std::stoul(log_seq_no_str);
    auto cause_host = 0;
    auto cause_seq_no = 0;

    char local_name[1024] = {};
    auto err = WooFLocalName(woof_name.c_str(), local_name, sizeof(local_name));
    if (err < 0) {
        DEBUG_WARN("WooFProcessPut local name failed");
        return;
    }

    unsigned long seq_no = WooFPutWithCause(
        local_name, hand_name.empty() ? nullptr : hand_name.c_str(), elem.data(), cause_host, cause_seq_no);

    auto resp = CreateMessage(std::to_string(seq_no));
    if (!res) {
        DEBUG_WARN("WooFProcessPut: Could not allocate message");
        return;
    }

    if (!Send(std::move(resp), *resp_sock)) {
        DEBUG_WARN("WooFProcessPut: Could not send response");
        return;
    }
}

void WooFProcessGet(ZMsgPtr req_msg, zsock_t* resp_sock) {
    auto res = ExtractMessage<std::string, std::string/*, std::string, std::string*/>(*req_msg);

    if (!res) {
        DEBUG_WARN("WooFProcessGet Bad message");
        return;
    }

    auto& [woof_name, seq_no_str/*, name_id_str, log_seq_no_str*/] = *res;
    auto seq_no = std::stoul(seq_no_str);
    // auto cause_host = std::stoul(name_id_str);
    // auto cause_seq_no = std::stoul(log_seq_no_str);
    auto cause_host = 0;
    auto cause_seq_no = 0;

    char local_name[1024] = {};
    auto err = WooFLocalName(woof_name.c_str(), local_name, sizeof(local_name));
    /*
     * attempt to get the element from the local woof_name
     */
    WOOF* wf;
    std::vector<uint8_t> elem;
    if (err < 0) {
        wf = WooFOpen(woof_name.c_str());
    } else {
        wf = WooFOpen(local_name);
    }

    if (!wf) {
        DEBUG_WARN("WooFProcessGet: couldn't open woof: %s\n", woof_name.c_str());
    } else {
        elem = std::vector<uint8_t>(wf->shared->element_size);
        err = WooFReadWithCause(wf, elem.data(), seq_no, cause_host, cause_seq_no);
        if (err < 0) {
            DEBUG_WARN("WooFProcessGet: read failed: %s at %lu\n", woof_name.c_str(), seq_no);
            elem = {};
        }
        WooFDrop(wf);
    }

    auto resp = CreateMessage(elem);
    if (!res) {
        DEBUG_WARN("WooFProcessGet: Could not allocate message");
        return;
    }

    if (!Send(std::move(resp), *resp_sock)) {
        DEBUG_WARN("WooFProcessGet: Could not send response");
        return;
    }
}

void WooFProcessGetLatestSeqno(ZMsgPtr req_msg, zsock_t* resp_sock) {
    auto res = ExtractMessage<std::string/*, std::string, std::string, std::string, std::string*/>(*req_msg);

    if (!res) {
        DEBUG_WARN("WooFProcessGet Bad message");
        return;
    }

    auto& [woof_name/*, name_id_str, log_seq_no_str, cause_woof, cause_woof_latest_seq_no_str*/] = *res;
    // auto cause_host = std::stoul(name_id_str);
    // auto cause_seq_no = std::stoul(log_seq_no_str);
    // auto cause_woof_latest_seq_no = std::stoul(cause_woof_latest_seq_no_str);
    // auto cause_host = 0;
    // auto cause_seq_no = 0;
    // auto cause_woof_latest_seq_no = 0;
    // std::string cause_woof = "";

    char local_name[1024] = {};
    auto err = WooFLocalName(woof_name.c_str(), local_name, sizeof(local_name));

    WOOF* wf;
    if (err < 0) {
        wf = WooFOpen(woof_name.c_str());
    } else {
        wf = WooFOpen(local_name);
    }

    unsigned long latest_seq_no = -1;
    if (!wf) {
        DEBUG_WARN("WooFProcessGet: couldn't open woof: %s\n", woof_name.c_str());
    } else {
        latest_seq_no = WooFLatestSeqno(wf);
//            WooFLatestSeqnoWithCause(wf, cause_host, cause_seq_no, cause_woof.c_str(), cause_woof_latest_seq_no);
        WooFDrop(wf);
    }

    auto resp = CreateMessage(std::to_string(latest_seq_no));
    if (!res) {
        DEBUG_WARN("WooFProcessGet: Could not allocate message");
        return;
    }

    if (!Send(std::move(resp), *resp_sock)) {
        DEBUG_WARN("WooFProcessGet: Could not send response");
        return;
    }
}

void WooFProcessGetTail(ZMsgPtr req_msg, zsock_t* resp_sock) {
    auto res = ExtractMessage<std::string, std::string>(*req_msg);

    if (!res) {
        DEBUG_WARN("WooFProcessGet Bad message");
        return;
    }

    auto& [woof_name, el_count_str] = *res;
    auto el_count = std::stoul(el_count_str);

    char local_name[1024] = {};
    auto err = WooFLocalName(woof_name.c_str(), local_name, sizeof(local_name));

    WOOF* wf;
    if (err < 0) {
        wf = WooFOpen(woof_name.c_str());
    } else {
        wf = WooFOpen(local_name);
    }

    uint32_t el_read = -1;
    std::vector<uint8_t> elements;
    if (!wf) {
        DEBUG_WARN("WooFProcessGet: couldn't open woof: %s\n", woof_name.c_str());
    } else {
        auto el_size = wf->shared->element_size;
        elements = std::vector<uint8_t>(el_size * el_count);
        el_read = WooFReadTail(wf, elements.data(), el_count);

        WooFDrop(wf);
    }

    auto resp = CreateMessage(std::to_string(el_read), elements);
    if (!res) {
        DEBUG_WARN("WooFProcessGet: Could not allocate message");
        return;
    }

    if (!Send(std::move(resp), *resp_sock)) {
        DEBUG_WARN("WooFProcessGet: Could not send response");
        return;
    }
}
} // namespace cspot::zmq
