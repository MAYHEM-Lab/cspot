//
// Created by fatih on 6/30/18.
//

#pragma once

#include <stdint.h>
#include <string.h>
#include <tos/span.hpp>
#include <algorithm>

namespace tos {
    /**
     * This class represents an IP port number
     */
    struct port_num_t {
        uint16_t port;
    };

    struct alignas(std::uint32_t) ipv4_addr_t {
        uint8_t addr[4];
    };

    struct mac_addr_t {
        uint8_t addr[6];
    };

    inline bool operator==(const port_num_t &a, const port_num_t &b) {
        return a.port == b.port;
    }

    inline bool operator!=(const port_num_t &a, const port_num_t &b) {
        return a.port != b.port;
    }

    inline bool operator==(const ipv4_addr_t &a, const ipv4_addr_t &b) {
        return memcmp(a.addr, b.addr, 4) == 0;
    }

    inline bool operator!=(const ipv4_addr_t &a, const ipv4_addr_t &b) {
        return memcmp(a.addr, b.addr, 4) != 0;
    }

    inline ipv4_addr_t parse_ip(tos::span<const char> addr);

    struct udp_endpoint_t
    {
        ipv4_addr_t addr;
        port_num_t port;
    };
}

namespace tos{
    inline int atoi(tos::span<const char> chars)
    {
        int res = 0;
        for (auto p = chars.begin(); p != chars.end() && *p != 0; ++p)
        {
            res = res * 10 + *p - '0';
        }
        return res;
    }

    inline ipv4_addr_t parse_ip(tos::span<const char> addr) {
        ipv4_addr_t res;

        auto it = addr.begin();
        auto end = it;
        while (end != addr.end() && *end != '.') ++end;
        res.addr[0] = atoi({it, end});

        it = ++end;
        end = it;
        while (end != addr.end() && *end != '.') ++end;
        res.addr[1] = atoi({it, end});

        it = ++end;
        end = it;
        while (end != addr.end() && *end != '.') ++end;
        res.addr[2] = atoi({it, end});

        it = ++end;
        end = it;
        while (end != addr.end() && *end != '.') ++end;
        res.addr[3] = atoi({it, end});

        return res;
    }
}