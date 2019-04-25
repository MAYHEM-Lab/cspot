//
// Created by fatih on 10/22/18.
//

#pragma once

#include <tos/span.hpp>

namespace tos
{
    template <class StreamT>
    span<char> read_to_end(StreamT& str, span<char> buf)
    {
        auto b = buf;
        while (b.size() != 0)
        {
            auto rd = str->read(b);
            if (!rd)
            {
                break;
            }

            auto& r = force_get(rd);
            b = b.slice(r.size(), b.size() - r.size());
        }
        return buf.slice(0, buf.size() - b.size());
    }
} // namespace tos