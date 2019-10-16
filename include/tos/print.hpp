//
// Created by Mehmet Fatih BAKIR on 25/03/2018.
//

#pragma once

#include <stdint.h>
#include <tos/utility.hpp>
#include <string.h>
#include <tos/span.hpp>

namespace tos
{
    inline const char* itoa(int64_t i, int base) {
        static char intbuf[std::numeric_limits<decltype(i)>::digits10 + 1];
        static constexpr char lookup[] = {\
            '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
            'A', 'B', 'C', 'D', 'E', 'F'
        };
        decltype(i) j = 0, isneg = 0;

        if (i == 0) {
            return "0";
        }

        if (i < 0) {
            isneg = 1;
            i = -i;
        }

        while (i != 0) {
            intbuf[j++] = lookup[(i % base)];
            i /= base;
        }

        if (isneg)
            intbuf[j++] = '-';

        intbuf[j] = '\0';
        j--;
        i = 0;
        while (i < j) {
            isneg = intbuf[i];
            intbuf[i] = intbuf[j];
            intbuf[j] = isneg;
            i++;
            j--;
        }

        return intbuf;
    }
}

namespace tos
{
    template <class CharOstreamT>
    void print(CharOstreamT& ostr, char c) {
        ostr->write({&c, 1});
    }

    template <class CharOstreamT>
    void print(CharOstreamT& ostr, uint8_t b) {
        print(ostr, char(b));
    }

    /**
     * Prints the given null terminated string to the output stream
     * @param ostr The output stream
     * @param str A null terminated string
     */
    template <class CharOstreamT>
    void print(CharOstreamT & ostr, const char *str) {
        auto len = strlen(str);
        if (len == 0) return;
        ostr->write({str, len});
    }

    template <class CharOstreamT>
    void print(CharOstreamT& ostr, span<const char> buf)
    {
        ostr->write(buf);
    }

    template <class CharOstreamT,
            class T = int16_t,
            typename = std::enable_if_t<!std::is_same<T, int>{}>>
    void print(CharOstreamT & ostr, int16_t x) {
        print(ostr, itoa(x, 10));
    }

    template <class CharOstreamT,
            class T = int32_t,
            typename = std::enable_if_t<!std::is_same<T, int>{}>>
    void print(CharOstreamT & ostr, int32_t x) {
        print(ostr, itoa(x, 10));
    }

    template <class CharOstreamT>
    void print(CharOstreamT & ostr, int x) {
        print(ostr, itoa(x, 10));
    }

    template <class CharOstreamT>
    void print(CharOstreamT & ostr, uintptr_t ptr) {
        //print(ostr, '0');
        //print(ostr, 'x');
        print(ostr, itoa(ptr, 16));
    }

    template <class CharOstreamT>
    void print(CharOstreamT & ostr, void *p) {
        print(ostr, reinterpret_cast<uintptr_t>(p));
    }

    template <class CharOstreamT>
    void print(CharOstreamT &ostr, bool b) {
        print(ostr, b ? "true" : "false");
    }

    /**
     * Prints the given arguments with the given separator in between
     *
     * To prevent overload ambiguities, this function takes 2 regular arguments
     * and then a variadic pack of arguments
     *
     * @tparam T1 Type of the first parameter
     * @tparam T2 Type of the second parameter
     * @tparam Ts Types of the rest of the parameters
     * @param ostr Stream to print the output to
     * @param t1 First parameter
     * @param t2 Second parameter
     * @param ts Rest of the parameters
     * @param sep Separator
     */
    template<class CharOstreamT, class T1, class T2, class... Ts>
    void print(CharOstreamT& ostr, T1&& t1, T2&& t2, Ts&&... ts)
    {
        constexpr auto sep = ' ';
        print(ostr, std::forward<T1>(t1));
        print(ostr, sep);
        print(ostr, std::forward<T2>(t2));

        //((print(ostr, sep), print(ostr, forward<Ts>(ts))), ...);
        int _[] = { 0, (print(ostr, sep), print(ostr, std::forward<Ts>(ts)), 0)... };
        (void)_;
    }

    template <class CharOstreamT>
    void println(CharOstreamT& ostr)
    {
        print(ostr, "\r\n");
    }

    template <class CharOstreamT, class... T>
    void println(CharOstreamT& ostr, T&&... t)
    {
        print(ostr, std::forward<T>(t)...);
        print(ostr, "\r\n");
    }
}

