//
// Created by fatih on 5/24/18.
//

#pragma once

#include <stddef.h>

namespace tos
{
    template <class T>
    class span
    {
    public:
        using iterator = T*;

        span(T* base, size_t len) : m_base(base), m_len(len) {}

        template <size_t Sz>
        span(T (&arr)[Sz]) : m_base(arr), m_len(Sz) {}

        size_t size() const { return m_len; }

        T* data() { return m_base; }
        const T* data() const { return m_base; }

        T& operator[](size_t ind) {
            return m_base[ind];
        }

        const T& operator[](size_t ind) const {
            return m_base[ind];
        }

        T* begin() { return m_base; }
        T* end() { return m_base + m_len; }

        const T* begin() const { return m_base; }
        const T* end() const { return m_base + m_len; }

        operator span<const T>() const {
            return { m_base, m_len };
        }

        span slice(size_t begin, size_t len){
            return { m_base + begin, len };
        }

    private:
        T* m_base;
        size_t m_len;
    };
}