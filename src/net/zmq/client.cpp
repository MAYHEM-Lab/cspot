#include "backend.hpp"
#include "common.hpp"

#include <cstring>
#include <debug.h>
#include <global.h>
#include "woofc-caplets.h"

namespace cspot::zmq {
int CapFile(char *capfile, int size)
{
	char *home_dir;
	struct stat file_stat; 
	char file_path[1024];
	int found = 0;
	memset(capfile,0,size);

	// for caplets
	// check local first
	// must be user read but otherwise not accessible
	if(access("./capabilities.yaml", R_OK) == 0) {
		if(stat("./capabilities.yaml",&file_stat) == 0) {
			if((file_stat.st_mode & S_IRUSR) != 0) {
				  if((file_stat.st_mode & 
				      (S_IRGRP | S_IWGRP | S_IXGRP |
				       S_IROTH | S_IWOTH | S_IROTH)) == 0) {
					  strncpy(capfile,"./capabilities.yaml",size-1);
					  found = 1;
				  }
			}
		}
	}

	home_dir = getenv("HOME");
	if(home_dir != NULL) {
		memset(file_path,0,sizeof(file_path));
		snprintf(file_path,sizeof(file_path),"%s/.cspot/capabilities.yaml",home_dir);
		if(access(file_path, R_OK) == 0) {
			if(stat(file_path,&file_stat) == 0) {
				if((file_stat.st_mode & S_IRUSR) != 0) {
					  if((file_stat.st_mode & 
					      (S_IRGRP | S_IWGRP | S_IXGRP |
					       S_IROTH | S_IWOTH | S_IROTH)) == 0) {
						  strncpy(capfile,file_path,size-1);
						  found = 1;
					  }
				}
			}
		}
	}


	return(found);
}

int32_t backend::remote_get(std::string_view woof_name, void* elem, uint32_t elem_size, uint32_t seq_no) {
    auto endpoint_opt = endpoint_from_woof(woof_name);
    std::string woof_n(woof_name);
    char cap_file[1024];
    int has_cap;
    WCAP cap;
    WCAP *new_cap;

    if (!endpoint_opt) {
        return -1;
    }

    has_cap = CapFile(cap_file,sizeof(cap_file));

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
    ZMsgPtr msg;
    if(has_cap == 1) {
	if(SearchKeychain(cap_file,(char *)std::string(woof_name).c_str(),&cap) >= 0) {
		// attenuate down to read only
		new_cap = WooFCapAttenuate(&cap,WCAP_READ);
		if(new_cap != NULL) {
			auto cap_ptr = reinterpret_cast<const uint8_t*>(new_cap);
			msg = CreateMessage(std::to_string(WOOF_MSG_GET_CAP),
			     std::vector<uint8_t>(cap_ptr,cap_ptr + sizeof(cap)),
                             std::string(woof_name),
                             std::to_string(seq_no)
                            //  ,
                            //  std::to_string(Name_id),
                            //  std::to_string(my_log_seq_no)
                             );
			free(new_cap);
			if(!msg) {
				has_cap = 0;
			}
		} else {
	    		DEBUG_WARN("WooFMsgGet: cap attenuate failed for %s %s\n",
					cap_file,(char *)std::string(woof_name).c_str());
			has_cap = 0;
		}
	} else {
	    	DEBUG_WARN("WooFMsgGet: cap earch failed for %s %s\n",
					cap_file,(char *)std::string(woof_name).c_str());
		has_cap = 0;
	}
    }


    // backward compatibility
    if((has_cap == 0) && (!msg)) {
    	msg = CreateMessage(std::to_string(WOOF_MSG_GET),
                             std::string(woof_name),
                             std::to_string(seq_no)
                            //  ,
                            //  std::to_string(Name_id),
                            //  std::to_string(my_log_seq_no)
                             );
    }
    if(!msg) {
	    DEBUG_WARN("WooFMsgGet: could not create msg\n");
	    return(-1);
    }

    auto r_msg = ZMsgPtr(ServerRequest(endpoint.c_str(), std::move(msg)));

    if (!r_msg) {
        DEBUG_WARN("Could not receive reply for WoofMsgGet");
	printf("WooFMsgGet: server request failed\n");
	perror("WooFMsgGet");
        return -1;
    }

    auto res = ExtractMessage<std::vector<uint8_t>>(*r_msg);
    if (!res) {
        return -1;
    }

    auto& [vec] = *res;
    if (vec.size() != elem_size) {
        DEBUG_WARN(
            "WooFMsgGet received a different element size than supplied, %d != %d!", int(vec.size()), int(elem_size));
        return -1;
    }

    std::memcpy(elem, vec.data(), vec.size());
    return 1;
}

int32_t backend::remote_get_tail(std::string_view woof_name, void* elements, unsigned long el_size, int el_count) {
    auto endpoint_opt = endpoint_from_woof(woof_name);
    char cap_file[1024];
    int has_cap;
    WCAP cap;
    WCAP *new_cap;

    if (!endpoint_opt) {
        return -1;
    }

    has_cap = CapFile(cap_file,sizeof(cap_file));

    auto& endpoint = *endpoint_opt;

    if (el_size == (unsigned long)-1) {
        return (-1);
    }


    ZMsgPtr msg;
    if(has_cap == 1) {
	if(SearchKeychain(cap_file,(char *)std::string(woof_name).c_str(),&cap) >= 0) {
		// attenuate down to read only
		new_cap = WooFCapAttenuate(&cap,WCAP_READ);
		if(new_cap != NULL) {
			auto cap_ptr = reinterpret_cast<const uint8_t*>(new_cap);
    			msg = CreateMessage(std::to_string(WOOF_MSG_GET_TAIL_CAP), 
					std::vector<uint8_t>(cap_ptr,cap_ptr + sizeof(cap)),
					std::string(woof_name), 
					std::to_string(el_count));
			free(new_cap);
			if(!msg) {
				has_cap = 0;
			}
		} else {
	    		DEBUG_WARN("WooFMsgGetTail: cap attenuate failed for %s %s\n",
					cap_file,(char *)std::string(woof_name).c_str());
			has_cap = 0;
		}
	} else {
	    	DEBUG_WARN("WooFMsgGetTail: cap search failed for %s %s\n",
					cap_file,(char *)std::string(woof_name).c_str());
		has_cap = 0;
	}
    }
    //
    // backward compatibility
    if((has_cap == 0) && (!msg)) {
    	msg = CreateMessage(std::to_string(WOOF_MSG_GET_TAIL), 
					std::string(woof_name), 
					std::to_string(el_count));
    }

    if(!msg) {
	    DEBUG_WARN("WooFMsgGetTail: could not create msg\n");
	    return(-1);
    }
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
    char cap_file[1024];
    int has_cap;
    WCAP cap;
    WCAP *new_cap;

    if (!endpoint_opt) {
        return -1;
    }

    auto& endpoint = *endpoint_opt;

    if (elem_size == (uint32_t)-1) {
        return (-1);
    }

    has_cap = CapFile(cap_file,sizeof(cap_file));


    unsigned long my_log_seq_no;
    if (auto namelog_seq_no = getenv("WOOF_NAMELOG_SEQNO")) {
        my_log_seq_no = strtoul(namelog_seq_no, nullptr, 10);
    } else {
        my_log_seq_no = 0;
    }

    auto elem_ptr = static_cast<const uint8_t*>(elem);
    ZMsgPtr msg;
    if(has_cap == 1) {
	if(SearchKeychain(cap_file,(char *)std::string(woof_name).c_str(),&cap) >= 0) {
//WooFCapPrint((char *)std::string(woof_name).c_str(),&cap);
		if(handler_name == NULL) {
			new_cap = WooFCapAttenuate(&cap,WCAP_WRITE); // drop privs if we can
		} else {
			new_cap = WooFCapAttenuate(&cap,WCAP_EXEC); // drop privs if we can
		}
		if(new_cap != NULL) {
			auto cap_ptr = reinterpret_cast<const uint8_t*>(new_cap);
			msg = CreateMessage(std::to_string(WOOF_MSG_PUT_CAP),
				     std::vector<uint8_t>(cap_ptr,cap_ptr + sizeof(cap)),
				     std::string(woof_name),
				     handler_name,
				    //  std::to_string(Name_id),
				    //  std::to_string(my_log_seq_no),
				     std::vector<uint8_t>(elem_ptr, elem_ptr + elem_size));
			free(new_cap);
		} else {
			has_cap = 0;
		}
		
	} else {
		has_cap = 0;
	}
    }


    // backwards compatibility: if we can't find a cap, try without one
    if((has_cap == 0) && (!msg)) { 
    	msg = CreateMessage(std::to_string(WOOF_MSG_PUT),
                             std::string(woof_name),
                             handler_name,
                            //  std::to_string(Name_id),
                            //  std::to_string(my_log_seq_no),
                             std::vector<uint8_t>(elem_ptr, elem_ptr + elem_size));
    }

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
    char cap_file[1024];
    int has_cap;
    WCAP cap;
    WCAP *new_cap;

    if (!endpoint_opt) {
        return -1;
    }

    has_cap = CapFile(cap_file,sizeof(cap_file));

    auto& endpoint = *endpoint_opt;

    DEBUG_LOG("WooFMsgGetElSize: woof: %s trying enpoint %s, has_cap: %d\n", woof_name.c_str(), 
		    endpoint.c_str(), has_cap);

    ZMsgPtr msg;
    if(has_cap == 1) {
	if(SearchKeychain(cap_file,(char *)std::string(woof_name).c_str(),&cap) >= 0) {
		// attenuate down to read only
		new_cap = WooFCapAttenuate(&cap,WCAP_READ);
		if(new_cap != NULL) {
			auto cap_ptr = reinterpret_cast<const uint8_t*>(new_cap);
    			msg = CreateMessage(std::to_string(WOOF_MSG_GET_EL_SIZE_CAP), 
					std::vector<uint8_t>(cap_ptr,cap_ptr + sizeof(cap)),
					std::string(woof_name));
			free(new_cap);
			if(!msg) {
				has_cap = 0;
			}
		} else {
	    		DEBUG_WARN("WooFMsgGetElSize: cap attenuate failed for %s %s\n",
					cap_file,(char *)std::string(woof_name).c_str());
			has_cap = 0;
		}
	} else {
	    	DEBUG_WARN("WooFMsgGetElSize: cap search failed for %s %s\n",
					cap_file,(char *)std::string(woof_name).c_str());
		has_cap = 0;
	}
    }

    // backward compatibility
    if((has_cap == 0) && (!msg)) {
    	msg = CreateMessage(std::to_string(WOOF_MSG_GET_EL_SIZE), std::string(woof_name));
    }

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
    char cap_file[1024];
    int has_cap;
    WCAP cap;
    WCAP *new_cap;

    if (!endpoint_opt) {
        return -1;
    }

    has_cap = CapFile(cap_file,sizeof(cap_file));

    auto& endpoint = *endpoint_opt;

    unsigned long my_log_seq_no;
    if (auto namelog_seq_no = getenv("WOOF_NAMELOG_SEQNO")) {
        my_log_seq_no = strtoul(namelog_seq_no, nullptr, 10);
    } else {
        my_log_seq_no = 0;
    }

    DEBUG_LOG("WooFGetLatestSeqno: called on %s, has_cap: %d\n",std::string(woof_name).c_str(),has_cap);

    ZMsgPtr msg;
    if(has_cap == 1) {
	if(SearchKeychain(cap_file,(char *)std::string(woof_name).c_str(),&cap) >= 0) {
		// attenuate down to read only
		new_cap = WooFCapAttenuate(&cap,WCAP_READ);
		if(new_cap != NULL) {
			auto cap_ptr = reinterpret_cast<const uint8_t*>(new_cap);
    			msg = CreateMessage(std::to_string(WOOF_MSG_GET_LATEST_SEQNO_CAP), 
					std::vector<uint8_t>(cap_ptr,cap_ptr + sizeof(cap)),
					std::string(woof_name)); 
			free(new_cap);
			if(!msg) {
				has_cap = 0;
			}
		} else {
	    		DEBUG_WARN("WooFMsgGetLatestSeqno: cap attenuate failed for %s %s\n",
					cap_file,(char *)std::string(woof_name).c_str());
			has_cap = 0;
		}
	} else {
	    	DEBUG_WARN("WooFMsgLatestSeqno: cap search failed for %s %s\n",
					cap_file,(char *)std::string(woof_name).c_str());
		has_cap = 0;
	}
    }

    // backward compatibility
    if((has_cap == 0) && (!msg)) {
    	msg = CreateMessage(std::to_string(WOOF_MSG_GET_LATEST_SEQNO),
                             std::string(woof_name)
                            //  ,std::to_string(Name_id),
                            //  std::to_string(my_log_seq_no),
                            //  cause_woof_name,
                            //  std::to_string(cause_woof_latest_seq_no)
                             );
    }

    if(!msg) {
	    DEBUG_WARN("WooFMsgGetLatestSeqno: could not create msg\n");
	    return(-1);
    }
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
} // namespace cspot::zmq
