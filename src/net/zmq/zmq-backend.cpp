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
#include <woofc-caplets.h>

namespace cspot::zmq {
void WooFProcessGetElSize(ZMsgPtr req_msg, zsock_t* resp_sock, int no_cap) {
    auto res = ExtractMessage<std::string>(*req_msg);

    if (!res) {
        DEBUG_WARN("WooFProcessGetElSize Bad message");
        return;
    }

    auto& [woof_name] = *res;

    char local_name[1024] = {};
    auto err = WooFLocalName(woof_name.c_str(), local_name, sizeof(local_name));
    DEBUG_LOG("WooFProcessGetElSize: called on %s",local_name);

    char cap_name[1028] = {};
    // if we find a CAP and there should not be one, error
    if(no_cap == 1) {
    	sprintf(cap_name,"%s.CAP",local_name);
    	WOOF* wfc;
    	wfc = WooFOpen(cap_name);
    	if(wfc) {
	    WooFDrop(wfc);
            DEBUG_WARN("WooFProcessGetElSize:  no cap in message for %s, denined\n",local_name);
	    return;
	}
	strcpy(cap_name,"CSPOT.CAP");
    	wfc = WooFOpen(cap_name);
    	if(wfc) {
	    WooFDrop(wfc);
            DEBUG_WARN("WooFProcessGetElSize:  no cap in message with ns cap for %s, denined\n",local_name);
	    return;
	}
    }

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

    DEBUG_LOG("WooFProcessGetElSize: sending %lu for %s\n",el_size,local_name);
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

void WooFProcessGetElSizewithCAP(ZMsgPtr req_msg, zsock_t* resp_sock) 
{
	zframe_t *cframe;
	WCAP *cap;
	char *wname;
	WOOF* wf;
	WOOF* wf_ns;
	WCAP principal;
	WCAP ns_principal;
	unsigned long seq_no;
	int err;

	cframe = zmsg_pop(req_msg.get()); // pop the cap frame

	if(cframe == NULL) { // call withCAP and no cap => fail
		DEBUG_WARN("WooFProcessGetElSizewithCAP: no cap frame\n");
		return;
	}

	cap = (WCAP *)zframe_data(cframe);

	if(cap == NULL) {
		DEBUG_WARN("WooFProcessGetElSizewithCAP: could not get woof cap frame\n");
		return;
	}

	wname = (char *)zframe_data(zmsg_first(req_msg.get())); // remaining frames
	if(wname == NULL) {
		DEBUG_WARN("WooFProcessGetElSizewithCAP: could not get woof name frame\n");
		return;
	}

	char local_name[1024] = {};
    	err = WooFLocalName(wname, local_name, sizeof(local_name));
	if (err < 0) {
		DEBUG_WARN("WooFProcessGetElSizewithCAP local name failed\n");
		return;
	}
	char cap_name[1028] = {};
	strcpy(cap_name,"CSPOT.CAP");
	wf_ns = WooFOpen(cap_name);

	sprintf(cap_name,"%s.CAP",local_name);
	wf = WooFOpen(cap_name);

	// backwards compatibility: no CAP => authorized
	if(!wf && !wf_ns) {
		WooFProcessGetElSize(std::move(req_msg),resp_sock,0);
		return;
	}

	WCAP *new_cap_p = NULL;
	if(wf) {
		memset(&principal,0,sizeof(WCAP));
		seq_no = WooFLatestSeqno(wf);
		err = WooFReadWithCause(wf,&principal,seq_no,0,0);
		WooFDrop(wf);
		if(err < 0) {
			if(wf_ns) {
				WooFDrop(wf_ns);
			}
			DEBUG_WARN("WooFProcessGetElSizewithCAP cap get failed for woof cap\n");
			return;
		}
		new_cap_p = WooFCapAttenuate(&principal,WCAP_READ);
		if(new_cap_p == NULL) {
			DEBUG_WARN("WooFProcessGetElSizewithCAP attn cap failed for woof cap\n");
			if(wf_ns) {
				WooFDrop(wf_ns);
			}
			return;
		}
		DEBUG_LOG("WooFProcessGetElSizewithCAP cap get suceeded for cap %s\n",cap_name);
	}
	WCAP *new_cap_ns = NULL;
	if(wf_ns) {
		memset(&ns_principal,0,sizeof(WCAP));
		seq_no = WooFLatestSeqno(wf_ns);
		err = WooFReadWithCause(wf_ns,&ns_principal,seq_no,0,0);
		WooFDrop(wf_ns);
		if(err < 0) {
			if(wf) {
				WooFDrop(wf);
			}
			if(new_cap_p != NULL) {
				free(new_cap_p);
			}
			DEBUG_WARN("WooFProcessGetElSizewithCAP cap get failed for ns cap\n");
			return;
		}
		new_cap_ns = WooFCapAttenuate(&ns_principal,WCAP_READ);
		if(new_cap_ns == NULL) {
			DEBUG_WARN("WooFProcessGetElSizewithCAP attn cap failed for ns cap\n");
			if(new_cap_p != NULL) {
				free(new_cap_p);
			}
			if(wf) {
				WooFDrop(wf);
			}
			return;
		}
		DEBUG_LOG("WooFProcessGetElSizewithCAP cap get suceeded for ns cap\n");
	}
	
	// check the signature using attenuated CAP
	uint64_t sig_p = 0;
	if(new_cap_p != NULL) {
		sig_p = WooFCapSign((unsigned char *)wname,strlen(wname),new_cap_p->check);
		free(new_cap_p);
	}
	uint64_t sig_ns = 0;
	if(new_cap_ns != NULL) {
		sig_ns = WooFCapSign((unsigned char *)wname,strlen(wname),new_cap_ns->check);
		free(new_cap_ns);
	}

//printf("p check: %lu, sig_p: %lu\n", new_cap_p->check,sig_p);

	if((sig_p == cap->check) || (sig_ns == cap->check)) {
		DEBUG_WARN("WooFProcessGetGetElSizewithCAP: CAP auth\n");
		WooFProcessGetElSize(std::move(req_msg),resp_sock,0);
		return;
	}
	DEBUG_WARN("WooFProcessGetElSizewithCAP: read CAP denied\n");
	// denied
	return;
}

void WooFProcessPut(ZMsgPtr req_msg, zsock_t* resp_sock, int no_cap) {
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

    const char *hname;
    if(strcmp(hand_name.c_str(),"NULL") == 0) {
	    hname = NULL;
    } else {
	    hname = hand_name.c_str();
    }

    char local_name[1024] = {};
    auto err = WooFLocalName(woof_name.c_str(), local_name, sizeof(local_name));
    if (err < 0) {
        DEBUG_WARN("WooFProcessPut local name failed");
        return;
    }

    char cap_name[1028] = {};
    // if there is a cap there should not be one, error
    if(no_cap == 1) {
    	sprintf(cap_name,"%s.CAP",local_name);
    	WOOF* wfc;
    	wfc = WooFOpen(cap_name);
    	if(wfc) {
	    WooFDrop(wfc);
            DEBUG_WARN("WooFProcessput:  no cap in message for %s, denined\n",local_name);
	    return;
	}
	strcpy(cap_name,"CSPOT.CAP");
    	wfc = WooFOpen(cap_name);
    	if(wfc) {
	    WooFDrop(wfc);
            DEBUG_WARN("WooFProcessput:  no cap in message but ns cap for %s, denined\n",local_name);
	    return;
	}
    }
    unsigned long seq_no = WooFPutWithCause(
        local_name, (hname == NULL) ? nullptr : hand_name.c_str(), elem.data(), cause_host, cause_seq_no);

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

void WooFProcessPutwithCAP(ZMsgPtr req_msg, zsock_t* resp_sock) {
	zframe_t *cframe;
	WCAP *cap;
	char *wname;
	char *hname;
	WOOF* wf;
	WOOF* wf_ns;
	WCAP principal;
	WCAP ns_principal;
	unsigned long seq_no;
	int err;

	cframe = zmsg_pop(req_msg.get()); // pop the cap frame

	if(cframe == NULL) { // call withCAP and no cap => fail
		DEBUG_WARN("WooFProcessPutwithCAP: no cap frame\n");
		return;
	}
        auto res = ExtractMessage<std::string, std::string, /*std::string, std::string,*/ std::vector<uint8_t>>(*req_msg);
	auto& [woof_name, hand_name, /*name_id_str, log_seq_no_str,*/ elem] = *res;

	cap = (WCAP *)zframe_data(cframe);

	if(cap == NULL) {
		DEBUG_WARN("WooFProcessPutwithCAP: could not get woof cap frame\n");
		return;
	}

	wname = (char *)zframe_data(zmsg_first(req_msg.get())); // remaining frames
	if(wname == NULL) {
		DEBUG_WARN("WooFProcessPutwithCAP: could not get woof name frame\n");
		return;
	}

	hname = (char *)zframe_data(zmsg_next(req_msg.get()));
	if(hname == NULL) {
		DEBUG_WARN("WooFProcessPutwithCAP: could not get handler frame\n");
		return;
	}

	int elem_size = elem.size();

	char local_name[1024] = {};
    	err = WooFLocalName(wname, local_name, sizeof(local_name));
	if (err < 0) {
		DEBUG_WARN("WooFProcessPutwithCAP local name failed\n");
		return;
	}
	char cap_name[1028] = {};
	strcpy(cap_name,"CSPOT.CAP");
	wf_ns = WooFOpen(cap_name);
	sprintf(cap_name,"%s.CAP",local_name);
	wf = WooFOpen(cap_name);

	// backwards compatibility: no CAP => authorized
	if(!wf_ns && !wf) {
		WooFProcessPut(std::move(req_msg),resp_sock,0);
		return;
	}

	WCAP *new_cap_p = NULL;
	if(wf){
		memset(&principal,0,sizeof(WCAP));
		seq_no = WooFLatestSeqno(wf);
		err = WooFReadWithCause(wf,&principal,seq_no,0,0);
		WooFDrop(wf);
		if(err < 0) {
			if(wf_ns) {
				WooFDrop(wf_ns);
			}
			DEBUG_WARN("WooFProcessPutwithCAP cap get failed for woof cap\n");
			return;
		}
		if(strncmp(hname,"NULL",strlen("NULL")) == 0) {
			new_cap_p = WooFCapAttenuate(&principal,WCAP_WRITE);
		} else {
			new_cap_p = WooFCapAttenuate(&principal,WCAP_EXEC);
		}
		if(new_cap_p == NULL) {
			if(wf_ns) {
				WooFDrop(wf_ns);
			}
			DEBUG_WARN("WooFProcessPutwithCAP attn cap failed for woof cap\n");
			return;
		}
		DEBUG_LOG("WooFProcessPutwithCAP cap get suceeded for woof cap %s\n",cap_name);
	}
	WCAP *new_cap_ns = NULL;
	if(wf_ns) {
		memset(&ns_principal,0,sizeof(WCAP));
		seq_no = WooFLatestSeqno(wf_ns);
		err = WooFReadWithCause(wf_ns,&ns_principal,seq_no,0,0);
		WooFDrop(wf_ns);
		if(err < 0) {
			if(wf) {
				WooFDrop(wf);
			}
			if(new_cap_p != NULL) {
				free(new_cap_p);
			}
			DEBUG_WARN("WooFProcessPutwithCAP cap get failed for ns cap\n");
			return;
		}
		if(strncmp(hname,"NULL",strlen("NULL")) == 0) {
			new_cap_ns = WooFCapAttenuate(&ns_principal,WCAP_WRITE);
		} else {
			new_cap_ns = WooFCapAttenuate(&ns_principal,WCAP_EXEC);
		}
		if(new_cap_ns == NULL) {
			DEBUG_WARN("WooFProcessPutwithCAP attn cap failed for ns cap\n");
			if(new_cap_p != NULL) {
				free(new_cap_p);
			}
			if(wf) {
				WooFDrop(wf);
			}
			return;
		}
		DEBUG_LOG("WooFProcessPutwithCAP cap get suceeded for ns cap\n");
	}

	int psize = elem.size() + strlen(wname) + strlen(hname);
	unsigned char *payload = (unsigned char *)malloc(psize);
	if(payload == NULL) {
		DEBUG_WARN("WooFProcessPutwithCAP could not get space for sig payload\n");
		if(new_cap_p != NULL) {
			free(new_cap_p);
		}
		if(new_cap_ns != NULL) {
			free(new_cap_ns);
		}
		return;
	}
	unsigned char *pptr = payload;
	memset(payload,0,psize);
	memcpy(pptr,wname,strlen(wname));
	pptr += strlen(wname);
	memcpy(pptr,hname,strlen(hname));
	pptr += strlen(hname);
	memcpy(pptr,elem.data(),elem.size());
	uint64_t sig_p = 0;
	if(new_cap_p != NULL) {
		sig_p = WooFCapSign((unsigned char *)payload,psize,new_cap_p->check);
		free(new_cap_p);
	}
	uint64_t sig_ns = 0;
	if(new_cap_ns != NULL) {
		sig_ns = WooFCapSign((unsigned char *)payload,psize,new_cap_ns->check);
		free(new_cap_ns);
	}
	free(payload);
	if((cap->check == sig_p) || (cap->check == sig_ns)) {
		WooFProcessPut(std::move(req_msg),resp_sock,0);
		DEBUG_WARN("WooFProcessPutwithCAP: no handler auth %s\n",cap_name);
		return;
	}

	// denied
	DEBUG_WARN("WooFProcessPutwithCAP: cap auth failed handler: %s check %lu, denied\n",
				hname,
				cap->check);
	return;
}

void WooFProcessGet(ZMsgPtr req_msg, zsock_t* resp_sock, int no_cap) {
    auto res = ExtractMessage<std::string, std::string/*, std::string, std::string*/>(*req_msg);
    unsigned long esize;

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

    char cap_name[1028] = {};
    // if there is a cap there should not be one, error
    if(no_cap == 1) {
    	sprintf(cap_name,"%s.CAP",local_name);
    	WOOF* wfc;
    	wfc = WooFOpen(cap_name);
    	if(wfc) {
	    WooFDrop(wfc);
            DEBUG_WARN("WooFProcessGet:  no cap in message for %s, denined\n",local_name);
	    return;
	}
	strcpy(cap_name,"CSPOT.CAP");
    	wfc = WooFOpen(cap_name);
    	if(wfc) {
	    WooFDrop(wfc);
            DEBUG_WARN("WooFProcessGet:  no cap in message but ns cap for %s, denined\n",local_name);
	    return;
	}
    }
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
	esize = wf->shared->element_size;
        elem = std::vector<uint8_t>(wf->shared->element_size);
        err = WooFReadWithCause(wf, elem.data(), seq_no, cause_host, cause_seq_no);
        if (err < 0) {
            DEBUG_WARN("WooFProcessGet: read failed: %s at %lu\n", woof_name.c_str(), seq_no);
            elem = {};
        }
        WooFDrop(wf);
    }

    DEBUG_LOG("WooFProcessGet: responding with element size %d\n",esize);
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

void WooFProcessGetwithCAP(ZMsgPtr req_msg, zsock_t* resp_sock) 
{
	zframe_t *cframe;
	WCAP *cap;
	char *wname;
	WOOF* wf;
	WOOF* wf_ns;
	WCAP principal;
	WCAP ns_principal;
	unsigned long seq_no;
	int err;

	cframe = zmsg_pop(req_msg.get()); // pop the cap frame

	if(cframe == NULL) { // call withCAP and no cap => fail
		DEBUG_WARN("WooFProcessGetwithCAP: no cap frame\n");
		return;
	}

	cap = (WCAP *)zframe_data(cframe);

	if(cap == NULL) {
		DEBUG_WARN("WooFProcessGetwithCAP: could not get woof cap frame\n");
		return;
	}

	wname = (char *)zframe_data(zmsg_first(req_msg.get())); // remaining frames
	if(wname == NULL) {
		DEBUG_WARN("WooFProcessGetwithCAP: could not get woof name frame\n");
		return;
	}

	char local_name[1024] = {};
    	err = WooFLocalName(wname, local_name, sizeof(local_name));
	if (err < 0) {
		DEBUG_WARN("WooFProcessGetwithCAP local name failed\n");
		return;
	}
	char cap_name[1028] = {};
	strcpy(cap_name,"CSPOT.CAP");
	wf_ns = WooFOpen(cap_name);

	sprintf(cap_name,"%s.CAP",local_name);
	wf = WooFOpen(cap_name);
	// backwards compatibility: no CAP => authorized
	if(!wf && !wf_ns) {
		WooFProcessGet(std::move(req_msg),resp_sock,0);
		return;
	}
	WCAP *new_cap_p = NULL;
	if(wf) {
		memset(&principal,0,sizeof(WCAP));
		seq_no = WooFLatestSeqno(wf);
		err = WooFReadWithCause(wf,&principal,seq_no,0,0);
		WooFDrop(wf);
		if(err < 0) {
			if(wf_ns) {
				WooFDrop(wf_ns);
			}
			DEBUG_WARN("WooFProcessGetwithCAP cap get failed\n");
			return;
		}
		new_cap_p = WooFCapAttenuate(&principal,WCAP_READ);
		if(new_cap_p == NULL) {
			if(wf_ns) {
				WooFDrop(wf_ns);
			}
			DEBUG_WARN("WooFProcessGetwithCAP attn cap failed\n");
			return;
		}
		DEBUG_LOG("WooFProcessGetwithCAP: cap get suceeded CAP %s\n",cap_name);
	}
	WCAP *new_cap_ns = NULL;
	if(wf_ns) {
		memset(&ns_principal,0,sizeof(WCAP));
		seq_no = WooFLatestSeqno(wf_ns);
		err = WooFReadWithCause(wf_ns,&ns_principal,seq_no,0,0);
		WooFDrop(wf_ns);
		if(err < 0) {
			if(wf) {
				WooFDrop(wf);
			}
			if(new_cap_p != NULL) {
				free(new_cap_p);
			}
			DEBUG_WARN("WooFProcessGetwithCAP cap get failed for ns\n");
			return;
		}
		new_cap_ns = WooFCapAttenuate(&ns_principal,WCAP_READ);
		if(new_cap_ns == NULL) {
			DEBUG_WARN("WooFProcessGetwithCAP attn cap failed for ns\n");
			if(wf) {
				WooFDrop(wf);
			}
			if(new_cap_p != NULL) {
				free(new_cap_p);
			}
			return;
		}
		DEBUG_LOG("WooFProcessGetwithCAP: cap get suceeded for ns CAP\n");
	}

	zmsg_t *zm = req_msg.get();
	zframe_t *sf = zmsg_first(zm); // woof name
	sf = zmsg_next(zm); // seqno
	char *c_seqno = (char *)zframe_data(sf);
	char payload[2048];
	snprintf(payload,sizeof(payload),"%s %s",wname,c_seqno);
	
	uint64_t sig_p = 0;
	if(new_cap_p != NULL) {
		sig_p = WooFCapSign((unsigned char *)payload,strlen(payload),new_cap_p->check);
		free(new_cap_p);
	}

	uint64_t sig_ns = 0;
	if(new_cap_ns != NULL) {
		sig_ns = WooFCapSign((unsigned char *)payload,strlen(payload),new_cap_ns->check);
		free(new_cap_ns);
	}

	if((cap->check == sig_p) || (cap->check == sig_ns)) {
		// reset cursor
		(void)zmsg_first(req_msg.get());
		DEBUG_LOG("WooFProcessGetwithCAP: CAP auth\n");
		WooFProcessGet(std::move(req_msg),resp_sock,0);
		return;
	}

	DEBUG_WARN("WooFProcessGetwithCAP: read CAP denied\n");
	// denied
	return;
}

void WooFProcessGetLatestSeqno(ZMsgPtr req_msg, zsock_t* resp_sock, int no_cap) {
    auto res = ExtractMessage<std::string/*, std::string, std::string, std::string, std::string*/>(*req_msg);

    if (!res) {
        DEBUG_WARN("WooFProcessGetLatestSeqno Bad message");
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

    char cap_name[1028] = {};
    // if there is a cap there should not be one, error
    if(no_cap == 1) {
    	sprintf(cap_name,"%s.CAP",local_name);
    	WOOF* wfc;
    	wfc = WooFOpen(cap_name);
    	if(wfc) {
            DEBUG_WARN("WooFProcessGetLatestSeqno: found CAP with no cap in message%s\n", woof_name.c_str());
	    WooFDrop(wfc);
	    return;
	}
	strcpy(cap_name,"CSPOT.CAP");
    	wfc = WooFOpen(cap_name);
    	if(wfc) {
            DEBUG_WARN("WooFProcessGetLatestSeqno: found CAP with no cap in message%s with ns\n", woof_name.c_str());
	    WooFDrop(wfc);
	    return;
	}
    }

    WOOF* wf;
    if (err < 0) {
        wf = WooFOpen(woof_name.c_str());
    } else {
        wf = WooFOpen(local_name);
    }

    unsigned long latest_seq_no = -1;
    if (!wf) {
        DEBUG_WARN("WooFProcessGetLatestSeqno: couldn't open woof: %s\n", woof_name.c_str());
    } else {
        latest_seq_no = WooFLatestSeqno(wf);
//            WooFLatestSeqnoWithCause(wf, cause_host, cause_seq_no, cause_woof.c_str(), cause_woof_latest_seq_no);
        WooFDrop(wf);
    }

    DEBUG_LOG("WooFProcessGetLatestSeqno: sending %lu for %s\n",latest_seq_no,woof_name.c_str());
    auto resp = CreateMessage(std::to_string(latest_seq_no));
    if (!res) {
        DEBUG_WARN("WooFProcessGetLatestSeqno: Could not allocate message");
        return;
    }

    if (!Send(std::move(resp), *resp_sock)) {
        DEBUG_WARN("WooFProcessGetLatestSeqno: Could not send response");
        return;
    }
}

void WooFProcessGetLatestSeqnowithCAP(ZMsgPtr req_msg, zsock_t* resp_sock) 
{
	zframe_t *cframe;
	WCAP *cap;
	char *wname;
	WOOF* wf;
	WOOF* wf_ns;
	WCAP principal;
	WCAP ns_principal;
	unsigned long seq_no;
	int err;
	zframe_t *zm;

	cframe = zmsg_pop(req_msg.get()); // pop the cap frame

	if(cframe == NULL) { // call withCAP and no cap => fail
		DEBUG_WARN("WooFProcessGetLatestSeqnowithCAP: no cap frame\n");
		return;
	}

	cap = (WCAP *)zframe_data(cframe);

	if(cap == NULL) {
		DEBUG_WARN("WooFProcessGetLatestSeqnowithCAP: could not get woof cap frame\n");
		return;
	}

	wname = (char *)zframe_data(zmsg_first(req_msg.get())); // remaining frames
	if(wname == NULL) {
		DEBUG_WARN("WooFProcessGetLatestSeqnowithCAP: could not get woof name frame\n");
		return;
	}

	char local_name[1024] = {};
    	err = WooFLocalName(wname, local_name, sizeof(local_name));
	if (err < 0) {
		DEBUG_WARN("WooFProcessGetLatestSeqnowithCAP local name failed\n");
		return;
	}
	char cap_name[1028] = {};
	strcpy(cap_name,"CSPOT.CAP");
	wf_ns = WooFOpen(cap_name);

	sprintf(cap_name,"%s.CAP",local_name);
	wf = WooFOpen(cap_name);
	// backwards compatibility: no CAP => authorized
	if(!wf && !wf_ns) {
		WooFProcessGetLatestSeqno(std::move(req_msg),resp_sock,0);
		return;
	}
	WCAP *new_cap_p = NULL;
	if(wf) {
		memset(&principal,0,sizeof(WCAP));
		seq_no = WooFLatestSeqno(wf);
		err = WooFReadWithCause(wf,&principal,seq_no,0,0);
		WooFDrop(wf);
		if(err < 0) {
			if(wf_ns) {
				WooFDrop(wf_ns);
			}
			DEBUG_WARN("WooFProcessGetLatestSeqnowithCAP cap get failed for %s\n",cap_name);
			return;
		}
		DEBUG_LOG("WooFProcessGetLatestSeqnowithCAP: read CAP from %s\n",cap_name);
		new_cap_p = WooFCapAttenuate(&principal, WCAP_READ);
		if(new_cap_p == NULL) {
			if(wf_ns != NULL) {
				WooFDrop(wf_ns);
			}
			DEBUG_WARN("WooFProcessGetLatestSeqnowithCAP atten cap failed for principal\n");
			return;
		}
	}
	WCAP *new_cap_ns = NULL;
	if(wf_ns) {
		memset(&ns_principal,0,sizeof(WCAP));
		seq_no = WooFLatestSeqno(wf_ns);
		err = WooFReadWithCause(wf_ns,&ns_principal,seq_no,0,0);
		WooFDrop(wf_ns);
		if(err < 0) {
			if(wf) {
				WooFDrop(wf);
			}
			if(new_cap_p != NULL) {
				free(new_cap_p);
			}
			DEBUG_WARN("WooFProcessGetLatestSeqnowithCAP cap get failed for ns\n");
			return;
		}
		DEBUG_LOG("WooFProcessGetLatestSeqnowithCAP: read CAP from ns\n");
		new_cap_ns = WooFCapAttenuate(&ns_principal, WCAP_READ);
		if(new_cap_ns == NULL) {
			DEBUG_WARN("WooFProcessGetLatestSeqnowithCAP atten cap failed for ns principal\n");
			if(new_cap_p != NULL) {
				free(new_cap_p);
			}
			if(wf) {
				WooFDrop(wf);
			}
			return;
		}
	}

	// check the signature using attenuated CAP
	uint64_t sig_p = 0;
	if(new_cap_p != NULL) {
		sig_p = WooFCapSign((unsigned char *)wname,strlen(wname),new_cap_p->check);
		free(new_cap_p);
	}
	uint64_t sig_ns = 0;
	if(new_cap_ns != NULL) {
		sig_ns = WooFCapSign((unsigned char *)wname,strlen(wname),new_cap_ns->check);
		free(new_cap_ns);
	}


	if((sig_p == cap->check) || (sig_ns == cap->check)) {
		DEBUG_WARN("WooFProcessGetLatestSeqnowithCAP: CAP auth\n");
		WooFProcessGetLatestSeqno(std::move(req_msg),resp_sock,0);
		return;
	}
	
	DEBUG_WARN("WooFProcessGetLatestSeqnowithCAP: read CAP denied %s\n",cap_name);
	// denied
	return;
}

void WooFProcessGetTail(ZMsgPtr req_msg, zsock_t* resp_sock, int no_cap) {
    auto res = ExtractMessage<std::string, std::string>(*req_msg);

    if (!res) {
        DEBUG_WARN("WooFProcessGet Bad message");
        return;
    }

    auto& [woof_name, el_count_str] = *res;
    auto el_count = std::stoul(el_count_str);

    char local_name[1024] = {};
    auto err = WooFLocalName(woof_name.c_str(), local_name, sizeof(local_name));

    char cap_name[1028] = {};
    // if there is a cap there should not be one, error
    if(no_cap == 1) {
    	sprintf(cap_name,"%s.CAP",local_name);
    	WOOF* wfc;
    	wfc = WooFOpen(cap_name);
    	if(wfc) {
	    WooFDrop(wfc);
	    return;
	}
	strcpy(cap_name,"CSPOT.CAP");
    	wfc = WooFOpen(cap_name);
    	if(wfc) {
	    WooFDrop(wfc);
	    return;
	}
    }

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

void WooFProcessGetTailwithCAP(ZMsgPtr req_msg, zsock_t* resp_sock) 
{
	zframe_t *cframe;
	WCAP *cap;
	char *wname;
	WOOF* wf;
	WCAP principal;
	unsigned long seq_no;
	int err;

	cframe = zmsg_pop(req_msg.get()); // pop the cap frame

	if(cframe == NULL) { // call withCAP and no cap => fail
		DEBUG_WARN("WooFProcessGetTailwithCAP: no cap frame\n");
		return;
	}

	cap = (WCAP *)zframe_data(cframe);

	if(cap == NULL) {
		DEBUG_WARN("WooFProcessGetTailwithCAP: could not get woof cap frame\n");
		return;
	}

	wname = (char *)zframe_data(zmsg_first(req_msg.get())); // remaining frames
	if(wname == NULL) {
		DEBUG_WARN("WooFProcessGetTailwithCAP: could not get woof name frame\n");
		return;
	}

	char local_name[1024] = {};
    	err = WooFLocalName(wname, local_name, sizeof(local_name));
	if (err < 0) {
		DEBUG_WARN("WooFProcessGetTailwithCAP local name failed\n");
		return;
	}
	char cap_name[1028] = {};
	sprintf(cap_name,"%s.CAP",local_name);

	wf = WooFOpen(cap_name);
	// backwards compatibility: no CAP => authorized
	if(!wf) {
		WooFProcessGetTail(std::move(req_msg),resp_sock,0);
		return;
	}
	seq_no = WooFLatestSeqno(wf);
	err = WooFReadWithCause(wf,&principal,seq_no,0,0);
	WooFDrop(wf);
	if(err < 0) {
		DEBUG_WARN("WooFProcessGetTailwithCAP cap get failed\n");
		return;
	}
	
	DEBUG_LOG("WooFProcessGetTailwithCAP: read CAP woof\n");
	//
	// reset cursor
	wname = (char *)zmsg_first(req_msg.get());
	// check read perms
	if(WooFCapAuthorized(principal.check,cap,WCAP_READ)) {
		DEBUG_WARN("WooFProcessGetLatestSeqnowithCAP: CAP auth %s\n",cap_name);
		WooFProcessGetTail(std::move(req_msg),resp_sock,0);
		return;
	} 
	DEBUG_WARN("WooFProcessGetTailwithCAP: read CAP denied %s\n",cap_name);
	// denied
	return;
}

} // namespace cspot::zmq
