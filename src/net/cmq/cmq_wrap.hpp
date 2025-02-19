#pragma once
    
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <debug.h>

#include <cmq-frame.h>

namespace cspot::cmq {

inline unsigned char*FrameFrom(const char* str) {
    if (!str) {
        return cmq_frame_create(NULL);
    }

    return cmq_frame_create(str, strlen(str) + 1);
}


template<class... DataTs>
unsigned char *CreateMessage(const DataTs&... data) {
    auto ptr = cmq_frame_list_create();

    if(!ptr) {
	return(nullptr);
    }

    auto append_one = [&](const auto& elem) {
        auto frame = FrameFrom(elem);
        if (!frame) {
            return nullptr;
        }
        cmq_frame_append(ptr, frame);
        return ptr;
    };

    auto res = (append_one(data) && ...);
    if (res) {
        return ptr;
    }
    return nullptr;
}

} // cspot::cmq
