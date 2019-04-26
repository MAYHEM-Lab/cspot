//
// Created by fatih on 10/22/18.
//

#pragma once

#include <cppspot/common.hpp>

namespace cspot {
    constexpr struct root_cap_t {} root_cap;

    enum class perms : uint8_t {
        get = 1,
        put = 2,
        root = 4
    };

    struct cap_t {
        union {
            char none;
            namespace_id_t ns_id;
            woof_id_t woof_id;
        };

        perms p;
        enum class types : uint8_t
        {
            nil,
            ns,
            woof,
            root
        } type;

        constexpr explicit cap_t() : p{}, type{types::nil}, none{0} {}
        constexpr explicit cap_t(woof_id_t id, perms pp) : p{pp}, type{types::woof}, woof_id{id} {}
        constexpr explicit cap_t(namespace_id_t id, perms pp) : p{pp}, type{types::ns}, ns_id{id} {}
        constexpr explicit cap_t(root_cap_t, perms pp) : p{pp}, type{types::root}, none{} {}

        constexpr cap_t(const cap_t&) = default;

        constexpr cap_t&operator=(const cap_t&) = default;
    };

    static_assert(sizeof(cap_t) == 8, "bad cap size");
}

namespace cspot {
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
