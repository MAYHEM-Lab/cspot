//
// Created by fatih on 8/22/18.
//

#pragma once

#include <common/driver_base.hpp>
#include "span.hpp"
#include "expected.hpp"

namespace tos
{
    class omemory_stream : public self_pointing<omemory_stream>
    {
    public:
        explicit constexpr omemory_stream(tos::span<char> buf) : m_buffer{buf}, m_wr_it{m_buffer.begin()} {}
        explicit constexpr omemory_stream(tos::span<uint8_t> buf) : omemory_stream({(char*)buf.data(), buf.size()}) {}

        constexpr size_t write(tos::span<const char> buf){
            auto buf_it = buf.begin();
            while (m_wr_it != m_buffer.end() && buf_it != buf.end())
            {
                *m_wr_it++ = *buf_it++;
            }
            return buf_it - buf.begin();
        }

        constexpr tos::span<const char> get() {
            return m_buffer.slice(0, m_wr_it - m_buffer.begin());
        }

    private:
        tos::span<char> m_buffer;
        char* m_wr_it;
    };

    class imemory_stream : public self_pointing<imemory_stream>
    {
    public:
        explicit constexpr imemory_stream(tos::span<const char> buf) : m_buffer{buf}, m_it{m_buffer.begin()} {}
        explicit constexpr imemory_stream(tos::span<const uint8_t> buf)
        : imemory_stream({(const char*)buf.data(), buf.size()}) {}

        tos::expected<tos::span<char>, int> read(tos::span<char> buf)
        {
            auto len = std::min<ptrdiff_t>(buf.size(), std::distance(m_it, m_buffer.end()));
            // std::copy is not constexpr, thus we can't use it
            // this is std::copy(m_it, m_it + len, buf.begin())
            auto buf_it = buf.begin();
            for (int i = 0; i < len; ++i)
            {
                *buf_it++ = *m_it++;
            }
            return buf.slice(0, len);
        }

    private:
        tos::span<const char> m_buffer;
        tos::span<const char>::iterator m_it;
    };
}