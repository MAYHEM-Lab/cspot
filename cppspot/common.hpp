//
// Created by Mehmet Fatih BAKIR on 28/04/2018.
//

#pragma once

#include <stdint.h>

namespace cspot
{
    struct event_seq_t
    {
        uint16_t seq;
    };

    struct woof_seq_t
    {
        uint32_t seq;
    };

    constexpr bool operator!=(const woof_seq_t& a, const woof_seq_t& b)
    {
        return a.seq != b.seq;
    }

    struct woof_ind_t
    {
        uint32_t ind;
    };

    struct namespace_id_t
    {
        uint32_t ns_id;
    };

    constexpr bool operator==(const namespace_id_t& a, const namespace_id_t& b)
    {
        return a.ns_id == b.ns_id;
    }

    struct woof_id_t
    {
        uint16_t woof_id;
    };

    constexpr bool operator==(const woof_id_t& a, const woof_id_t& b)
    {
        return a.woof_id == b.woof_id;
    }
}
