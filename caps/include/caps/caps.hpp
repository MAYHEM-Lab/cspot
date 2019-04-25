//
// Created by fatih on 10/19/18.
//

#pragma once

#include "common.hpp"
#include "emsha_signer.hpp"
#include <string.h>
#include <initializer_list>
#include <new>
#include <memory>
#include <algorithm>
#include <tos/streams.hpp>

namespace caps
{
    template <class CapabilityT, class SignerT>
    auto sign(const caps::token<CapabilityT, SignerT> &c, SignerT &s) -> typename SignerT::sign_t;

    template <class CapabilityT, class SignerT>
    auto hash(const caps::cap_list<CapabilityT> &c, SignerT& h) -> typename SignerT::hash_t;

    template <class CapabilityT, class SignerT>
    bool verify(const caps::token<CapabilityT, SignerT> &c, SignerT &s);

    template <class CapabilityT, class SatisfyCheckerT>
    bool validate(
            const caps::cap_list<CapabilityT>& haystack,
            const CapabilityT& needle,
            SatisfyCheckerT&& satisfies);

    template <class CapabilityT, class SignerT, class SatisfyCheckerT>
    bool validate(
            const caps::token<CapabilityT, SignerT>& haystack,
            const CapabilityT& needle,
            SatisfyCheckerT&& satisfies)
    {
        return validate(haystack.c, needle, std::forward<SatisfyCheckerT>(satisfies));
    }
}

namespace caps
{
    template<class CapabilityT, class SatisfyCheckerT>
    bool validate(
            const caps::cap_list<CapabilityT> &haystack,
            const CapabilityT &needle,
            SatisfyCheckerT &&satisfies) {
        auto* leaf = &haystack;
        for (; leaf->child; leaf = leaf->child.get()); // find the end of the hmac chain

        return std::any_of(begin(*leaf), end(*leaf), [&needle, &satisfies](auto& cap){
            return satisfies(cap, needle);
        });
    }

    template <class CapabilityT, class SignerT>
    auto sign(const caps::token<CapabilityT, SignerT> &c, SignerT &s) -> typename SignerT::sign_t
    {
        typename SignerT::sign_t res{};
        auto beg = (const uint8_t *) c.c.all;
        size_t sz = sizeof(CapabilityT) * c.c.num_caps;
        s.sign({beg, sz}, res.buf);
        return res;
    }

    template <class CapabilityT, class Hasher>
    auto hash(const caps::cap_list<CapabilityT> &c, Hasher& h) -> typename Hasher::hash_t
    {
        const auto beg = (const uint8_t *) c.all;
        const auto sz = sizeof(CapabilityT) * c.num_caps;
        return h.hash({beg, sz});
    }

    template <class CapabilityT>
    inline cap_list<CapabilityT>* get_leaf_cap(caps::cap_list<CapabilityT>& c)
    {
        auto it = &c;
        while (it->child)
        {
            it = it->child.get();
        }
        return it;
    }

    template <class CapabilityT, class SignerT>
    inline void attach(caps::token<CapabilityT, SignerT> &c, list_ptr<CapabilityT> child, SignerT& s)
    {
        //TODO: do verification here
        auto child_hash = hash(*child, s);
        merge_into(c.signature, child_hash);
        get_leaf_cap(c.c)->child = std::move(child);
    }

    template <class CapabilityT, class SignerT>
    inline auto get_signature(const caps::token<CapabilityT, SignerT> &c, SignerT &s)
    {
        auto signature = sign(c, s);

        for (auto child = c.c.child.get(); child; child = child->child.get())
        {
            auto child_hash = hash(*child, s);
            merge_into(signature, child_hash);
        }

        return signature;
    }

    template <class CapabilityT, class SignerT>
    inline bool verify(const caps::token<CapabilityT, SignerT> &c, SignerT &s)
    {
        return get_signature(c, s) == c.signature;
    }

    template <class CapabilityT, class SignerT>
    inline typename SignerT::sign_t get_req_sign(const caps::token<CapabilityT, SignerT> &c, SignerT &s,
                       const uint64_t& seq, const typename SignerT::hash_t& req_hash)
    {
        auto sign = c.signature;

        auto h = s.hash(seq);

        merge_into(sign, h);

        merge_into(sign, req_hash);

        return sign;
    }

    template <class CapabilityT, class SignerT>
    inline bool verify(const caps::token<CapabilityT, SignerT> &c, SignerT &s,
            const uint64_t& seq, const typename SignerT::hash_t& req_hash)
    {
        auto sign = get_signature(c, s);

        auto h = s.hash(seq);

        merge_into(sign, h);

        merge_into(sign, req_hash);

        return sign == c.signature;
    }

    template <class CapabilityT>
    list_ptr <CapabilityT> mkcaps(std::initializer_list<CapabilityT> capabs)
    {
        auto mem = new char[sizeof(caps::cap_list<CapabilityT>) + sizeof(CapabilityT) * capabs.size()];
        auto cps = new(mem) caps::cap_list<CapabilityT>;
        int i = 0;
        for (auto &c : capabs) {
            new(cps->all + i++) CapabilityT(c);
        }
        cps->num_caps = i;
        cps->child = nullptr;
        return list_ptr<CapabilityT>(cps);
    }

    template <class CapabilityT, class SignerT>
    token_ptr<CapabilityT, SignerT>
    mkcaps(std::initializer_list<CapabilityT> capabs, SignerT &s)
    {
        auto mem = new char[sizeof(token<CapabilityT, SignerT>) + sizeof(CapabilityT) * capabs.size()];
        auto cps = new(mem) token<CapabilityT, SignerT>;
        int i = 0;
        for (auto &c : capabs) {
            new(cps->c.all + i++) CapabilityT(c);
        }
        cps->c.num_caps = i;
        cps->c.child = nullptr;
        cps->signature = sign(*cps, s);
        return token_ptr<CapabilityT, SignerT>{cps};
    }
}