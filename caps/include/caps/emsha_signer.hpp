//
// Created by fatih on 10/19/18.
//

#pragma once

#include <emsha/hmac.hh>
#include <tos/span.hpp>
#include <cstring>

namespace caps
{
namespace emsha
{
    struct hash_t
    {
        uint8_t buf[32];
    };

    inline bool operator==(const hash_t& h, const hash_t& b)
    {
        return memcmp(h.buf, b.buf, 32) == 0;
    }

    struct sign_t
    {
        uint8_t buf[32];
    };

    inline bool operator==(const sign_t& a, const sign_t& b)
    {
        return std::equal(a.buf, a.buf + 32, b.buf);
    }

    inline void merge_into(sign_t& s, const hash_t& h)
    {
        auto i = s.buf;
        auto i_end = s.buf + 32;
        auto j = h.buf;
        for (; i != i_end; ++i, ++j)
        {
            *i ^= *j;
        }
        ::emsha::sha256_digest(s.buf, 32, s.buf);
    }

    class signer {
    public:
        using sign_t = emsha::sign_t;
        using hash_t = emsha::hash_t;

        explicit signer(tos::span<const char> key)
                : m_hmac{(const uint8_t *)key.data(), uint32_t(key.size())} {}

        signer(const signer &) = delete;

        void sign(tos::span<const uint8_t> msg, tos::span<uint8_t> out) noexcept
        {
            m_hmac.update(msg.data(), msg.size());
            m_hmac.finalize(out.data());
            m_hmac.reset();
        }

        sign_t sign(tos::span<const uint8_t> msg) noexcept
        {
            sign_t signature;
            sign(msg, signature.buf);
            return signature;
        }

        hash_t hash(tos::span<const uint8_t> msg)
        {
            hash_t h;
            ::emsha::sha256_digest(msg.data(), msg.size(), h.buf);
            return h;
        }

        hash_t hash(tos::span<const char> seq)
        {
            return hash(tos::span<const uint8_t>{ (const uint8_t*)seq.data(), seq.size() });
        }

        hash_t hash(const uint64_t& seq)
        {
            return hash(tos::span<const uint8_t>{ (const uint8_t*)&seq, sizeof seq });
        }

    private:
        ::emsha::HMAC m_hmac;
    };
}
}