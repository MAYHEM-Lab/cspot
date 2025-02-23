#include "common.hpp"
#include "backend.hpp"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <cstring>
#include <debug.h>
#include <global.h>

extern "C" {
#include <cmq-pkt.h>
}

namespace cspot::cmq {
int32_t backend::remote_get(std::string_view woof_name, void* elem, uint32_t elem_size, uint32_t seq_no) {
    auto ip = ip_from_woof(woof_name);
    auto port = port_from_woof(woof_name);
    std::string woof_n(woof_name);
    int sd;
    int err;
    unsigned char *f;
    unsigned char *fl;

    if (!ip) {
        return -1;
    }
    if (!port) {
        return -1;
    }

    if (elem_size == (uint32_t)-1) {
        return (-1);
    }

    err = cmq_frame_list_create(&fl);
    if(err < 0) {
        DEBUG_WARN("Could not connect create msg for WoofMsgGet");
	printf("WooFMsgGet: could not create msg\n");
	return -1;
    }

    const char *cmd = std::to_string(WOOF_MSG_GET).c_str();
    err = cmq_frame_create(&f,(unsigned char *)cmd,strlen(cmd)+1);
    if(err < 0) {
        DEBUG_WARN("Could not connect create cmd for WoofMsgGet");
	printf("WooFMsgGet: could not create cmd\n");
	cmq_frame_list_destroy(fl);
	return -1;
    }
    err = cmq_frame_append(fl,f);
    if(err < 0) {
        DEBUG_WARN("Could not connect append cmd for WoofMsgGet");
	printf("WooFMsgGet: could not append cmd\n");
	cmq_frame_list_destroy(fl);
	return -1;
    }

    const unsigned char *w_ptr = reinterpret_cast<const unsigned char*>(woof_name.data());
    err = cmq_frame_create(&f,(unsigned char *)w_ptr,strlen((const char *)w_ptr)+1);
    if(err < 0) {
        DEBUG_WARN("Could not connect create woof_name for WoofMsgGet");
	printf("WooFMsgGet: could not create woof_name\n");
	cmq_frame_list_destroy(fl);
	return -1;
    }
    err = cmq_frame_append(fl,f);
    if(err < 0) {
        DEBUG_WARN("Could not connect append woof_name for WoofMsgGet");
	printf("WooFMsgGet: could not append woof_name\n");
	cmq_frame_list_destroy(fl);
	return -1;
    }

    const char *sn = std::to_string(seq_no).c_str();
    err = cmq_frame_create(&f,(unsigned char *)sn,strlen(sn)+1);
    if(err < 0) {
        DEBUG_WARN("Could not connect create seqno for WoofMsgGet");
	printf("WooFMsgGet: could not create seq_no\n");
	cmq_frame_list_destroy(fl);
	return -1;
    }
    err = cmq_frame_append(fl,f);
    if(err < 0) {
        DEBUG_WARN("Could not connect append seq_no for WoofMsgGet");
	printf("WooFMsgGet: could not append seq_no\n");
	cmq_frame_list_destroy(fl);
	return -1;
    }


    /*
    auto msg = CreateMessage(std::to_string(WOOF_MSG_GET),
                             std::string(woof_name),
                             std::to_string(seq_no)
                             );
     */

    std::string ip_str = ip.value();
    const char *c_ip_str = ip_str.c_str();

    std::string port_str = port.value();

    sd = cmq_pkt_connect((char *)c_ip_str, stoi(port_str), WOOF_MSG_REQ_TIMEOUT);
    if(sd < 0) {
        DEBUG_WARN("Could not connect to server for WoofMsgGet");
	printf("WooFMsgGet: server connect failed to %s:%d\n",
		c_ip_str,stoi(port_str));
	cmq_frame_list_destroy(fl);
	return -1;
    }

    err = cmq_pkt_send_msg(sd,fl);
    if(err < 0) {
        DEBUG_WARN("Could not send to server for WoofMsgGet");
	printf("WooFMsgGet: server request send failed to %s:%d\n",
		c_ip_str,
		stoi(port_str));
	cmq_frame_list_destroy(fl);
	close(sd);
        return -1;
    }

    cmq_frame_list_destroy(fl);

    unsigned char *r_fl;
    err = cmq_pkt_recv_msg(sd,&r_fl);
    if(err < 0) {
        DEBUG_WARN("Could not receive reply for WoofMsgGet");
	printf("WooFMsgGet: server request recv failed from %s:%d\n",
		c_ip_str,stoi(port_str));
	perror("WooFMsgGet");
	close(sd);
        return -1;
    }

    close(sd);

    if(cmq_frame_list_empty(r_fl)) {
        DEBUG_WARN("Empty receive reply for WoofMsgGet");
	printf("WooFMsgGet: empty recv from %s:%d\n",
		c_ip_str,stoi(port_str));
	perror("WooFMsgGet");
	cmq_frame_list_destroy(r_fl);
        return -1;
    }

    unsigned char *frame;
    err = cmq_frame_pop(r_fl, &frame);
    if(err < 0) {
        DEBUG_WARN("msg pop failed receive reply for WoofMsgGet");
	printf("WooFMsgGet: pop failed for recv from %s:%d\n",
		c_ip_str,stoi(port_str));
	perror("WooFMsgGet");
	cmq_frame_list_destroy(r_fl);
        return -1;
    }

    cmq_frame_list_destroy(r_fl);

    // zero frame indicates read error
    if((cmq_frame_payload(frame) == NULL) ||
	(cmq_frame_size(frame) == 0))
    {
        DEBUG_WARN("empty frame receive reply for WoofMsgGet");
	printf("WooFMsgGet: empty frame for recv from %s:%d\n",
		c_ip_str,stoi(port_str));
	//perror("WooFMsgGet");
	cmq_frame_destroy(frame);
        return -1;
    }

    std::memcpy(elem, cmq_frame_payload(frame), cmq_frame_size(frame));

    cmq_frame_destroy(frame);
    return 1;
}

int32_t backend::remote_get_tail(std::string_view woof_name, void* elements, unsigned long el_size, int el_count) {
    auto ip = ip_from_woof(woof_name);
    auto port = port_from_woof(woof_name);
    std::string woof_n(woof_name);
    int sd;
    int err;
    unsigned char *f;
    unsigned char *fl;

    if (!ip) {
        return -1;
    }
    if (!port) {
        return -1;
    }


    if (el_size == (unsigned long)-1) {
        return (-1);
    }

    err = cmq_frame_list_create(&fl);
    if(err < 0) {
        DEBUG_WARN("Could not connect create msg for WoofMsgGetTail");
	printf("WooFMsgGetTail: could not create msg\n");
	return -1;
    }

    const char *cmd = std::to_string(WOOF_MSG_GET_TAIL).c_str();
    err = cmq_frame_create(&f,(unsigned char *)cmd,strlen(cmd)+1);
    if(err < 0) {
        DEBUG_WARN("Could not connect create cmd for WoofMsgGetTail");
	printf("WooFMsgGetTail: could not create cmd\n");
	cmq_frame_list_destroy(fl);
	return -1;
    }
    err = cmq_frame_append(fl,f);
    if(err < 0) {
        DEBUG_WARN("Could not connect append cmd for WoofMsgGetTail");
	printf("WooFMsgGetTail: could not append cmd\n");
	cmq_frame_list_destroy(fl);
	return -1;
    }

    const unsigned char *w_ptr = reinterpret_cast<const unsigned char*>(woof_name.data());
    err = cmq_frame_create(&f,(unsigned char *)w_ptr,strlen((const char *)w_ptr)+1);
    if(err < 0) {
        DEBUG_WARN("Could not connect create woof_name for WoofMsgGetTail");
	printf("WooFMsgGetTail: could not create woof_name\n");
	cmq_frame_list_destroy(fl);
	return -1;
    }
    err = cmq_frame_append(fl,f);
    if(err < 0) {
        DEBUG_WARN("Could not connect append woof_name for WoofMsgGetTail");
	printf("WooFMsgGetTail: could not append woof_name\n");
	cmq_frame_list_destroy(fl);
    }

    const char *el_c = std::to_string(el_count).c_str();
    err = cmq_frame_create(&f,(unsigned char *)el_c,strlen(el_c)+1);
    if(err < 0) {
        DEBUG_WARN("Could not connect create el_count for WoofMsgGetTail");
	printf("WooFMsgGetTail: could not create el_count\n");
	cmq_frame_list_destroy(fl);
	return -1;
    }
    err = cmq_frame_append(fl,f);
    if(err < 0) {
        DEBUG_WARN("Could not connect append el_count for WoofMsgGetTail");
	printf("WooFMsgGet: could not append el_count\n");
	cmq_frame_list_destroy(fl);
	return -1;
    }

    /*
    auto msg = CreateMessage(std::to_string(WOOF_MSG_GET_TAIL), std::string(woof_name), std::to_string(el_count));
    */

    std::string ip_str = ip.value();
    const char *c_ip_str = ip_str.c_str();

    std::string port_str = port.value();

    sd = cmq_pkt_connect((char *)c_ip_str, stoi(port_str), WOOF_MSG_REQ_TIMEOUT);
    if(sd < 0) {
        DEBUG_WARN("Could not connect to server for WoofMsgGetTail");
	printf("WooFMsgGetTail: server connect failed to %s:%d\n",
		c_ip_str,stoi(port_str));
	cmq_frame_list_destroy(fl);
	return -1;
    }

    err = cmq_pkt_send_msg(sd,fl);
    if(err < 0) {
        DEBUG_WARN("Could not send to server for WoofMsgGetTail");
	printf("WooFMsgGetTail: server request send failed to %s:%d\n",
		c_ip_str,
		stoi(port_str));
	cmq_frame_list_destroy(fl);
	close(sd);
        return -1;
    }
    cmq_frame_list_destroy(fl);

    // auto r_msg = ZMsgPtr(ServerRequest(endpoint.c_str(), std::move(msg)));

    unsigned char *r_fl;
    err = cmq_pkt_recv_msg(sd,&r_fl);
    if(err < 0) {
        DEBUG_WARN("Could not receive reply for WoofMsgGetTail");
	printf("WooFMsgGetTail: server request recv failed from %s:%d\n",
		c_ip_str,stoi(port_str));
	perror("WooFMsgGetTail");
	close(sd);
        return -1;
    }

    close(sd);

    if(cmq_frame_list_empty(r_fl)) {
        DEBUG_WARN("Empty receive reply for WoofMsgGetTail");
	printf("WooFMsgGetTail: empty recv from %s:%d\n",
		c_ip_str,stoi(port_str));
	perror("WooFMsgGetTail");
	cmq_frame_list_destroy(r_fl);
        return -1;
    }

    // first frame is element count
    unsigned char *frame;
    err = cmq_frame_pop(r_fl, &frame);
    if(err < 0) {
        DEBUG_WARN("msg pop failed receive reply for WoofMsgGetTail");
	printf("WooFMsgGetTail: pop failed for recv from %s:%d\n",
		c_ip_str,stoi(port_str));
	perror("WooFMsgGetTail");
	cmq_frame_list_destroy(r_fl);
        return -1;
    }

    if((cmq_frame_payload(frame) == NULL) ||
	(cmq_frame_size(frame) == 0))
    {
        DEBUG_WARN("empty first frame receive reply for WoofMsgGetTail");
	printf("WooFMsgGetTail: empty frame for recv from %s:%d\n",
		c_ip_str,stoi(port_str));
	perror("WooFMsgGetTail");
	cmq_frame_destroy(frame);
        return -1;
    }

    unsigned long el_read = strtoul((char *)cmq_frame_payload(frame),NULL,10);
    cmq_frame_destroy(frame);

    // second frame is elements
    // // could be zero frame
    err = cmq_frame_pop(r_fl,&frame);
    if(err < 0) {
        DEBUG_WARN("msg 2 pop failed receive reply for WoofMsgGetTail");
	printf("WooFMsgGetTail: second pop failed for recv from %s:%d\n",
		c_ip_str,stoi(port_str));
	perror("WooFMsgGetTail");
	cmq_frame_list_destroy(r_fl);
        return -1;
    }
    cmq_frame_list_destroy(r_fl);

    if((cmq_frame_payload(frame) == NULL) ||
	(cmq_frame_size(frame) == 0)) {
        DEBUG_WARN("empty second frame receive reply for WoofMsgGetTail");
	printf("WooFMsgGetTail: empty second frame for recv from %s:%d\n",
		c_ip_str,stoi(port_str));
	perror("WooFMsgGetTail");
	cmq_frame_destroy(frame);
        return -1;
    }

    // check to see if got eveything based on size
    if(cmq_frame_size(frame) != (el_count*el_size)) {
        DEBUG_WARN("frame size != el_count*el_size receive reply for WoofMsgGetTail");
	printf("WooFMsgGetTail: size match failed for recv from %d, %d, %ld\n",
		cmq_frame_size(frame),el_count,el_size);
    }
    int len = cmq_frame_size(frame);
    if((unsigned int)len < (el_count*el_size)) {
	    len = el_count*el_size;
	    el_read = el_count;
    }

    std::memcpy(elements, cmq_frame_payload(frame), len);

    cmq_frame_destroy(frame);
    return el_read;

}

int32_t
backend::remote_put(std::string_view woof_name, const char* handler_name, const void* elem, uint32_t elem_size) {
	auto ip = ip_from_woof(woof_name);
	auto port = port_from_woof(woof_name);
	int err;
	int sd;
	unsigned char *fl;
	unsigned char *f;
	unsigned char *r_fl;
	unsigned char *r_f;

	if(!ip) {
	    return(-1);
	}
	if(!port) {
	    return(-1);
	}

	if (elem_size == (uint32_t)-1) {
		return (-1);
	}

	err = cmq_frame_list_create(&fl);
	if(err < 0) {
		DEBUG_WARN("Could not create frame list for WooFMsgPut");
		return(-1);
	}

	// tag is first
	const char *t_str = std::to_string(WOOF_MSG_PUT).c_str();
	err = cmq_frame_create(&f,(unsigned char *)t_str,strlen(t_str)+1);
	if(err < 0) {
		DEBUG_WARN("Could not create tag frame for WooFMsgPut");
		cmq_frame_list_destroy(fl);
		return(-1);
	}

	err = cmq_frame_append(fl,f);
	if(err < 0) {
		DEBUG_WARN("Could not append tag frame for WooFMsgPut");
		cmq_frame_list_destroy(fl);
		cmq_frame_destroy(f);
		return(-1);
	}

	const char *w_str = std::string(woof_name).c_str();
	err = cmq_frame_create(&f,(unsigned char *)w_str,strlen(w_str)+1);
	if(err < 0) {
		DEBUG_WARN("Could not create woof name frame for WooFMsgPut");
		cmq_frame_list_destroy(fl);
		return(-1);
	}

	err = cmq_frame_append(fl,f);
	if(err < 0) {
		DEBUG_WARN("Could not append woof name frame for WooFMsgPut");
		cmq_frame_list_destroy(fl);
		cmq_frame_destroy(f);
		return(-1);
	}

	// send NULL as place holder when no handler name
	const char *h_str;
	if(handler_name == NULL) {
	    h_str = "NULL";
	} else {
	    h_str = handler_name;
	}

	err = cmq_frame_create(&f,(unsigned char *)h_str,strlen(h_str)+1);
	if(err < 0) {
		DEBUG_WARN("Could not create handler name frame for WooFMsgPut");
		cmq_frame_list_destroy(fl);
		return(-1);
	}

	err = cmq_frame_append(fl,f);
	if(err < 0) {
		DEBUG_WARN("Could not append handler name frame for WooFMsgPut");
		cmq_frame_list_destroy(fl);
		cmq_frame_destroy(f);
		return(-1);
	}

	err = cmq_frame_create(&f,(unsigned char *)elem, elem_size);
	if(err < 0) {
		DEBUG_WARN("Could not create element frame for WooFMsgPut");
		cmq_frame_list_destroy(fl);
		return(-1);
	}

	err = cmq_frame_append(fl,f);
	if(err < 0) {
		DEBUG_WARN("Could not append element frame for WooFMsgPut");
		cmq_frame_list_destroy(fl);
		cmq_frame_destroy(f);
		return(-1);
	}

	std::string ip_str = ip.value();
	const char *c_ip_str = ip_str.c_str();
	std::string port_str = port.value();
	sd = cmq_pkt_connect((char *)c_ip_str, stoi(port_str), WOOF_MSG_REQ_TIMEOUT);
	if(sd < 0) {
		DEBUG_WARN("Could not connect to server for WooFMsgPut with timeout %d ms\n",
				WOOF_MSG_REQ_TIMEOUT);
		perror("MooFMsgPut: connect failed");
		cmq_frame_list_destroy(fl);
		return(-1);
	}
	err = cmq_pkt_send_msg(sd,fl);
	if(err < 0) {
		DEBUG_WARN("Could not send msg for WooFMsgPut");
		cmq_frame_list_destroy(fl);
		close(sd);
		return(-1);
	}

	err = cmq_pkt_recv_msg(sd,&r_fl);
	if(err < 0) {
		DEBUG_WARN("Could not recv msg for WooFMsgPut response");
		close(sd);
		return(-1);
	}
	
	close(sd);
	err = cmq_frame_pop(r_fl,&r_f);
	if(err < 0) {
		DEBUG_WARN("Could not pop frame for WooFMsgPut response");
		cmq_frame_list_destroy(r_fl);
		return(-1);
	}
	cmq_frame_list_destroy(r_fl);

	// response should be seq_no
	int32_t seq_no = strtoul((char *)cmq_frame_payload(r_f),NULL,10);
	cmq_frame_destroy(r_f);

    	return seq_no;
}

int32_t backend::remote_get_elem_size(std::string_view woof_name_v) {
	std::string woof_name(woof_name_v);
	auto ip = ip_from_woof(woof_name);
	auto port = port_from_woof(woof_name);
	int err;
	int sd;
	unsigned char *fl;
	unsigned char *f;
	unsigned char *r_fl;
	unsigned char *r_f;

	if(!ip) {
	    return(-1);
	}
	if(!port) {
	    return(-1);
	}

	DEBUG_LOG("WooFMsgGetElSize: woof: %s trying enpoint\n", woof_name.c_str());
	err = cmq_frame_list_create(&fl);
	if(err < 0) {
        	DEBUG_WARN("Could not create message for GetElSize for %s", woof_name.c_str());
		return(-1);
	}

	// tag is first
	const char *t_str = std::to_string(WOOF_MSG_GET_EL_SIZE).c_str();
	err = cmq_frame_create(&f,(unsigned char *)t_str,strlen(t_str)+1);
	if(err < 0) {
		DEBUG_WARN("Could not create tag frame for GetElSize for %s", woof_name.c_str());
		cmq_frame_list_destroy(fl);
		return(-1);
	}
	err = cmq_frame_append(fl,f);
	if(err < 0) {
		DEBUG_WARN("Could not append tag frame for GetElSize for %s", woof_name.c_str());
		cmq_frame_list_destroy(fl);
		cmq_frame_destroy(f);
		return(-1);
	}

	// woof name next
	const char *w_str = woof_name.c_str();
	err = cmq_frame_create(&f,(unsigned char *)w_str,strlen(w_str)+1);
	if(err < 0) {
		DEBUG_WARN("Could not create woof name frame for GetElSize for %s", woof_name.c_str());
		cmq_frame_list_destroy(fl);
		return(-1);
	}
	err = cmq_frame_append(fl,f);
	if(err < 0) {
		DEBUG_WARN("Could not append woof name frame for GetElSize for %s", woof_name.c_str());
		cmq_frame_list_destroy(fl);
		cmq_frame_destroy(f);
		return(-1);
	}

	std::string ip_str = ip.value();
	const char *c_ip_str = ip_str.c_str();
	std::string port_str = port.value();
	sd = cmq_pkt_connect((char *)c_ip_str, stoi(port_str), WOOF_MSG_REQ_TIMEOUT);
	if(sd < 0) {
		DEBUG_WARN("Could not connect to server for GetElSize with timeout %d ms\n",
				WOOF_MSG_REQ_TIMEOUT);
		perror("MooFMsgPut: connect failed");
		cmq_frame_list_destroy(fl);
		return(-1);
	}
	err = cmq_pkt_send_msg(sd,fl);
	if(err < 0) {
		DEBUG_WARN("Could not send msg for GetElSize");
		cmq_frame_list_destroy(fl);
		close(sd);
		return(-1);
	}
	cmq_frame_list_destroy(fl);

	err = cmq_pkt_recv_msg(sd,&r_fl);
	if(err < 0) {
		DEBUG_WARN("Could not recv msg for GetElSize response");
        	perror("WooFMsgGetElSize");
		close(sd);
		return(-1);
	}
	close(sd);

	err = cmq_frame_pop(r_fl,&r_f);
	if(err < 0) {
		DEBUG_WARN("Could not pop frame for GetElSize response");
		cmq_frame_list_destroy(r_fl);
		return(-1);
	}
	 cmq_frame_list_destroy(r_fl);

        // response should be seq_no
        int32_t seq_no = strtoul((char *)cmq_frame_payload(r_f),NULL,10);
        cmq_frame_destroy(r_f);

        return seq_no;

}

int32_t backend::remote_get_latest_seq_no(std::string_view woof_name_v,
                                          const char* cause_woof_name,
                                          uint32_t cause_woof_latest_seq_no) {
	std::string woof_name(woof_name_v);
	auto ip = ip_from_woof(woof_name);
	auto port = port_from_woof(woof_name);
	int err;
	int sd;
	unsigned char *fl;
	unsigned char *f;
	unsigned char *r_fl;
	unsigned char *r_f;

	if(!ip) {
	    return(-1);
	}
	if(!port) {
	    return(-1);
	}

	DEBUG_LOG("WooFMsgGetLatestSeqno: woof: %s trying enpoint\n", woof_name.c_str());
	err = cmq_frame_list_create(&fl);
	if(err < 0) {
        	DEBUG_WARN("Could not create message for GetLatestSeqno for %s %s %d", 
				woof_name.c_str(),
				cause_woof_name,
				(int)cause_woof_latest_seq_no);
        	printf("Could not create message for GetLatestSeqno for %s %s %d", 
				woof_name.c_str(),
				cause_woof_name,
				(int)cause_woof_latest_seq_no);
		return(-1);
	}

	// tag is first
	const char *t_str = std::to_string(WOOF_MSG_GET_LATEST_SEQNO).c_str();
	err = cmq_frame_create(&f,(unsigned char *)t_str,strlen(t_str)+1);
	if(err < 0) {
		DEBUG_WARN("Could not create tag frame for GetLatestSeqno for %s", woof_name.c_str());
		cmq_frame_list_destroy(fl);
		return(-1);
	}
	err = cmq_frame_append(fl,f);
	if(err < 0) {
		DEBUG_WARN("Could not append tag frame for GetLatestSeqno for %s", woof_name.c_str());
		cmq_frame_list_destroy(fl);
		cmq_frame_destroy(f);
		return(-1);
	}

	// woof name next
	const char *w_str = woof_name.c_str();
	err = cmq_frame_create(&f,(unsigned char *)w_str,strlen(w_str)+1);
	if(err < 0) {
		DEBUG_WARN("Could not create woof name frame for GetLatestSeqno for %s", woof_name.c_str());
		cmq_frame_list_destroy(fl);
		return(-1);
	}
	err = cmq_frame_append(fl,f);
	if(err < 0) {
		DEBUG_WARN("Could not append woof name frame for GetLatestSeqno for %s", woof_name.c_str());
		cmq_frame_list_destroy(fl);
		cmq_frame_destroy(f);
		return(-1);
	}

	std::string ip_str = ip.value();
	const char *c_ip_str = ip_str.c_str();
	std::string port_str = port.value();
	sd = cmq_pkt_connect((char *)c_ip_str, stoi(port_str), WOOF_MSG_REQ_TIMEOUT);
	if(sd < 0) {
		DEBUG_WARN("Could not connect to server for GetElSize with timeout %d ms\n",
				WOOF_MSG_REQ_TIMEOUT);
		perror("MooFMsgPut: connect failed");
		cmq_frame_list_destroy(fl);
		return(-1);
	}
	err = cmq_pkt_send_msg(sd,fl);
	if(err < 0) {
		DEBUG_WARN("Could not send msg for LatestSeqno");
		cmq_frame_list_destroy(fl);
		close(sd);
		return(-1);
	}
	cmq_frame_list_destroy(fl);

	err = cmq_pkt_recv_msg(sd,&r_fl);
	if(err < 0) {
		DEBUG_WARN("Could not recv msg for LatestSeqno response");
        	perror("WooFMsgGetElSize");
		close(sd);
		return(-1);
	}
	close(sd);

	err = cmq_frame_pop(r_fl,&r_f);
	if(err < 0) {
		DEBUG_WARN("Could not pop frame for LatestSeqno response");
		cmq_frame_list_destroy(r_fl);
		return(-1);
	}
	 cmq_frame_list_destroy(r_fl);

        // response should be seq_no
        int32_t seq_no = strtoul((char *)cmq_frame_payload(r_f),NULL,10);
        cmq_frame_destroy(r_f);

        return seq_no;

}
} // namespace cspot::cmq
