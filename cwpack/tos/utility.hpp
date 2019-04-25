//
// Created by Mehmet Fatih BAKIR on 25/03/2018.
//
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <chrono>
#include <utility>

namespace tos {
    template<class T> using invoke_t = typename T::type;

    template<class S1, class S2>
    struct concat;

    template<class T, T... I1, T... I2>
    struct concat<std::integer_sequence<T, I1...>, std::integer_sequence<T, I2...>>
    : std::integer_sequence<T, I1..., (sizeof...(I1)+I2)...>
    {
    };

    template<class S1, class S2>
    using concat_t = invoke_t<concat<S1, S2>>;

    template <class AlarmT, class FunT>
    void forever(AlarmT& alarm, std::chrono::milliseconds ms, FunT&& fun)
    {
        while (true)
        {
            std::forward<FunT>(fun)();
            alarm.sleep_for(ms);
        }
    }

    struct non_copyable
    {
        non_copyable() = default;
        non_copyable(const non_copyable&) = delete;
        non_copyable(non_copyable&&) = default;
        non_copyable& operator=(const non_copyable&) = delete;
        non_copyable& operator=(non_copyable&&) = default;
        ~non_copyable() = default;
    };

    struct non_movable
    {
        non_movable() = default;
        non_movable(const non_movable&) = default;
        non_movable(non_movable&&) = delete;
        non_movable& operator=(const non_movable&) = default;
        non_movable& operator=(non_movable&&) = delete;
        ~non_movable() = default;
    };

    struct non_copy_movable : non_copyable, non_movable {};

} // namespace tos

