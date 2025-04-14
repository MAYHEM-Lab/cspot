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
#include <woofc-caplets.h>
#include <string.h>

extern "C" {
#include <cmq-pkt.h>
}

namespace cspot::cmq {
void WooFProcessGetElSize(unsigned char *fl, int sd, int no_cap) {
	int err;
	unsigned char *woof_name;
	unsigned char *r_fl;
	unsigned char *r_frame;
	const char *s_str;

	if(cmq_frame_list_empty(fl)) {
        	DEBUG_WARN("WooFProcessGetElSize Bad message");
		cmq_frame_list_destroy(fl);
        	return;
	}
	// tag  has been stripped
	// first frame is woof name
	err = cmq_frame_pop(fl,&woof_name);
	if(err < 0) {
		cmq_frame_list_destroy(fl);
        	DEBUG_WARN("WooFProcessGetElSize could not get woof name");
        	return;
	}

	// no more valid frames
	cmq_frame_list_destroy(fl);

	char local_name[1024] = {};
	err = WooFLocalName((char *)cmq_frame_payload(woof_name), local_name, sizeof(local_name));

	char cap_name[1028] = {};
	sprintf(cap_name,"%s.CAP",local_name);
	// if we find a CAP and there should not be one, error
	if(no_cap == 1) {
        	WOOF* wfc;
        	wfc = WooFOpen(cap_name);
        	if(wfc) {
            		WooFDrop(wfc);
            		return;
        	}
	}

	WOOF* wf;
	if (err < 0) {
		wf = WooFOpen(local_name);
	} else {
		wf = WooFOpen((char *)cmq_frame_payload(woof_name));
	}

	// -1 is error return
    	unsigned long el_size = -1;
	if (!wf) {
		DEBUG_LOG("WooFProcessGetElSize: couldn't open %s (%s)\n", local_name, (char *)cmq_frame_payload(woof_name));
	} else {
		el_size = wf->shared->element_size;
		WooFDrop(wf);
	}

	// done with woof_name
	cmq_frame_destroy(woof_name);

	// create reply msg
	err = cmq_frame_list_create(&r_fl);
	if(err < 0) {
        	DEBUG_WARN("WooFProcessGetElSize could not create resp message");
        	return;
	}

	// convert el_size to a string
	s_str = std::to_string(el_size).c_str();
	err = cmq_frame_create(&r_frame,(unsigned char *)s_str,strlen(s_str)+1);
	if(err < 0) {
        	DEBUG_WARN("WooFProcessGetElSize could not create resp frame");
		cmq_frame_list_destroy(r_fl);
        	return;
	}

	// add it to the msg
	err = cmq_frame_append(r_fl,r_frame);
	if(err < 0) {
        	DEBUG_WARN("WooFProcessGetElSize could not append resp frame");
		cmq_frame_list_destroy(r_fl);
		cmq_frame_destroy(r_frame);
        	return;
	}

	// send response
	err = cmq_pkt_send_msg(sd,r_fl);
	if(err < 0) {
		DEBUG_WARN("WooFProcessGetElSize: Could not send response");
	}

	// destreoy the message
	cmq_frame_list_destroy(r_fl);
	return;
}


void WooFProcessGetElSizewithCAP(unsigned char *fl, int sd) 
{
	unsigned char *cframe;
	unsigned char *wframe;
	char *wname;
	WCAP *cap;
	WOOF* wf;
	WCAP principal;
	unsigned long seq_no;
	int err;

	if(cmq_frame_list_empty(fl)) {
        	DEBUG_WARN("WooFProcessGetElSizewithCAP Bad message");
		cmq_frame_list_destroy(fl);
        	return;
	}
	//
	// tag  has been stripped
	// first frame is woof name
	err = cmq_frame_pop(fl,&cframe);
	if(err < 0) {
		cmq_frame_list_destroy(fl);
        	DEBUG_WARN("WooFProcessGetElSizewithCAP could not get cap frame");
        	return;
	}

	cap = (WCAP *)cmq_frame_payload(cframe);

	if(cap == NULL) {
		DEBUG_WARN("WooFProcessGetElSizewithCAP: could not get woof cap frame payload\n");
		return;
	}

	wframe = cmq_frame_list_head(fl);
	if(wframe == NULL) {
		DEBUG_WARN("WooFProcessGetElSizewithCAP: could not get woof name frame\n");
		return;
	}

	wname = (char *)cmq_frame_payload(wframe); // remaining frames
	if(wname == NULL) {
		DEBUG_WARN("WooFProcessGetElSizewithCAP: could not get woof name frame payload\n");
		return;
	}

	char local_name[1024] = {};
    	err = WooFLocalName(wname, local_name, sizeof(local_name));
	if (err < 0) {
		DEBUG_WARN("WooFProcessGetElSizewithCAP local name failed\n");
		return;
	}
	char cap_name[1028] = {};
	sprintf(cap_name,"%s.CAP",local_name);

	wf = WooFOpen(cap_name);
	// backwards compatibility: no CAP => authorized
	if(!wf) {
		WooFProcessGetElSize(fl,sd,0);
		return;
	}
	seq_no = WooFLatestSeqno(wf);
	err = WooFReadWithCause(wf,&principal,seq_no,0,0);
	WooFDrop(wf);
	if(err < 0) {
		DEBUG_WARN("WooFProcessGetElSizewithCAP cap get failed\n");
		return;
	}
	
	DEBUG_LOG("WooFProcessGetElSizewithCAP: read CAP woof\n");
	// check read perms
	if(WooFCapAuthorized(principal.check,cap,WCAP_READ)) {
		DEBUG_WARN("WooFProcessGetElSizewithCAP: CAP auth %s\n",cap_name);
		WooFProcessGetElSize(fl,sd,0);
		return;
	} 
	DEBUG_WARN("WooFProcessGetElSizewithCAP: read CAP denied %s\n",cap_name);
	// denied
	return;
}

void WooFProcessPut(unsigned char *fl, int sd, int no_cap) {
	int err;
	unsigned char *r_fl;
	unsigned char *r_frame;
	unsigned char *woof_name;
	unsigned char *hand_name;
	unsigned char *elem;
	const char *s_str;

	if(cmq_frame_list_empty(fl)) {
		DEBUG_WARN("WooFProcessPut Bad message");
		cmq_frame_list_destroy(fl);
		return;
	}
	// tag has been stripped
	// pop woof name frame
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
		// use NULL ptr in this local routine if handler name is NULL
		// FIX: use zero frame instead of NULL
		cmq_frame_destroy(hand_name);
		hand_name = NULL;
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

	// destroy request msg
	cmq_frame_list_destroy(fl);

	auto cause_host = 0;
	auto cause_seq_no = 0;
		
	// do the put
	char local_name[1024] = {};
	err = WooFLocalName((char *)cmq_frame_payload(woof_name), local_name, sizeof(local_name));

	// done with woof name
	cmq_frame_destroy(woof_name);

	char cap_name[1028] = {};
	sprintf(cap_name,"%s.CAP",local_name);
    // if there is a cap there should not be one, error
	if(no_cap == 1) {
		WOOF* wfc;
		wfc = WooFOpen(cap_name);
		if(wfc) {
			WooFDrop(wfc);
			return;
		}
	}

	// do the put using frame payloads as inputs
	unsigned long seq_no = WooFPutWithCause(
        	local_name, (hand_name == NULL) ? nullptr : (char *)cmq_frame_payload(hand_name), 
		cmq_frame_payload(elem), cause_host, cause_seq_no);
	// destroy element frame
	cmq_frame_destroy(elem);
	// destroy hand_name frame if there was one not NULL
	if(hand_name != NULL) {
		cmq_frame_destroy(hand_name);
	}

	// create response msg
	err = cmq_frame_list_create(&r_fl);
	if(err < 0) {
        	DEBUG_WARN("WooFProcessPut: Could not allocate message");
        	return;
	}

	// convert seq_no to string and create a frame with it
	s_str = std::to_string(seq_no).c_str();
	err = cmq_frame_create(&r_frame,(unsigned char *)s_str,strlen(s_str)+1);
	if(err < 0) {
        	DEBUG_WARN("WooFProcessPut: Could not allocate response frame");
		cmq_frame_list_destroy(r_fl);
        	return;
	}
	// add seq_no to response msg
	err = cmq_frame_append(r_fl,r_frame);
	if(err < 0) {
        	DEBUG_WARN("WooFProcessPut: Could not append response frame");
		cmq_frame_list_destroy(r_fl);
		cmq_frame_destroy(r_frame);
        	return;
	}

	// send response msg == timeout set by accept()
	err = cmq_pkt_send_msg(sd,r_fl);
	if(err < 0) {
		DEBUG_WARN("WooFProcessPut: Could not send response");
	}
	// destroy (deep delete) response msg
	cmq_frame_list_destroy(r_fl);
	return;
}

void WooFProcessPutwithCAP(unsigned char *fl, int sd) 
{
	unsigned char *cframe;
	unsigned char *wframe;
	char *wname;
	unsigned char *hframe;
	char *hname;
	WCAP *cap;
	WOOF* wf;
	WCAP principal;
	unsigned long seq_no;
	int err;

	if(cmq_frame_list_empty(fl)) {
        	DEBUG_WARN("WooFProcessPutwithCAP Bad message");
		cmq_frame_list_destroy(fl);
        	return;
	}
	//
	// tag  has been stripped
	// first frame is woof name
	err = cmq_frame_pop(fl,&cframe);
	if(err < 0) {
		cmq_frame_list_destroy(fl);
        	DEBUG_WARN("WooFProcessPutwithCAP could not get cap frame");
        	return;
	}

	cap = (WCAP *)cmq_frame_payload(cframe);

	if(cap == NULL) {
		DEBUG_WARN("WooFProcessPutwithCAP: could not get woof cap frame payload\n");
		return;
	}

	wframe = cmq_frame_list_head(fl);
	if(wframe == NULL) {
		DEBUG_WARN("WooFProcessPutwithCAP: could not get woof name frame\n");
		return;
	}

	wname = (char *)cmq_frame_payload(wframe); // remaining frames
	if(wname == NULL) {
		DEBUG_WARN("WooFProcessPutwithCAP: could not get woof name frame payload\n");
		return;
	}

	hframe = cmq_frame_next(wframe);
	if(hframe == NULL) {
		DEBUG_WARN("WooFProcessPutwithCAP: could not get handler name frame\n");
		return;
	}

	hname = (char *)cmq_frame_payload(hframe);
	if(hname == NULL) {
		DEBUG_WARN("WooFProcessPutwithCAP: could not get handler name string\n");
		return;
	}

	char local_name[1024] = {};
    	err = WooFLocalName(wname, local_name, sizeof(local_name));
	if (err < 0) {
		DEBUG_WARN("WooFProcessPutwithCAP local name failed\n");
		return;
	}
	char cap_name[1028] = {};
	sprintf(cap_name,"%s.CAP",local_name);
	wf = WooFOpen(cap_name);
	// backwards compatibility: no CAP => authorized
	if(!wf) {
		WooFProcessPut(fl,sd,0);
		return;
	}

	if(strcmp(hname,"NULL") == 0) {
		if(WooFCapAuthorized(principal.check,cap,WCAP_WRITE)) {
                        WooFProcessPut(fl,sd,0);
                        DEBUG_WARN("WooFProcessPutwithCAP: no handler auth %s\n",cap_name);
                        return;
                } else {
                        DEBUG_WARN("WooFProcessPutwithCAP: cap auth failed for WCAP_WRITE: check %lu\n",
                                        cap->check);
                }
	} else {
		if(WooFCapAuthorized(principal.check,cap,WCAP_EXEC)) {
                        DEBUG_WARN("WooFProcessPutwithCAP: handler %s auth %s\n",hname,cap_name);
                        WooFProcessPut(fl,sd,0);
                        return;
                } else {
                        DEBUG_WARN("WooFProcessPutwithCAP: cap auth failed for WCAP_EXEC: handler: %s check %lu\n",
                                        hname,
                                        cap->check);
                }
	}
	// denied
	return;
}


void WooFProcessGet(unsigned char *fl, int sd, int no_cap) 
{
	int err;
	unsigned char *woof_name;
	unsigned char *seqno_frame;
	unsigned long seq_no;
	unsigned char *r_fl;
	unsigned char *r_frame;


	if(cmq_frame_list_empty(fl)) {
        	DEBUG_WARN("WooFProcessGet Bad message");
		cmq_frame_list_destroy(fl);
        	return;
    	}

	// the server thread has stripped the tag
	// first remaining frame is woof_name
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
	// destroy request msg
	cmq_frame_list_destroy(fl);
	// convert seq_no from second frame
    	seq_no = strtoul((char *)cmq_frame_payload(seqno_frame),NULL,10);
	// destroy second frame
    	cmq_frame_destroy(seqno_frame);
    	auto cause_host = 0;
    	auto cause_seq_no = 0;

    	char local_name[1024] = {};
    	err = WooFLocalName((char *)cmq_frame_payload(woof_name), local_name, sizeof(local_name));

	char cap_name[1028] = {};
        sprintf(cap_name,"%s.CAP",local_name);
        // if we find a CAP and there should not be one, error
        if(no_cap == 1) {
                WOOF* wfc;
                wfc = WooFOpen(cap_name);
                if(wfc) {
                        WooFDrop(wfc);
                        return;
                }
        }

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
		// create empty frame (to be filled in later) for response
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

	// if response frame is not zero frame, fill it in with read using
	// frame payload as read buffer
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
	// done with woof name from request
	cmq_frame_destroy(woof_name);
	// done with local woof
	if(wf) {
		WooFDrop(wf);
	}

	// create response msg -- r_frame is holding response
	err = cmq_frame_list_create(&r_fl);
	if(err < 0) {
		DEBUG_WARN("WooFProcessGet: Could not allocate message");
		cmq_frame_destroy(r_frame);
		return;
	}

	// add r_frame to response msg
	// r_frame could be zero frame if open or read fails
	err = cmq_frame_append(r_fl,r_frame);
	if(err < 0) {
		cmq_frame_list_destroy(r_fl);
		cmq_frame_destroy(r_frame);
		DEBUG_WARN("WooFProcessGet: Could not append frame");
		return;
	}

	// send response -- timeout set in accept()
	err = cmq_pkt_send_msg(sd,r_fl);
	if(err < 0) {
		DEBUG_WARN("WooFProcessGet: Could not send response");
	}
	// destroy response msg
	cmq_frame_list_destroy(r_fl);
	return;
}

void WooFProcessGetwithCAP(unsigned char *fl, int sd) 
{
	unsigned char *cframe;
	unsigned char *wframe;
	char *wname;
	WCAP *cap;
	WOOF* wf;
	WCAP principal;
	unsigned long seq_no;
	int err;

	if(cmq_frame_list_empty(fl)) {
        	DEBUG_WARN("WooFProcessGetwithCAP Bad message");
		cmq_frame_list_destroy(fl);
        	return;
	}
	//
	// tag  has been stripped
	// first frame is woof name
	err = cmq_frame_pop(fl,&cframe);
	if(err < 0) {
		cmq_frame_list_destroy(fl);
        	DEBUG_WARN("WooFProcessGetwithCAP could not get cap frame");
        	return;
	}

	cap = (WCAP *)cmq_frame_payload(cframe);

	if(cap == NULL) {
		DEBUG_WARN("WooFProcessGetwithCAP: could not get woof cap frame payload\n");
		return;
	}

	wframe = cmq_frame_list_head(fl);
	if(wframe == NULL) {
		DEBUG_WARN("WooFProcessGetwithCAP: could not get woof name frame\n");
		return;
	}

	wname = (char *)cmq_frame_payload(wframe); // remaining frames
	if(wname == NULL) {
		DEBUG_WARN("WooFProcessGetwithCAP: could not get woof name frame payload\n");
		return;
	}

	char local_name[1024] = {};
    	err = WooFLocalName(wname, local_name, sizeof(local_name));
	if (err < 0) {
		DEBUG_WARN("WooFProcessGetwithCAP local name failed\n");
		return;
	}
	char cap_name[1028] = {};
	sprintf(cap_name,"%s.CAP",local_name);

	wf = WooFOpen(cap_name);
	// backwards compatibility: no CAP => authorized
	if(!wf) {
		WooFProcessGet(fl,sd,0);
		return;
	}
	seq_no = WooFLatestSeqno(wf);
	err = WooFReadWithCause(wf,&principal,seq_no,0,0);
	WooFDrop(wf);
	if(err < 0) {
		DEBUG_WARN("WooFProcessGetwithCAP cap get failed\n");
		return;
	}
	
	DEBUG_LOG("WooFProcessGetwithCAP: read CAP woof\n");
	// check read perms
	if(WooFCapAuthorized(principal.check,cap,WCAP_READ)) {
		DEBUG_WARN("WooFProcessGetwithCAP: CAP auth %s\n",cap_name);
		WooFProcessGet(fl,sd,0);
		return;
	} 
	DEBUG_WARN("WooFProcessGetwithCAP: read CAP denied %s\n",cap_name);
	// denied
	return;
}

unsigned char *LeakTest()
{
	unsigned char *fl;
	unsigned char *f;
	int err;

	err = cmq_frame_list_create(&fl);
	if(err < 0) {
		return(NULL);
	}

	const char *str = std::to_string(100).c_str();

	err = cmq_frame_create(&f,(unsigned char *)str,strlen(str)+1);
	if(err < 0) {
		cmq_frame_list_destroy(fl);
		return(NULL);
	}

	err = cmq_frame_append(fl,f);
	if(err < 0) {
		cmq_frame_list_destroy(fl);
		cmq_frame_destroy(f);
		return(NULL);
	}
	return(fl);
}

void WooFProcessGetLatestSeqno(unsigned char *fl, int sd, int no_cap) {

	int err;
	unsigned char *woof_name;
	unsigned char *r_fl;
	unsigned char *r_f;
	const char *s_str;
	
	if(cmq_frame_list_empty(fl)) {
        	DEBUG_WARN("WooFProcessGetLatestSeqno Bad message");
		cmq_frame_list_destroy(fl);
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

	// destroy request msg
	cmq_frame_list_destroy(fl);

	/*
	auto cause_host = 0;
	auto cause_seq_no = 0;
	auto cause_woof_latest_seq_no = 0;
	std::string cause_woof = "";
	*/

	char local_name[1024] = {};
	err = WooFLocalName((char *)cmq_frame_payload(woof_name), local_name, sizeof(local_name));
	        char cap_name[1028] = {};

        sprintf(cap_name,"%s.CAP",local_name);
        // if we find a CAP and there should not be one, error
        if(no_cap == 1) {
                WOOF* wfc;
                wfc = WooFOpen(cap_name);
                if(wfc) {
                        WooFDrop(wfc);
                        return;
                }
        }


	// try and open the woof
	WOOF* wf;
	if (err < 0) {
		wf = WooFOpen((char *)cmq_frame_payload(woof_name));
	} else {
		wf = WooFOpen(local_name);
	}

	// get latest seq_no if woof is open
	unsigned long latest_seq_no = -1;
	if (!wf) {
		DEBUG_WARN("WooFProcessGetLatestSeqno: couldn't open woof: %s\n", (char *)cmq_frame_payload(woof_name));
	} else {
		//latest_seq_no = WooFLatestSeqnoWithCause(wf, cause_host, cause_seq_no, cause_woof.c_str(), cause_woof_latest_seq_no);
		latest_seq_no = WooFLatestSeqno(wf);
		WooFDrop(wf);
	}
	// done with woof_name
	cmq_frame_destroy(woof_name);

	// create response msg
	err = cmq_frame_list_create(&r_fl);
	if(err < 0) {
        	DEBUG_WARN("WooFProcessGetLatestSeqno: Could not allocate message");
        	return;
	}

	// convbert latest seq_)no to string and create frame
	s_str = std::to_string(latest_seq_no).c_str();
	err = cmq_frame_create(&r_f,(unsigned char *)s_str,strlen(s_str)+1);
	if(err < 0) {
        	DEBUG_WARN("WooFProcessGetLatestSeqno: Could not allocate frame");
		cmq_frame_list_destroy(r_fl);
        	return;
	}
	// add latest seqno to msg
	err = cmq_frame_append(r_fl,r_f);
	if(err < 0) {
        	DEBUG_WARN("WooFProcessGetLatestSeqno: Could not append frame");
		cmq_frame_list_destroy(r_fl);
		cmq_frame_destroy(r_f);
        	return;
	}
	// send the response
	err = cmq_pkt_send_msg(sd,r_fl);
	if(err < 0) {
        	DEBUG_WARN("WooFProcessGetLatestSeqno: Could not send response");
        	return;
	}
	// destroy response msg
	cmq_frame_list_destroy(r_fl);
	return;

}

void WooFProcessGetLatestSeqnowithCAP(unsigned char *fl, int sd) 
{
	unsigned char *cframe;
	unsigned char *wframe;
	char *wname;
	WCAP *cap;
	WOOF* wf;
	WCAP principal;
	unsigned long seq_no;
	int err;

	if(cmq_frame_list_empty(fl)) {
        	DEBUG_WARN("WooFProcessGetLatestSeqnowithCAP Bad message");
		cmq_frame_list_destroy(fl);
        	return;
	}
	//
	// tag  has been stripped
	// first frame is woof name
	err = cmq_frame_pop(fl,&cframe);
	if(err < 0) {
		cmq_frame_list_destroy(fl);
        	DEBUG_WARN("WooFProcessGetLatestSeqnowithCAP could not get cap frame");
        	return;
	}

	cap = (WCAP *)cmq_frame_payload(cframe);

	if(cap == NULL) {
		DEBUG_WARN("WooFProcessGetLatestSeqnowithCAP: could not get woof cap frame payload\n");
		return;
	}

	wframe = cmq_frame_list_head(fl);
	if(wframe == NULL) {
		DEBUG_WARN("WooFProcessGetLatestSeqnowithCAP: could not get woof name frame\n");
		return;
	}

	wname = (char *)cmq_frame_payload(wframe); // remaining frames
	if(wname == NULL) {
		DEBUG_WARN("WooFProcessGetLatestSeqnowithCAP: could not get woof name frame payload\n");
		return;
	}

	char local_name[1024] = {};
    	err = WooFLocalName(wname, local_name, sizeof(local_name));
	if (err < 0) {
		DEBUG_WARN("WooFProcessGetLatestSeqnowithCAP local name failed\n");
		return;
	}
	char cap_name[1028] = {};
	sprintf(cap_name,"%s.CAP",local_name);

	wf = WooFOpen(cap_name);
	// backwards compatibility: no CAP => authorized
	if(!wf) {
		WooFProcessGetLatestSeqno(fl,sd,0);
		return;
	}
	seq_no = WooFLatestSeqno(wf);
	err = WooFReadWithCause(wf,&principal,seq_no,0,0);
	WooFDrop(wf);
	if(err < 0) {
		DEBUG_WARN("WooFProcessGetLatestSeqnowithCAP cap get failed\n");
		return;
	}
	
	DEBUG_LOG("WooFProcessGetLatestSeqnowithCAP: read CAP woof\n");
	// check read perms
	if(WooFCapAuthorized(principal.check,cap,WCAP_READ)) {
		DEBUG_WARN("WooFProcessGetLatestSeqnowithCAP: CAP auth %s\n",cap_name);
		WooFProcessGetLatestSeqno(fl,sd,0);
		return;
	} 
	DEBUG_WARN("WooFProcessGetLatestSeqnowithCAP: read CAP denied %s\n",cap_name);
	// denied
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

	// tag frame is stripped
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
	// convert frame payload to el_count
	el_count = strtoul((char *)cmq_frame_payload(f),NULL,10);
	// done with el_count frame
	cmq_frame_destroy(f);

	// done with request
	cmq_frame_list_destroy(fl);

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
		// create empty frame for space for el_count elements
		err = cmq_frame_create(&e_f,NULL,el_size * el_count);
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
		wf = NULL;
	}

	// done with woof name
	cmq_frame_destroy(woof_name);

	// create response message
	err = cmq_frame_list_create(&r_fl);
	if(err < 0) {
		DEBUG_WARN("WooFProcessGetTail could not allocate response list");
		return;
	}

	// convert el_read to string and create frame
	s_str = std::to_string(el_read).c_str();
	err = cmq_frame_create(&r_f,(unsigned char *)s_str,strlen(s_str)+1); 
	if(err < 0) {
		cmq_frame_list_destroy(r_fl);
		DEBUG_WARN("WooFProcessGetTail could not allocate frame for el_read");
		return;
	}
	// add el_read to response
	err = cmq_frame_append(r_fl,r_f);
	if(err < 0) {
		cmq_frame_list_destroy(r_fl);
		cmq_frame_destroy(r_f);
		DEBUG_WARN("WooFProcessGetTail could not append frame for el_read");
		return;
	}

	// if there is no data at the tail, send zero frame with no payload
	if(el_read <= 0) {
		err = cmq_frame_create(&e_f,NULL,0); // zero frame for elements
		if(err < 0) {
			cmq_frame_list_destroy(r_fl);
			DEBUG_WARN("WooFProcessGetTail could not allocate frame for elements");
			return;
		}
	}
	// append elements frame
	err = cmq_frame_append(r_fl,e_f);
	if(err < 0) {
		cmq_frame_list_destroy(r_fl);
		cmq_frame_destroy(e_f);
		DEBUG_WARN("WooFProcessGetTail could not append frame for elements");
		return;
	}

	// send the response
	err = cmq_pkt_send_msg(sd,r_fl);
	if(err < 0) {
		DEBUG_WARN("WooFProcessGetTail could not send response");
		return;
	}
	// destroy response msg
	cmq_frame_list_destroy(r_fl);
	return;
}
} // namespace cspot::cmq
