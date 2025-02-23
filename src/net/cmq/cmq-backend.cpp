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
#include <string.h>

extern "C" {
#include <cmq-pkt.h>
}

namespace cspot::cmq {
void WooFProcessGetElSize(unsigned char *fl, int sd) {
	int err;
	unsigned char *woof_name;
	unsigned char *r_fl;
	unsigned char *r_frame;
	const char *s_str;

	if(cmq_frame_list_empty(fl)) {
        	DEBUG_WARN("WooFProcessGetElSize Bad message");
        	return;
	}
	// tag  has been stripped
	err = cmq_frame_pop(fl,&woof_name);
	if(err < 0) {
		cmq_frame_list_destroy(fl);
        	DEBUG_WARN("WooFProcessGetElSize could not get woof name");
        	return;
	}

	char local_name[1024] = {};
	err = WooFLocalName((char *)cmq_frame_payload(woof_name), local_name, sizeof(local_name));

	WOOF* wf;
	if (err < 0) {
		wf = WooFOpen(local_name);
	} else {
		wf = WooFOpen((char *)cmq_frame_payload(woof_name));
	}

    	unsigned long el_size = -1;
	if (!wf) {
		DEBUG_LOG("WooFProcessGetElSize: couldn't open %s (%s)\n", local_name, (char *)cmq_frame_payload(woof_name));
	} else {
		el_size = wf->shared->element_size;
		WooFDrop(wf);
	}
	cmq_frame_destroy(woof_name);

	err = cmq_frame_list_create(&r_fl);
	if(err < 0) {
        	DEBUG_WARN("WooFProcessGetElSize could not create resp message");
        	return;
	}

	s_str = std::to_string(el_size).c_str();
	err = cmq_frame_create(&r_frame,(unsigned char *)s_str,strlen(s_str)+1);
	if(err < 0) {
        	DEBUG_WARN("WooFProcessGetElSize could not create resp frame");
		cmq_frame_list_destroy(r_fl);
        	return;
	}
	err = cmq_frame_append(r_fl,r_frame);
	if(err < 0) {
        	DEBUG_WARN("WooFProcessGetElSize could not append resp frame");
		cmq_frame_list_destroy(r_fl);
		cmq_frame_destroy(r_frame);
        	return;
	}

	err = cmq_pkt_send_msg(sd,r_fl);
	if(err < 0) {
		DEBUG_WARN("WooFProcessGetElSize: Could not send response");
	}
	cmq_frame_list_destroy(r_fl);
	return;
}

void WooFProcessPut(unsigned char *fl, int sd) {
	int err;
	unsigned char *r_fl;
	unsigned char *r_frame;
	unsigned char *woof_name;
	unsigned char *hand_name;
	unsigned char *elem;
	const char *s_str;

	if(cmq_frame_list_empty(fl)) {
		DEBUG_WARN("WooFProcessPut Bad message");
		return;
	}
	// pop woof name
	err = cmq_frame_pop(fl,&woof_name);
	if(err < 0) {
		DEBUG_WARN("WooFProcessPut could not pop woof name");
		cmq_frame_list_destroy(fl);
		return;
	}
	// pop handler name
	// "NULL" is place holder
	err = cmq_frame_pop(fl,&hand_name);
	if(err < 0) {
		DEBUG_WARN("WooFProcessPut could not pop handler name");
		cmq_frame_list_destroy(fl);
		cmq_frame_destroy(woof_name);
		return;
	}
	if(strncmp((char *)cmq_frame_payload(hand_name),"NULL",strlen("NULL")) == 0) {
		hand_name = NULL;
		cmq_frame_destroy(hand_name);
	}
	// pop elemnt frame
	err = cmq_frame_pop(fl,&elem);
	if(err < 0) {
		DEBUG_WARN("WooFProcessPut could not pop element");
		cmq_frame_list_destroy(fl);
		cmq_frame_destroy(woof_name);
		if(hand_name != NULL) {
			cmq_frame_destroy(hand_name);
		}
		return;
	}
	cmq_frame_list_destroy(fl);

	auto cause_host = 0;
	auto cause_seq_no = 0;
		
	char local_name[1024] = {};
	err = WooFLocalName((char *)cmq_frame_payload(woof_name), local_name, sizeof(local_name));
	if (err < 0) {
		DEBUG_WARN("WooFProcessPut local name failed");
		cmq_frame_destroy(woof_name);
		if(hand_name != NULL) {
			cmq_frame_destroy(hand_name);
		}
		return;
	}
	cmq_frame_destroy(woof_name);

	unsigned long seq_no = WooFPutWithCause(
        	local_name, (hand_name == NULL) ? nullptr : (char *)cmq_frame_payload(hand_name), 
		cmq_frame_payload(elem), cause_host, cause_seq_no);
	cmq_frame_destroy(elem);
	if(hand_name != NULL) {
		cmq_frame_destroy(hand_name);
	}

	cmq_frame_destroy(elem);
	if(hand_name != NULL) {
		cmq_frame_destroy(hand_name);
	}

	err = cmq_frame_list_create(&r_fl);
	if(err < 0) {
        	DEBUG_WARN("WooFProcessPut: Could not allocate message");
        	return;
	}

	s_str = std::to_string(seq_no).c_str();
	err = cmq_frame_create(&r_frame,(unsigned char *)s_str,strlen(s_str)+1);
	if(err < 0) {
        	DEBUG_WARN("WooFProcessPut: Could not allocate response frame");
		cmq_frame_list_destroy(r_fl);
        	return;
	}
	err = cmq_frame_append(r_fl,r_frame);
	if(err < 0) {
        	DEBUG_WARN("WooFProcessPut: Could not append response frame");
		cmq_frame_list_destroy(r_fl);
        	return;
	}

	err = cmq_pkt_send_msg(sd,r_fl);
	if(err < 0) {
		DEBUG_WARN("WooFProcessPut: Could not send response");
	}
	cmq_frame_list_destroy(r_fl);
	return;
}

void WooFProcessGet(unsigned char *fl, int sd) 
{
	int err;
	unsigned char *woof_name;
	unsigned char *seqno_frame;
	unsigned long seq_no;
	unsigned char *r_fl;
	unsigned char *r_frame;


	if(cmq_frame_list_empty(fl)) {
        	DEBUG_WARN("WooFProcessGet Bad message");
        	return;
    	}

	// the server thread has stripped the cmd
	// first remaining frame is woof_name
	// second remaining frame is seq_no
	err = cmq_frame_pop(fl,&woof_name);
	if(err < 0) {
		DEBUG_WARN("WooFProcessGet: no woof name in msg\n");
		cmq_frame_list_destroy(fl);
		return;
	}

	err = cmq_frame_pop(fl,&seqno_frame);
	if(err < 0) {
		DEBUG_WARN("WooFProcessGet: no seqno in msg\n");
		cmq_frame_destroy(woof_name);
		cmq_frame_list_destroy(fl);
		return;
	}
	cmq_frame_list_destroy(fl);
    	seq_no = strtoul((char *)cmq_frame_payload(seqno_frame),NULL,10);
    	cmq_frame_destroy(seqno_frame);
    // auto cause_host = std::stoul(name_id_str);
    // auto cause_seq_no = std::stoul(log_seq_no_str);
    	auto cause_host = 0;
    	auto cause_seq_no = 0;

    	char local_name[1024] = {};
    	err = WooFLocalName((char *)cmq_frame_payload(woof_name), local_name, sizeof(local_name));
    /*
     * attempt to get the element from the local woof_name
     */
    	WOOF* wf;
    	if (err < 0) {
        	wf = WooFOpen((char *)cmq_frame_payload(woof_name));
    	} else {
        	wf = WooFOpen(local_name);
    	}


    	if (!wf) {
        	DEBUG_WARN("WooFProcessGet: couldn't open woof: %s\n", (char *)cmq_frame_payload(woof_name));
		// zero frame indicates error
		err = cmq_frame_create(&r_frame,NULL,0); 
    	} else {
		// create empty frame for response
		err = cmq_frame_create(&r_frame,NULL,wf->shared->element_size);
	}
	if(err < 0) {
		DEBUG_WARN("WooFProcessGet: couldn't create response frame woof: %s\n", (char *)cmq_frame_payload(woof_name));
		cmq_frame_destroy(woof_name);
		if(wf) {
			WooFDrop(wf);
		}
		return;
	}

	if(cmq_frame_payload(r_frame) != NULL) {
		err = WooFReadWithCause(wf, cmq_frame_payload(r_frame), seq_no, cause_host, cause_seq_no);
		if (err < 0) {
		    DEBUG_WARN("WooFProcessGet: read failed: %s at %lu\n", (char *)cmq_frame_payload(woof_name), seq_no);
		    cmq_frame_destroy(woof_name);
		    cmq_frame_destroy(r_frame);
		    err = cmq_frame_create(&r_frame,NULL,0); // create zero frame for error
		    if(err < 0) {
				DEBUG_WARN("WooFProcessGet: Could not allocate zero frame for error");
				return;
		    }
		}
	}
	cmq_frame_destroy(woof_name);
	if(wf) {
		WooFDrop(wf);
	}

	err = cmq_frame_list_create(&r_fl);
	if(err < 0) {
		DEBUG_WARN("WooFProcessGet: Could not allocate message");
		cmq_frame_destroy(r_frame);
		return;
	}
	if(err < 0) {
		cmq_frame_list_destroy(r_fl);
		DEBUG_WARN("WooFProcessGet: Could not allocate frame");
		return;
	}

	// r_frame could be zero frame if open or read fails
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

void WooFProcessGetLatestSeqno(unsigned char *fl, int sd) {

	int err;
	unsigned char *woof_name;
	unsigned char *r_fl;
	unsigned char *r_frame;
	const char *s_str;
	
	if(cmq_frame_list_empty(fl)) {
        	DEBUG_WARN("WooFProcessGetLatestSeqno Bad message");
		return;
	}
	// tag has been stripped
	// pop the woof name
	err = cmq_frame_pop(fl,&woof_name);
	if(err < 0) {
        	DEBUG_WARN("WooFProcessGetLatestSeqno could not pop woof_name");
		cmq_frame_list_destroy(fl);
		return;
	}

	cmq_frame_list_destroy(fl);

	auto cause_host = 0;
	auto cause_seq_no = 0;
	auto cause_woof_latest_seq_no = 0;
	std::string cause_woof = "";

	char local_name[1024] = {};
	err = WooFLocalName((char *)cmq_frame_payload(woof_name), local_name, sizeof(local_name));

	WOOF* wf;
	if (err < 0) {
		wf = WooFOpen((char *)cmq_frame_payload(woof_name));
	} else {
		wf = WooFOpen(local_name);
	}

	unsigned long latest_seq_no = -1;
	if (!wf) {
		DEBUG_WARN("WooFProcessGetLatestSeqno: couldn't open woof: %s\n", (char *)cmq_frame_payload(woof_name));
	} else {
		latest_seq_no =
            		WooFLatestSeqnoWithCause(wf, cause_host, cause_seq_no, cause_woof.c_str(), cause_woof_latest_seq_no);
		WooFDrop(wf);
	}
	cmq_frame_destroy(woof_name);

	err = cmq_frame_list_create(&r_fl);
	if(err < 0) {
        	DEBUG_WARN("WooFProcessGetLatestSeqno: Could not allocate message");
        	return;
	}

	s_str = std::to_string(latest_seq_no).c_str();
	err = cmq_frame_create(&r_frame,(unsigned char *)s_str,strlen(s_str)+1);
	if(err < 0) {
        	DEBUG_WARN("WooFProcessGetLatestSeqno: Could not allocate frame");
		cmq_frame_list_destroy(r_fl);
        	return;
	}
	err = cmq_frame_append(r_fl,r_frame);
	if(err < 0) {
        	DEBUG_WARN("WooFProcessGetLatestSeqno: Could not append frame");
		cmq_frame_list_destroy(r_fl);
		cmq_frame_destroy(r_frame);
        	return;
	}
	err = cmq_pkt_send_msg(sd,r_fl);
	if(err < 0) {
        	DEBUG_WARN("WooFProcessGetLatestSeqno: Could not send response");
        	return;
	}
	cmq_frame_list_destroy(r_fl);
	return;

}


void WooFProcessGetTail(unsigned char *fl, int sd) {

	int err;
	unsigned char *woof_name;
	unsigned char *f;
	unsigned int el_count;
	unsigned char *r_fl;
	unsigned char *r_f;
	unsigned char *e_f;
	const char *s_str;


	if (cmq_frame_list_empty(fl)) {
		DEBUG_WARN("WooFProcessGetTail empty message");
		return;
	}

	// first frame is woof_name
	err = cmq_frame_pop(fl,&woof_name);
	if(err < 0) {
		DEBUG_WARN("WooFProcessGetTail could not pop woof_name");
		return;
	}

	// second frame is el_count
	err = cmq_frame_pop(fl,&f);
	if(err < 0) {
		cmq_frame_destroy(woof_name);
		DEBUG_WARN("WooFProcessGetTail could not pop woof_name");
		return;
	}
	el_count = strtoul((char *)cmq_frame_payload(f),NULL,10);
	cmq_frame_destroy(f);

    	char local_name[1024] = {};
    	err = WooFLocalName((char *)cmq_frame_payload(woof_name), local_name, sizeof(local_name));

	WOOF* wf;
	if (err < 0) {
        	wf = WooFOpen((char *)cmq_frame_payload(woof_name));
	} else {
        	wf = WooFOpen(local_name);
	}

	uint32_t el_read = 0;
	uint32_t el_size = 0;

	if (!wf) {
        	DEBUG_WARN("WooFProcessGetTail: couldn't open woof: %s\n", (char *)cmq_frame_payload(woof_name));
	} else {
		el_size = wf->shared->element_size;
		err = cmq_frame_create(&e_f,NULL,el_size * el_count); // create empty frame
		if(err < 0) {
			DEBUG_WARN("WooFProcessGetTail could not get space for elements");
			cmq_frame_destroy(woof_name);
			return;
		}
		el_read = WooFReadTail(wf, cmq_frame_payload(e_f), el_count); // read into empty frame
		if(el_read <= 0) {
			cmq_frame_destroy(e_f);
		}
        	WooFDrop(wf);
	}

	cmq_frame_destroy(woof_name);
	cmq_frame_list_destroy(fl);

	// create response message
	err = cmq_frame_list_create(&r_fl);
	if(err < 0) {
		DEBUG_WARN("WooFProcessGetTail could not allocate response list");
		return;
	}
	s_str = std::to_string(el_read).c_str();
	err = cmq_frame_create(&r_f,(unsigned char *)s_str,strlen(s_str)+1); // send el_read
	if(err < 0) {
		cmq_frame_list_destroy(r_fl);
		DEBUG_WARN("WooFProcessGetTail could not allocate frame for el_read");
		return;
	}
	err = cmq_frame_append(r_fl,r_f);
	if(err < 0) {
		cmq_frame_list_destroy(r_fl);
		cmq_frame_destroy(r_f);
		DEBUG_WARN("WooFProcessGetTail could not append frame for el_read");
		return;
	}
	if(el_read <= 0) {
		err = cmq_frame_create(&e_f,NULL,0); // zero frame for elements
		if(err < 0) {
			cmq_frame_list_destroy(r_fl);
			DEBUG_WARN("WooFProcessGetTail could not allocate frame for elements");
			return;
		}
	}
	err = cmq_frame_append(r_fl,e_f);
	if(err < 0) {
		cmq_frame_list_destroy(r_fl);
		cmq_frame_destroy(e_f);
		DEBUG_WARN("WooFProcessGetTail could not append frame for elements");
		return;
	}
	err = cmq_pkt_send_msg(sd,r_fl);
	if(err < 0) {
		DEBUG_WARN("WooFProcessGetTail could not send response");
		return;
	}
	cmq_frame_list_destroy(r_fl);
	return;
}
} // namespace cspot::cmq
