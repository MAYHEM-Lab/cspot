//
// Created by fatih on 10/19/18.
//

#pragma once

#include <cstring>
#include <emsha/hmac.hh>
#include <tos/span.hpp>
#include <tos/utility.hpp>

namespace caps {
namespace emsha {
struct hash_t {
    uint8_t buf[32];
};

inline bool operator==(const hash_t& h, const hash_t& b) {
    return memcmp(h.buf, b.buf, 32) == 0;
}

struct sign_t {
    uint8_t buf[32];
};

inline bool operator==(const sign_t& a, const sign_t& b) {
    return std::equal(a.buf, a.buf + 32, b.buf);
}

inline void merge_into(sign_t& s, const hash_t& h) {
    auto i = s.buf;
    auto i_end = s.buf + 32;
    auto j = h.buf;
    for (; i != i_end; ++i, ++j) {
        *i ^= *j;
    }
    ::emsha::sha256_digest(s.buf, 32, s.buf);
}

struct hasher {
    using hash_t = emsha::hash_t;
    hash_t hash(tos::span<const uint8_t> msg) const {
        hash_t h;
        ::emsha::sha256_digest(msg.data(), msg.size(), h.buf);
        return h;
    }

    hash_t hash(tos::span<const char> seq) const {
        return hash(tos::span<const uint8_t>{(const uint8_t*)seq.data(), seq.size()});
    }

    hash_t hash(const uint64_t& seq) const {
        return hash(tos::span<const uint8_t>{(const uint8_t*)&seq, sizeof seq});
    }
};

class signer : public tos::non_copy_movable {
public:
    using sign_t = emsha::sign_t;

    explicit signer(tos::span<const char> key)
        : m_hmac{(const uint8_t*)key.data(), uint32_t(key.size())} {
    }

    void sign(tos::span<const uint8_t> msg, tos::span<uint8_t> out) noexcept {
        m_hmac.update(msg.data(), msg.size());
        m_hmac.finalize(out.data());
        m_hmac.reset();
    }

    sign_t sign(tos::span<const uint8_t> msg) noexcept {
        sign_t signature;
        sign(msg, signature.buf);
        return signature;
    }

private:
    ::emsha::HMAC m_hmac;
};

struct model {
    using signer_type = signer;
    using hasher_type = hasher;

    using sign_type = signer_type::sign_t;
    using hash_type = hasher_type::hash_t;
};
} // namespace emsha
} // namespace caps