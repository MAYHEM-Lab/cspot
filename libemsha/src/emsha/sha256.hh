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


#ifndef __EMSHA_SHA256_HH
#define __EMSHA_SHA256_HH

#include <stdint.h>

#include <emsha/emsha.hh>

namespace emsha {

    // SHA256_MB_SIZE is the size of a message block.
    const uint32_t SHA256_MB_SIZE = 64;

    class SHA256
    {
    public:
        // A SHA256 context does not need any special
        // construction. It can be declared and
        // immediately start being used.
        SHA256();

        // reset clears the internal state of the SHA256
        // context and returns it to its initial state.
        // It should always return EMSHA_ROK.
        EMSHA_RESULT reset();

        // update writes data into the context. While
        // there is an upper limit on the size of data
        // that SHA-256 can operate on, this package is
        // designed for small systems that will not
        // approach that level of data (which is on the
        // order of 2 exabytes), so it is not thought
        // to be a concern.
        //
        // Inputs:
        //     m: a byte array containing the message to
        //     be written. It must not be NULL (unless
        //     the message length is zero).
        //
        //     ml: the message length, in bytes.
        //
        // Outputs:
        //     EMSHA_NULLPTR is returned if m is NULL
        //     and ml is nonzero.
        //
        //     EMSHA_INVALID_STATE is returned if the
        //     update is called after a call to
        //     finalize.
        //
        //     SHA256_INPUT_TOO_LONG is returned if too
        //     much data has been written to the
        //     context.
        //
        //     EMSHA_ROK is returned if the data was
        //     successfully added to the SHA-256
        //     context.
        //
        EMSHA_RESULT update(const uint8_t* m, uint32_t ml);

        // finalize completes the digest. Once this
        // method is called, the context cannot be
        // updated unless the context is reset.
        //
        // Inputs:
        //     d: a byte buffer that must be at least
        //     SHA256.size() in length.
        //
        // Outputs:
        //     EMSHA_NULLPTR is returned if d is the
        //     null pointer.
        //
        //     EMSHA_INVALID_STATE is returned if the
        //     SHA-256 context is in an invalid state,
        //     such as if there were errors in previous
        //     updates.
        //
        //     EMSHA_ROK is returned if the context was
        //     successfully finalised and the digest
        //     copied to d.
        //
        EMSHA_RESULT finalize(uint8_t* d);

        // result copies the result from the SHA-256
        // context into the buffer pointed to by d,
        // running finalize if needed. Once called,
        // the context cannot be updated until the
        // context is reset.
        //
        // Inputs:
        //     d: a byte buffer that must be at least
        //     SHA256.size() in length.
        //
        // Outputs:
        //     EMSHA_NULLPTR is returned if d is the
        //     null pointer.
        //
        //     EMSHA_INVALID_STATE is returned if the
        //     SHA-256 context is in an invalid state,
        //     such as if there were errors in previous
        //     updates.
        //
        //     EMSHA_ROK is returned if the context was
        //     successfully finalised and the digest
        //     copied to d.
        //
        EMSHA_RESULT result(uint8_t* d);

        // size returns the output size of SHA256, e.g.
        // the size that the buffers passed to finalize
        // and result should be.
        //
        // Outputs:
        //     a uint32_t representing the expected size
        //     of buffers passed to result and finalize.
        uint32_t size() const { return SHA256_HASH_SIZE; }

    private:
        // mlen stores the current message length.
        uint64_t mlen;

        // The intermediate hash is 8x 32-bit blocks.
        uint32_t i_hash[8];

        // hstatus is the hash status, and hcomplete indicates
        // whether the hash has been finalised.
        EMSHA_RESULT hstatus;
        uint8_t hcomplete;

        // mb is the message block, and mbi is the message
        // block index.
        uint8_t mbi;
        uint8_t mb[SHA256_MB_SIZE];

        EMSHA_RESULT add_length(uint32_t);

        void update_message_block();

        void pad_message(uint8_t);
    }; // end class SHA256

    // sha256_digest performs a single pass hashing of the message
    // passed in.
    //
    // Inputs:
    //     m: byte buffer containing the message to hash.
    //
    //     ml: the length of m.
    //
    //     d: byte buffer that will be used to store the resulting
    //     hash; it should have at least emsha::SHA256_HASH_SIZE
    //     bytes available.
    //
    // Outputs:
    //     This function handles setting up a SHA256 context, calling
    //     update using the message data, and then calling finalize. Any
    //     of the errors that can occur in those functions can be
    //     returned here, or EMSHA_ROK if the digest was computed
    //     successfully.
    //
    EMSHA_RESULT sha256_digest(const uint8_t* m, uint32_t ml, uint8_t* d);

    // sha256_self_test runs through two test cases to ensure that the
    // SHA-256 functions are working correctly.
    //
    // Outputs:
    //     EMSHA_ROK is returned if the self tests pass.
    //
    //     EMSHA_SELFTEST_DISABLED is returned if the self tests
    //     have been disabled (e.g., libemsha was compiled with the
    //     EMSHA_NO_SELFTEST #define).
    //
    //     If a fault occurred inside the SHA-256 code, the error
    //     code from one of the update, finalize, result, or reset
    //     methods is returned.
    //
    //     If the fault is that the output does not match the test
    //     vector, EMSHA_TEST_FAILURE is returned.
    //
    EMSHA_RESULT sha256_self_test();
} // end of namespace emsha


#endif // __EMSHA_SHA256_HH
