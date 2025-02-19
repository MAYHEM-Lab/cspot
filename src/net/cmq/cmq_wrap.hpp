#pragma once
    
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <debug.h>

#include <string.h>
#include <cmq-frame.h>

namespace cspot::cmq {

inline unsigned char*FrameFrom(const char* str) {
    int err;
    unsigned char *f;


    if (!str) {
	err = cmq_frame_create(&f,NULL,0);
	if(err < 0) {
		return NULL;
	} else {
		return f;
	}
    }
    err = cmq_frame_create(&f, (unsigned char *)str, strlen(str) + 1);
    if(err < 0) {
	    return NULL;
    } else {
    	return f;
    }
}


#if 0
template<class... DataTs>
unsigned char *CreateMessage(const DataTs&... data) {
    int err;
    unsigned char *fl;

    err = cmq_frame_list_create(&fl);
    if(err < 0) {
	    return NULL;
    }

    auto append_one = [&](const auto& elem) {
        auto frame = FrameFrom(elem.c_str());
        if (!frame) {
            return NULL;
        }
        cmq_frame_append(fl, frame);
        return fl;
    };

    auto res = (append_one(data) && ...);
    if (res) {
        return fl;
    }
    return NULL;
}
#endif

} // cspot::cmq
