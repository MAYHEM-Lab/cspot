//
// Created by fatih on 1/8/19.
//

#pragma once

#include "common.hpp"
#include <type_traits>
#include <tos/streams.hpp>

namespace caps
{
    template <class StreamT, class SignerT, class CapabilityT>
    void serialize(StreamT& to, const token<CapabilityT, SignerT>& caps);

    template <class CapabilityT, class SignerT, class StreamT>
    auto deserialize(StreamT& from);
}

namespace caps
{
    namespace detail
    {
        template <class SignT, class StreamT>
        void serialize(StreamT& to, const SignT& signature)
        {
            static_assert(std::is_trivially_copyable<SignT>{}, "signature must be a pod");
            to->write({ (const char*)&signature, sizeof signature });
        }

        template <class StreamT, class CapabilityT>
        void serialize(StreamT& to, const cap_list<CapabilityT>& list)
        {
            to->write({ (const char*)&list.num_caps, sizeof list.num_caps });
            for (auto& cap : list)
            {
                static_assert(std::is_trivially_copyable<std::remove_reference_t<decltype(cap)>>{}, "capabilities must be pods");
                to->write({ (const char*)&cap, sizeof cap });
            }
        }

        template <class SignT, class StreamT>
        SignT deserialize_sign(StreamT& from)
        {
            static_assert(std::is_trivially_copyable<SignT>{}, "signature must be a pod");

            SignT signature;
            tos::read_to_end(from, { (char*)&signature, sizeof signature });
            return signature;
        }

        template <class CapabilityT, class StreamT>
        void read_list(StreamT& from, cap_list<CapabilityT>& to, int16_t len)
        {
            for (int i = 0; i < len; ++i)
            {
                tos::read_to_end(from, { (char*)&to.all[i], sizeof to.all[i] });
            }
            to.num_caps = len;
        }

        template <class CapabilityT, class SignerT, class StreamT>
        token_ptr<CapabilityT, SignerT>
        deserialize_root(StreamT& from)
        {
            decltype(cap_list<CapabilityT>::num_caps) len;
            tos::read_to_end(from, { (char*)&len, sizeof len });
            tos_debug_print("len: %d\n", int(len));
            auto mem = new char[sizeof(token<CapabilityT, SignerT>) + sizeof(CapabilityT) * len];
            auto cps = new(mem) token<CapabilityT, SignerT>;
            read_list(from, cps->c, len);
            return token_ptr<CapabilityT, SignerT>{cps};
        }

        template <class CapabilityT, class StreamT>
        list_ptr<CapabilityT> deserialize_list(StreamT& from)
        {
            decltype(cap_list<CapabilityT>::num_caps) len;
            tos::read_to_end(from, { (char*)&len, sizeof len });
            if (len == 0) return nullptr;
            auto mem = new char[sizeof(caps::cap_list<CapabilityT>) + sizeof(CapabilityT) * len];
            auto cps = new(mem) caps::cap_list<CapabilityT>;
            read_list(from, *cps, len);
            return list_ptr<CapabilityT>{cps};
        }
    }

    template<class StreamT, class SignerT, class CapabilityT>
    void serialize(StreamT& to, const token<CapabilityT, SignerT>& caps)
    {
        for (auto child = &caps.c; child; child = child->child.get())
        {
            detail::serialize(to, *child);
        }
        decltype(caps.c.num_caps) c[] = {0};
        to->write({(const char*)&c, sizeof c});

        /*
         * The signature goes last into the wire
         *
         * If it went first, since the receiver couldn't know the size of the
         * overall object, it'd have to store the signature in a temporary location
         * probably on the stack. Signatures could be huge, so we prefer not to
         * store a temporary signature and then copy it.
         *
         * When the signature goes last, the receiver can actually construct the
         * whole capability chain and just copy the signature to the signature
         * field of the root. This way, we save stack space and avoid copying
         * large signatures.
         */
        detail::serialize(to, caps.signature);
    }

    template<class CapabilityT, class SignerT, class StreamT>
    auto deserialize(StreamT& from)
    {
        auto root = detail::deserialize_root<CapabilityT, SignerT>(from);
        auto tail = &root->c;
        for (
                auto l = detail::deserialize_list<CapabilityT>(from);
                l;
                l = detail::deserialize_list<CapabilityT>(from))
        {
            tail->child = std::move(l);
            tail = tail->child.get();
        }
        tail->child = nullptr;
        root->signature = detail::deserialize_sign<typename SignerT::sign_t>(from);
        return root;
    }
}