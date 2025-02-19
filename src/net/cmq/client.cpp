#include "backend.hpp"
#include "common.hpp"

#include <cstring>
#include <debug.h>
#include <global.h>

extern "C" {
#include <cmq_pkt.h>
}

namespace cspot::cmq {
int32_t backend::remote_get(std::string_view woof_name, void* elem, uint32_t elem_size, uint32_t seq_no) {
    auto ip = ip_from_woof(woof_name);
    auto port = port_from_woof(woof_name);
    std::string woof_n(woof_name);
    int sd;
    int err;

    if (!ip) {
        return -1;
    }
    if (!port) {
        return -1;
    }

    if (elem_size == (uint32_t)-1) {
        return (-1);
    }

    auto msg = CreateMessage(std::to_string(WOOF_MSG_GET),
                             std::string(woof_name),
                             std::to_string(seq_no)
                             );

    sd = cmq_pkt_connect(ip.c_str(), stoi(port));
    if(sd < 0) {
        DEBUG_WARN("Could not connect to server for WoofMsgGet");
	printf("WooFMsgGet: server connect failed to %s:%s\n",
		ip.c_str(),port.c_str());
	cmq_frame_list_destroy(msg);
	return -1;
    }

    err = cmq_pkt_send_msg(sd,msg);
    if(err < 0) {
        DEBUG_WARN("Could not send to server for WoofMsgGet");
	printf("WooFMsgGet: server request send failed to %s:%s\n",
		ip.c_str(),
		port.c_str());
	cmq_frame_list_destroy(msg);
        return -1;
    }

    cmq_frame_list_destroy(msg);

    unsigned int *r_msg;
    err = cmq_pkt_recv_msg(sd,&r_msg);
    if(err < 0) {
        DEBUG_WARN("Could not receive reply for WoofMsgGet");
	printf("WooFMsgGet: server request recv failed from %s:%s\n",
		ip.c_str(),port.c_str());
	perror("WooFMsgGet");
        return -1;
    }

    if(cmq_frame_list_empty(r_msg)) {
        DEBUG_WARN("Empty receive reply for WoofMsgGet");
	printf("WooFMsgGet: empty recv from %s:%s\n",
		ip.c_str(),port.c_str());
	perror("WooFMsgGet");
	cmq_frame_list_destroy(r_msg);
        return -1;
    }

    unsigned char *frame;
    err = cmq_frame_pop(r_msg);
    if(err < 0) {
        DEBUG_WARN("msg pop failed receive reply for WoofMsgGet");
	printf("WooFMsgGet: pop failed for recv from %s:%s\n",
		ip.c_str(),port.c_str());
	perror("WooFMsgGet");
	cmq_frame_list_destroy(r_msg);
        return -1;
    }

    cmq_frame_list_destroy(r_msg);

    if((cmq_frame_payload(frame) == NULL) ||
	(cmq_frame_size(frame) == 0))
    {
        DEBUG_WARN("empty frame receive reply for WoofMsgGet");
	printf("WooFMsgGet: empty frame for recv from %s:%s\n",
		ip.c_str(),port.c_str());
	perror("WooFMsgGet");
	cmq_frame_destroy(frame);
        return -1;
    }

    std::memcpy(elem, cmq_frame_payload(frame), cmq_frame_size(frame));

    cmq_frame_destroy(frame);
    return 1;
}

#if 0
int32_t backend::remote_get_tail(std::string_view woof_name, void* elements, unsigned long el_size, int el_count) {
    auto endpoint_opt = endpoint_from_woof(woof_name);

    if (!endpoint_opt) {
        return -1;
    }

    auto& endpoint = *endpoint_opt;

    if (el_size == (unsigned long)-1) {
        return (-1);
    }

    auto msg = CreateMessage(std::to_string(WOOF_MSG_GET_TAIL), std::string(woof_name), std::to_string(el_count));

    auto r_msg = ZMsgPtr(ServerRequest(endpoint.c_str(), std::move(msg)));

    if (!r_msg) {
        DEBUG_WARN("Could not receive reply for WooFMsgGetTail");
        return -1;
    }

    auto res = ExtractMessage<std::string, std::vector<uint8_t>>(*r_msg);
    if (!res) {
        return -1;
    }

    auto& [len_str, vec] = *res;

    auto len = std::stoul(len_str);
    if (vec.size() != el_size * len) {
        DEBUG_WARN("WooFMsgGetTail received a different element size than supplied, %d != %d!",
                   int(vec.size() / len),
                   int(el_size));
        return -1;
    }

    len = std::min<uint32_t>(len, el_count);

    std::memcpy(elements, vec.data(), len * el_size);
    return 1;
}

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
