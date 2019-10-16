//
// Created by fatih on 10/19/18.
//

#pragma once

#include "common.hpp"
#include "emsha_signer.hpp"

#include <algorithm>
#include <cstring>
#include <initializer_list>
#include <memory>
#include <new>
#include <tos/streams.hpp>

namespace caps {
template<class CapabilityT, class CryptoModelT>
auto sign(const caps::token<CapabilityT, CryptoModelT>& c,
          typename CryptoModelT::signer_type& s) -> typename CryptoModelT::sign_t;

template<class CapabilityT, class HasherT>
auto hash(const caps::cap_list<CapabilityT>& c, HasherT& h) -> typename HasherT::hash_t;

template<class CapabilityT, class CryptoModelT>
bool verify(const caps::token<CapabilityT, CryptoModelT>& c,
            typename CryptoModelT::signer_type& signer);

template<class CapabilityT, class SatisfyCheckerT>
bool validate(const caps::cap_list<CapabilityT>& haystack,
              const CapabilityT& needle,
              SatisfyCheckerT&& satisfies);

template<class CapabilityT, class CryptoModelT, class SatisfyCheckerT>
bool validate(const caps::token<CapabilityT, CryptoModelT>& haystack,
              const CapabilityT& needle,
              SatisfyCheckerT&& satisfies) {
    return validate(haystack.c, needle, std::forward<SatisfyCheckerT>(satisfies));
}

template<class CapabilityT, class CryptoModelT>
inline void attach(caps::token<CapabilityT, CryptoModelT>& c,
                   list_ptr<CapabilityT> child,
                   const typename CryptoModelT::hasher_type& hasher);
} // namespace caps

namespace caps {
template<class CapabilityT, class SatisfyCheckerT>
bool validate(const caps::cap_list<CapabilityT>& haystack,
              const CapabilityT& needle,
              SatisfyCheckerT&& satisfies) {
    auto* leaf = &haystack;
    for (; leaf->child; leaf = leaf->child.get())
        ; // find the end of the hmac chain

    return std::any_of(begin(*leaf), end(*leaf), [&needle, &satisfies](auto& cap) {
        return satisfies(cap, needle);
    });
}

template<class CapabilityT, class CryptoModelT>
auto sign(const caps::token<CapabilityT, CryptoModelT>& c,
          typename CryptoModelT::signer_type& s) -> typename CryptoModelT::sign_type {
    typename CryptoModelT::sign_type res{};
    auto beg = (const uint8_t*)c.c.all;
    size_t sz = sizeof(CapabilityT) * c.c.num_caps;
    s.sign({beg, sz}, res.buf);
    return res;
}

template<class CapabilityT, class HasherT>
auto hash(const caps::cap_list<CapabilityT>& c, const HasherT& h) -> typename HasherT::hash_t {
    const auto beg = (const uint8_t*)c.all;
    const auto sz = sizeof(CapabilityT) * c.num_caps;
    return h.hash({beg, sz});
}

template<class CapabilityT>
cap_list<CapabilityT>* get_leaf_cap(caps::cap_list<CapabilityT>& c) {
    auto it = &c;
    while (it->child) {
        it = it->child.get();
    }
    return it;
}

template<class CapabilityT, class CryptoModelT>
void attach(caps::token<CapabilityT, CryptoModelT>& c,
            list_ptr<CapabilityT> child,
            const typename CryptoModelT::hasher_type& hasher) {
    // TODO: do verification here
    auto child_hash = hash(*child, hasher);
    merge_into(c.signature, child_hash);
    get_leaf_cap(c.c)->child = std::move(child);
}

template<class CapabilityT, class CryptoModelT>
auto get_signature(const caps::token<CapabilityT, CryptoModelT>& c, typename CryptoModelT::signer_type& s) {
    auto signature = sign(c, s);

    for (auto child = c.c.child.get(); child; child = child->child.get()) {
        auto child_hash = hash(*child, typename CryptoModelT::hasher_type{});
        merge_into(signature, child_hash);
    }

    return signature;
}

template<class CapabilityT, class CryptoModelT>
bool verify(const caps::token<CapabilityT, CryptoModelT>& c,
            typename CryptoModelT::signer_type& signer) {
    return get_signature(c, signer) == c.signature;
}

template<class CapabilityT, class CryptoModelT>
auto get_req_sign(const caps::token<CapabilityT, CryptoModelT>& c,
                  const typename CryptoModelT::hasher_type& hasher,
                  const uint64_t& seq,
                  const typename CryptoModelT::hash_type& req_hash) ->
    typename CryptoModelT::sign_type {
    auto sign = c.signature;

    auto h = hasher.hash(seq);

    merge_into(sign, h);

    merge_into(sign, req_hash);

    return sign;
}

template<class CapabilityT, class CryptoModelT>
inline bool verify(const caps::token<CapabilityT, CryptoModelT>& c,
                   typename CryptoModelT::signer_type& s,
                   const uint64_t& seq,
                   const typename CryptoModelT::hash_type& req_hash) {
    auto sign = get_signature(c, s);

    auto h = typename CryptoModelT::hasher_type().hash(seq);

    merge_into(sign, h);

    merge_into(sign, req_hash);

    return sign == c.signature;
}

template<class CapabilityT>
list_ptr<CapabilityT> mkcaps(std::initializer_list<CapabilityT> capabs) {
    auto mem = new char[sizeof(caps::cap_list<CapabilityT>) +
                        sizeof(CapabilityT) * capabs.size()];
    auto cps = new (mem) caps::cap_list<CapabilityT>;
    int i = 0;
    for (auto& c : capabs) {
        new (cps->all + i++) CapabilityT(c);
    }
    cps->num_caps = i;
    cps->child = nullptr;
    return list_ptr<CapabilityT>(cps);
}

template<class CryptoModelT, class CapabilityT>
token_ptr<CapabilityT, CryptoModelT> mkcaps(std::initializer_list<CapabilityT> capabs,
                                            typename CryptoModelT::signer_type& s) {
    auto mem = new char[sizeof(token<CapabilityT, CryptoModelT>) +
                        sizeof(CapabilityT) * capabs.size()];
    auto cps = new (mem) token<CapabilityT, CryptoModelT>;
    int i = 0;
    for (auto& c : capabs) {
        new (cps->c.all + i++) CapabilityT(c);
    }
    cps->c.num_caps = i;
    cps->c.child = nullptr;
    cps->signature = sign(*cps, s);
    return token_ptr<CapabilityT, CryptoModelT>{cps};
}
} // namespace caps