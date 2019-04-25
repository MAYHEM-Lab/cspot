/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 K. Isom <coder@kyleisom.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * copy of this  software and associated documentation  files (the "Software"),
 * to deal  in the Software  without restriction, including  without limitation
 * the rights  to use,  copy, modify,  merge, publish,  distribute, sublicense,
 * and/or  sell copies  of the  Software,  and to  permit persons  to whom  the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS  PROVIDED "AS IS", WITHOUT WARRANTY OF  ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING  BUT NOT  LIMITED TO  THE WARRANTIES  OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS  OR COPYRIGHT  HOLDERS BE  LIABLE FOR  ANY CLAIM,  DAMAGES OR  OTHER
 * LIABILITY,  WHETHER IN  AN ACTION  OF CONTRACT,  TORT OR  OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#pragma once

#include <cstdint>

#define EMSHA_CHECK(condition, retval) if (!(condition)) { return (retval); }

namespace emsha {

    // SHA256_HASH_SIZE is the output length of SHA-256 in bytes.
    constexpr uint32_t SHA256_HASH_SIZE = 32;

    // The EMSHA_RESULT type is used to indicate whether an
    // operation succeeded, and if not, what the general fault type
    // was.
    enum EMSHA_RESULT : uint8_t
    {
        // All operations have completed successfully so far.
        EMSHA_ROK = 0,

        // A self test or unit test failed.
        EMSHA_TEST_FAILURE = 1,

        // A null pointer was passed in as a buffer where it shouldn't
        // have been.
        EMSHA_NULLPTR = 2,

        // The Hash is in an invalid state.
        EMSHA_INVALID_STATE = 3,

        // The input to SHA256::update is too large.
        SHA256_INPUT_TOO_LONG = 4,

        // The self tests have been disabled, but a self-test function
        // was called.
        EMSHA_SELFTEST_DISABLED = 5
    };

    bool hash_equal(const uint8_t* a, const uint8_t* b);
#ifndef EMSHA_NO_HEXSTRING
    void hexstring(uint8_t *dest, uint8_t *src, uint32_t srclen);
#endif
}

