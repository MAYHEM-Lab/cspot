#include "backend.hpp"
#include "common.hpp"
#include "debug.h"
#include "net.h"
#include "cmq_wrap.hpp"

#include <global.h>
#include <thread>
#include <unordered_map>
#include <woofc-access.h>
#include <woofc-priv.h>
#include <woofc.h>

extern "C" {
#include <cmq-pkt.h>
}

namespace cspot::cmq {
#if 0
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
#endif

void WooFProcessGet(unsigned char *fl, int sd) 
{
	unsigned char *woof_frame;
	unsigned char *seqno_frame;
	int err;
	char *woof_name;
	unsigned long seq_no;
	unsigned char *elem;
	unsigned char *r_fl;
	unsigned char *r_frame;

	if(cmq_frame_list_empty(fl)) {
        	DEBUG_WARN("WooFProcessGet Bad message");
        	return;
    	}

	// the server thread has stripped the cmd
	// first remaining frame is woof_name
	// second remaining frame is seq_no
	err = cmq_frame_pop(fl,&woof_frame);
	if(err < 0) {
		DEBUG_WARN("WooFProcessGet: no woof name in msg\n");
		cmq_frame_list_destroy(fl);
		return;
	}

	err = cmq_frame_pop(fl,&seqno_frame);
	if(err < 0) {
		DEBUG_WARN("WooFProcessGet: no seqno in msg\n");
		cmq_frame_destroy(woof_frame);
		cmq_frame_list_destroy(fl);
		return;
	}
	cmq_frame_list_destroy(fl);
    	seq_no = stoul(cmq_frame_payload(seqno_frame),NULL,10);
    	cmq_frame_destroy(seqno_frame);
    // auto cause_host = std::stoul(name_id_str);
    // auto cause_seq_no = std::stoul(log_seq_no_str);
    	auto cause_host = 0;
    	auto cause_seq_no = 0;

    	char local_name[1024] = {};
    	auto err = WooFLocalName(cmq_frame_payload(woof_frame), local_name, sizeof(local_name));
    /*
     * attempt to get the element from the local woof_name
     */
    	WOOF* wf;
    	if (err < 0) {
        	wf = WooFOpen(cmq_frame_payload(woof_frame));
    	} else {
        	wf = WooFOpen(local_name);
    	}

    	if (!wf) {
        	DEBUG_WARN("WooFProcessGet: couldn't open woof: %s\n", cmq_frame_payload(woof_frame));
    	} else {
		elem = (unsigned char *)malloc(wf->shared->element_size);
	}

	if(elem != NULL) {
		err = WooFReadWithCause(wf, elem, seq_no, cause_host, cause_seq_no);
		if (err < 0) {
		    DEBUG_WARN("WooFProcessGet: read failed: %s at %lu\n", cmq_frame_payload(woof_name), seq_no);
		    free(elem);
		    cmq_frame_destroy(woof_name);
		    elem = NULL;
		}
	}
	 cmq_frame_destroy(woof_name);

	err = cmq_frame_list_create(&r_fl);
	if(err < 0) {
        	DEBUG_WARN("WooFProcessGet: Could not allocate message");
		free(elem);
        	return;
	}
	err = cmq_frame_create(&r_frame,elem,wf->shared->element_size);
	WooFDrop(wf);
	free(elem);
	if(err < 0) {
		cmq_frame_list_destroy(r_fl);
        	DEBUG_WARN("WooFProcessGet: Could not allocate frame");
		return;
	}

	err = cmq_frame_append(r_fl,r_frame);
	if(err < 0) {
		cmq_frame_list_destroy(r_fl);
		cmq_frame_destroy(r_frame);
        	DEBUG_WARN("WooFProcessGet: Could not append frame");
		return;
	}

	err = cmq_pkt_send_msg(sd,r_fl);
	if(err < 0) {
        	DEBUG_WARN("WooFProcessGet: Could not send response");
	}
	cmq_frame_list_destroy(r_fl);
        return;
}

#if 0
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
    auto cause_host = 0;
    auto cause_seq_no = 0;
    auto cause_woof_latest_seq_no = 0;
    std::string cause_woof = "";

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
        latest_seq_no =
            WooFLatestSeqnoWithCause(wf, cause_host, cause_seq_no, cause_woof.c_str(), cause_woof_latest_seq_no);
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
#endif
} // namespace cspot::zmq
