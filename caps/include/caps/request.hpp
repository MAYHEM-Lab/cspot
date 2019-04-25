//
// Created by fatih on 3/9/19.
//

#pragma once

#include <cstdint>
#include <utility>
#include <tos/span.hpp>
#include <tos/expected.hpp>
#include <cwpack.hpp>
#include <tos/mem_stream.hpp>
#include "raw_serdes.hpp"
#include "caps.hpp"

namespace caps
{
    enum class deser_errors
    {
        not_array,
        no_token,
        no_req,
        no_seq,
        bad_seq,
        bad_sign,
        bad_cap
    };

    template <class CapT, class SignerT, class ReqParser, class Satisfies>
    auto req_deserializer(SignerT& signer, ReqParser&& parse, Satisfies&& satisfies)
    {
        return [
                &signer,
                parse = std::forward<ReqParser>(parse),
                satisfies = std::forward<Satisfies>(satisfies),
                last_seq = uint64_t(0)]
                (tos::span<const char> req) mutable ->
                tos::expected<decltype(parse(std::declval<tos::span<const uint8_t>>())), deser_errors>
        {
            cw_unpack_context uc;
            cw_unpack_context_init(&uc, (void*)req.data(), req.size(), nullptr);

            cw_unpack_next(&uc);
            if (uc.item.type != CWP_ITEM_ARRAY)
            {
                return tos::unexpected(deser_errors::not_array);
            }

            cw_unpack_next(&uc);
            if (uc.item.type != CWP_ITEM_BIN)
            {
                return tos::unexpected(deser_errors::no_token);
            }
            
            tos::imemory_stream cap_str({ (const char*)uc.item.as.bin.start, (size_t)uc.item.as.bin.length });

            auto cps = caps::deserialize<CapT, SignerT>(cap_str);

            cw_unpack_next(&uc);
            if (uc.item.type != CWP_ITEM_BIN)
            {
                return tos::unexpected(deser_errors::no_req);
            }

            auto req_str = tos::span<const uint8_t>((uint8_t*)uc.item.as.bin.start, uc.item.as.bin.length);
            auto req_obj = parse(req_str);
            auto req_hash = signer.hash(req_str);

            cw_unpack_next(&uc);
            if (uc.item.type != CWP_ITEM_POSITIVE_INTEGER)
            {
                return tos::unexpected(deser_errors::no_seq);
            }

            uint64_t seq = uc.item.as.u64;

            if (seq <= last_seq)
            {
                return tos::unexpected(deser_errors::bad_seq);
            }

            auto res = verify(*cps, signer, seq, req_hash);

            if (!res)
            {
                return tos::unexpected(deser_errors::bad_sign);
            }

            const auto& needle = req_to_tok(req_obj);

            if (!validate(*cps, needle, satisfies))
            {
                return tos::unexpected(deser_errors::bad_cap);
            }

            last_seq = seq;

            return req_obj;
        };
    }
} // namespace caps