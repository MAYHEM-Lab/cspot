#include "common.hpp"
#include "backend.hpp"

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
        return -1;
    }

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

    if((cmq_frame_payload(frame) == NULL) ||
	(cmq_frame_size(frame) == 0))
    {
        DEBUG_WARN("empty frame receive reply for WoofMsgGet");
	printf("WooFMsgGet: empty frame for recv from %s:%d\n",
		c_ip_str,stoi(port_str));
	perror("WooFMsgGet");
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

    const char *els = std::to_string(el_size).c_str();
    err = cmq_frame_create(&f,(unsigned char *)els,strlen(els)+1);
    if(err < 0) {
        DEBUG_WARN("Could not connect create el_szie for WoofMsgGetTail");
	printf("WooFMsgGetTail: could not create el_size\n");
	cmq_frame_list_destroy(fl);
	return -1;
    }
    err = cmq_frame_append(fl,f);
    if(err < 0) {
        DEBUG_WARN("Could not connect append el_size for WoofMsgGetTail");
	printf("WooFMsgGet: could not append el_size\n");
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
        return -1;
    }

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

    unsigned long el_cnt = strtol((char *)cmq_frame_payload(frame),NULL,10);
    cmq_frame_destroy(frame);

    // second frame is elements
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
	printf("WooFMsgGetTail: size match failed for recv from %d, %d, %d\n",
		cmq_frame_size(frame),el_count,el_size);
    }
    int len = cmq_frame_size(frame);
    if(len < (el_count*el_size)) {
	    len = el_count*el_size;
    }

    std::memcpy(elements, cmq_frame_payload(frame), len);

    cmq_frame_destroy(frame);
    return 1;

}
#if 0

int32_t
backend::remote_put(std::string_view woof_name, const char* handler_name, const void* elem, uint32_t elem_size) {
    auto endpoint_opt = endpoint_from_woof(woof_name);

    if (!endpoint_opt) {
        return -1;
    }

    auto& endpoint = *endpoint_opt;

    if (elem_size == (uint32_t)-1) {
        return (-1);
    }

    unsigned long my_log_seq_no;
    if (auto namelog_seq_no = getenv("WOOF_NAMELOG_SEQNO")) {
        my_log_seq_no = strtoul(namelog_seq_no, nullptr, 10);
    } else {
        my_log_seq_no = 0;
    }

    auto elem_ptr = static_cast<const uint8_t*>(elem);
    auto msg = CreateMessage(std::to_string(WOOF_MSG_PUT),
                             std::string(woof_name),
                             handler_name,
                            //  std::to_string(Name_id),
                            //  std::to_string(my_log_seq_no),
                             std::vector<uint8_t>(elem_ptr, elem_ptr + elem_size));

    if (!msg) {
        DEBUG_WARN("Could not create message for WooFMsgPut for %s", woof_name);
        return -1;
    }

    auto r_msg = ZMsgPtr(ServerRequest(endpoint.c_str(), std::move(msg)));

    if (!r_msg) {
        DEBUG_WARN("Could not receive reply for WoofMsgPut");
	printf("WooFPut: server request failed");
	perror("WooFMsgPut");
        return -1;
    }

    auto res = ExtractMessage<std::string>(*r_msg);
    if (!res) {
        return -1;
    }

    auto& [str] = *res;
    return std::stoul(str, nullptr, 10);
}

int32_t backend::remote_get_elem_size(std::string_view woof_name_v) {
    std::string woof_name(woof_name_v);
    auto endpoint_opt = endpoint_from_woof(woof_name);

    if (!endpoint_opt) {
        return -1;
    }

    auto& endpoint = *endpoint_opt;

    DEBUG_LOG("WooFMsgGetElSize: woof: %s trying enpoint %s\n", woof_name.c_str(), endpoint.c_str());

    auto msg = CreateMessage(std::to_string(WOOF_MSG_GET_EL_SIZE), std::string(woof_name));

    if (!msg) {
        DEBUG_WARN("Could not create message for GetElSize for %s", woof_name.c_str());
        return -1;
    }

    auto r_msg = ZMsgPtr(ServerRequest(endpoint.c_str(), std::move(msg)));

    if (!r_msg) {
        fprintf(stderr,
                "WooFMsgGetElSize: woof: %s couldn't recv msg for element size from "
                "server at %s\n",
                woof_name.c_str(),
                endpoint.c_str());
	printf("WooFMsgGetElSize: server request failed\n");
        perror("WooFMsgGetElSize");
        return -1;
    }

    auto res = ExtractMessage<std::string>(*r_msg);
    if (!res) {
        return -1;
    }

    auto& [str] = *res;
    return std::stoul(str, nullptr, 10);
}

int32_t backend::remote_get_latest_seq_no(std::string_view woof_name,
                                          const char* cause_woof_name,
                                          uint32_t cause_woof_latest_seq_no) {

    std::string woof_n(woof_name);
    auto endpoint_opt = endpoint_from_woof(woof_name);

    if (!endpoint_opt) {
        return -1;
    }

    auto& endpoint = *endpoint_opt;

    unsigned long my_log_seq_no;
    if (auto namelog_seq_no = getenv("WOOF_NAMELOG_SEQNO")) {
        my_log_seq_no = strtoul(namelog_seq_no, nullptr, 10);
    } else {
        my_log_seq_no = 0;
    }

    auto msg = CreateMessage(std::to_string(WOOF_MSG_GET_LATEST_SEQNO),
                             std::string(woof_name)
                            //  ,std::to_string(Name_id),
                            //  std::to_string(my_log_seq_no),
                            //  cause_woof_name,
                            //  std::to_string(cause_woof_latest_seq_no)
                             );

    auto r_msg = ZMsgPtr(ServerRequest(endpoint.c_str(), std::move(msg)));

    if (!r_msg) {
        DEBUG_WARN("Could not receive reply for WoofMsgGet");
	printf("WooFMsgGetLatestSeqno: server request failed\n");
	perror("WooFMsgGetLatestSeqno");
        return -1;
    }

    auto res = ExtractMessage<std::string>(*r_msg);
    if (!res) {
        return -1;
    }

    auto& [str] = *res;
    return std::stoul(str);
}
#endif
} // namespace cspot::zmq
