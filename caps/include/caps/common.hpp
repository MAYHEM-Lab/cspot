//
// Created by fatih on 5/17/18.
//

#pragma once

#include <cstdint>
#include <cstring>
#include <memory>

namespace caps {
struct raw_deleter {
    template<class T>
    void operator()(T* ptr) const {
        delete[](char*) ptr;
    }
};

template<class CapabilityT>
struct cap_list;

template<class CapT>
using list_ptr = std::unique_ptr<cap_list<CapT>, raw_deleter>;

/**
 * This struct template represents a single node in a capability derivation chain.
 * @tparam CapabilityT type of the capabilities stored in this node.
 */
template<class CapabilityT>
struct cap_list : tos::non_copy_movable {
    list_ptr<CapabilityT> child;
    int16_t num_caps;
    int16_t __padding;
    CapabilityT all[0];
};

template<class CapabilityT>
const CapabilityT* begin(const cap_list<CapabilityT>& cl) {
    return cl.all;
}

template<class CapabilityT>
const CapabilityT* end(const cap_list<CapabilityT>& cl) {
    return cl.all + cl.num_caps;
}

template<class T>
list_ptr<T> clone(const cap_list<T>& l) {
    auto mem = new char[sizeof(cap_list<T>) + l.num_caps * sizeof(T)];
    auto res = new (mem) cap_list<T>;
    int i = 0;
    for (auto& c : l) {
        new (res->all + i++) T(c);
    }
    res->num_caps = l.num_caps;
    res->child = l.child ? clone(*l.child) : nullptr;
    return list_ptr<T>(res);
}

/**
 * This struct template represents the root of a capability token.
 *
 * @tparam CapabilityT an application defined type for storing authorization information.
 * @tparam CryptoModelT the cryptographic model for signing and verifying signatures.
 */
template<class CapabilityT, class CryptoModelT>
struct token {
    using sign_t = typename CryptoModelT::sign_type;

    sign_t signature{};
    cap_list<CapabilityT> c;
};

template<class CapT, class CryptoModelT>
using token_ptr = std::unique_ptr<token<CapT, CryptoModelT>, raw_deleter>;

template<class CT, class ST>
token_ptr<CT, ST> clone(const token<CT, ST>& t) {
    auto mem = new char[sizeof(token<CT, ST>) + sizeof(CT) * t.c.num_caps];
    auto cps = new (mem) token<CT, ST>;
    int i = 0;
    for (auto& c : t.c) {
        new (cps->c.all + i++) CT(c);
    }
    cps->c.num_caps = t.c.num_caps;
    cps->signature = t.signature;
    cps->c.child = cps->c.child ? clone(*cps->c.child) : nullptr;
    return token_ptr<CT, ST>(cps);
}

/**
 * Returns the size of the given capability token in bytes.
 * Useful as the type uses a variable length array.
 */
template<class CapabilityT, class CryptoModelT>
constexpr size_t size_of(const token<CapabilityT, CryptoModelT>& cr) {
    return sizeof cr + cr.c.num_caps * sizeof(CapabilityT);
}
} // namespace caps