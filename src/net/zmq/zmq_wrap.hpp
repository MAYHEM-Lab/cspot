#pragma once

#include <czmq.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <debug.h>

namespace cspot::zmq {
template<class T, void (*Deleter)(T**)>
struct basic_deleter {
    void operator()(T* sock) {
        Deleter(&sock);
    }
};

using ZSockDeleter = basic_deleter<zsock_t, zsock_destroy>;
using ZServerPtr = std::unique_ptr<zsock_t, ZSockDeleter>;

using ZPollerDeleter = basic_deleter<zpoller_t, zpoller_destroy>;
using ZPollerPtr = std::unique_ptr<zpoller_t, ZPollerDeleter>;

using ZMsgDeleter = basic_deleter<zmsg_t, zmsg_destroy>;
using ZMsgPtr = std::unique_ptr<zmsg_t, ZMsgDeleter>;

using ZFrameDeleter = basic_deleter<zframe_t, zframe_destroy>;
using ZFramePtr = std::unique_ptr<zframe_t, ZFrameDeleter>;

using ZActorDeleter = basic_deleter<zactor_t, zactor_destroy>;
using ZActorPtr = std::unique_ptr<zactor_t, ZActorDeleter>;
using ZProxyPtr = std::unique_ptr<zproxy_t , ZActorDeleter>;

inline bool Send(ZMsgPtr msg, zsock_t& server) {
    // It'll be destroyed by zmsg_send
    auto ptr = msg.release();
    if (auto err = zmsg_send(&ptr, &server); err == 0) {
        // Sent successfully.
        return true;
    }
    return false;
}

inline ZMsgPtr Receive(zsock_t& server) {
    auto msg = zmsg_recv(&server);
    return ZMsgPtr(msg);
}

inline ZFramePtr FrameFrom(const char* str) {
    if (!str) {
        return ZFramePtr(zframe_new_empty());
    }

    return ZFramePtr (zframe_new(str, strlen(str) + 1));
}

inline ZFramePtr FrameFrom(const std::string& str) {
    return FrameFrom(str.c_str());
}

inline ZFramePtr FrameFrom(const std::vector<uint8_t>& data) {
    auto frame = zframe_new(data.data(), data.size());
    return ZFramePtr(frame);
}

template<class T>
std::optional<T> FromFrame(zframe_t& frame);

template<>
inline std::optional<std::string> FromFrame(zframe_t& frame) {
    auto str = reinterpret_cast<const char*>(zframe_data(&frame));
    return std::string(str, zframe_size(&frame));
}

template<>
inline std::optional<std::vector<uint8_t>> FromFrame(zframe_t& frame) {
    auto data = reinterpret_cast<const uint8_t*>(zframe_data(&frame));
    auto size = zframe_size(&frame);
    return std::vector<uint8_t>(data, data + size);
}

inline ZMsgPtr NewMsg() {
    return ZMsgPtr(zmsg_new());
}

inline ZFramePtr PopFront(zmsg_t& msg) {
    return ZFramePtr(zmsg_pop(&msg));
}

inline bool Append(zmsg_t& msg, ZFramePtr frame) {
    auto frame_ptr = frame.release();
    return zmsg_append(&msg, &frame_ptr) == 0;
}

template<class... DataTs>
ZMsgPtr CreateMessage(const DataTs&... data) {
    auto ptr = NewMsg();

    auto append_one = [&](const auto& elem) {
        auto frame = FrameFrom(elem);
        if (!frame) {
            return false;
        }
        Append(*ptr, std::move(frame));
        return true;
    };

    auto res = (append_one(data) && ...);
    if (res) {
        return ptr;
    }
    return nullptr;
}

template<class... DataTs, size_t... Is>
std::optional<std::tuple<DataTs...>> DoExtractMessage(const std::vector<zframe_t*>& frames,
                                                      std::index_sequence<Is...>) {
    std::tuple<std::optional<DataTs>...> all{FromFrame<DataTs>((*frames[Is]))...};

    if (!(std::get<Is>(all) && ...)) {
        return {};
    }

    return std::tuple<DataTs...>{std::move(*std::get<Is>(all))...};
}

template<class... DataTs>
std::optional<std::tuple<DataTs...>> ExtractMessage(zmsg_t& msg) {
    if (zmsg_size(&msg) != sizeof...(DataTs)) {
        return {};
    }

    std::vector<zframe_t*> all_frames;
    all_frames.reserve(sizeof...(DataTs));

    auto first = zmsg_first(&msg);
    while (first) {
        all_frames.push_back(first);
        first = zmsg_next(&msg);
    }

    return DoExtractMessage<DataTs...>(all_frames, std::make_index_sequence<sizeof...(DataTs)>{});
}
} // namespace cspot::zmq
