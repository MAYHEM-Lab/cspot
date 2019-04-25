//
// Created by fatih on 10/22/18.
//

#pragma once

#include <cppspot/common.hpp>
#include <tos/print.hpp>

namespace cspot {
    constexpr struct root_cap_t {} root_cap;

    enum class perms : uint8_t {
        get = 1,
        put = 2,
        root = 4
    };

    struct cap_t {
        perms p : 3;
        enum class types : uint8_t
        {
            nil,
            ns,
            woof,
            root
        } type : 2;

        union {
            char none;
            namespace_id_t ns_id;
            woof_id_t woof_id;
        };

        constexpr explicit cap_t() : p{}, type{types::nil}, none{0} {}
        constexpr explicit cap_t(woof_id_t id, perms pp) : p{pp}, type{types::woof}, woof_id{id} {}
        constexpr explicit cap_t(namespace_id_t id, perms pp) : p{pp}, type{types::ns}, ns_id{id} {}
        constexpr explicit cap_t(root_cap_t, perms pp) : p{pp}, type{types::root}, none{} {}

        constexpr cap_t(const cap_t&) = default;

        constexpr cap_t&operator=(const cap_t&) = default;
    };
}

namespace cspot {
    template <class StreamT>
    void print(StreamT& str, const cap_t& id)
    {
        switch (id.type)
        {
            case cap_t::types::nil: tos::print(str, "nil"); break;
            case cap_t::types::ns: tos::print(str, "ns", int(id.ns_id.ns_id)); break;
            case cap_t::types::woof: tos::print(str, "woof", int(id.woof_id.woof_id)); break;
            case cap_t::types::root: tos::print(str, "root"); break;
        }
    }

    inline constexpr bool operator==(const cap_t& a, const cap_t& b)
    {
        if (a.type != b.type) return false;
        switch (a.type)
        {
            case cap_t::types::ns: return a.ns_id == b.ns_id;
            case cap_t::types::woof: return a.woof_id == b.woof_id;

            case cap_t::types::nil:
            case cap_t::types::root:
                return true;
        }
        __builtin_unreachable();
    }

    inline constexpr perms operator&(perms a, perms b) { return perms((uint8_t)a & (uint8_t)b); }
    inline constexpr perms operator|(perms a, perms b) { return perms((uint8_t)a | (uint8_t)b); }

    template <class ObjT>
    constexpr cap_t mkcap(ObjT obj, perms p)
    {
        return cap_t{ obj, p };
    }

    constexpr bool is_subobject(const cap_t& haystack, const cap_t& needle)
    {
        return false;
    }

    inline bool satisfies(const cap_t &haystack, const cap_t &needle)
    {
        tos_debug_print("searching for %d %d %d in %d %d %d\n"
                , int(needle.type), int(needle.woof_id.woof_id), int(needle.p)
                , int(haystack.type), int(haystack.woof_id.woof_id), int (haystack.p));
        if (haystack.type == cap_t::types::root) return true;
        if ((perms(haystack.p) & perms(needle.p)) != perms(needle.p)) return false;
        if (needle == haystack)
        {
            return true;
        }
        else
        {
            return is_subobject(haystack, needle);
        }
    }
}
