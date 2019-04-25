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

#include <stdint.h>

#include <emsha/emsha.hh>
#include <emsha/sha256.hh>

namespace emsha {

    const uint32_t HMAC_KEY_LENGTH = SHA256_MB_SIZE;

    // HMAC is a keyed hash that can be used to produce an
    // authenticated hash of some data. The HMAC is built on (and
    // uses internally) the SHA-256 class; it's helpful to note that
    // faults that occur in the SHA-256 code will be propagated up
    // as the return value from many of the HMAC functions.
    class HMAC
    {
    public:
        // An HMAC is constructed with a key and the
        // length of the key. This key is stored in
        // the HMAC context, and is wiped by the HMAC
        // destructor.
        //
        // Inputs:
        //     k: the HMAC key.
        //     kl: the length of the HMAC key.
        //
        HMAC(const uint8_t* k, uint32_t kl);

        // reset clears any data written to the HMAC;
        // this is equivalent to constructing a new HMAC,
        // but it preserves the keys.
        //
        // Outputs:
        //     EMSHA_ROK is returned if the reset occurred
        //     without (detected) fault.
        //
        //     If a fault occurs with the underlying SHA-256
        //     context, the error code is returned.
        //
        EMSHA_RESULT reset();

        // update writes data into the context. While there is
        // an upper limit on the size of data that the
        // underlying hash can operate on, this package is
        // designed for small systems that will not approach
        // that level of data (which is on the order of 2
        // exabytes), so it is not thought to be a concern.
        //
        // Inputs:
        //     m: a byte array containing the message to be
        //     written. It must not be NULL (unless the message
        //     length is zero).
        //
        //     ml: the message length, in bytes.
        //
        // Outputs:
        //     EMSHA_NULLPTR is returned if m is NULL and ml is
        //     nonzero.
        //
        //     EMSHA_INVALID_STATE is returned if the update
        //     is called after a call to finalize.
        //
        //     SHA256_INPUT_TOO_LONG is returned if too much
        //     data has been written to the context.
        //
        //     EMSHA_ROK is returned if the data was
        //     successfully written into the HMAC context.
        //
        EMSHA_RESULT update(const uint8_t* m, uint32_t ml);

        // finalize completes the HMAC computation. Once this
        // method is called, the context cannot be updated
        // unless the context is reset.
        //
        // Inputs:
        //     d: a byte buffer that must be at least
        //     HMAC.size() in length.
        //
        // Outputs:
        //     EMSHA_NULLPTR is returned if d is the null
        //     pointer.
        //
        //     EMSHA_INVALID_STATE is returned if the HMAC
        //     context is in an invalid state, such as if there
        //     were errors in previous updates.
        //
        //     EMSHA_ROK is returned if the context was
        //     successfully finalised and the digest copied to
        //     d.
        //
        EMSHA_RESULT finalize(uint8_t* d);

        // result copies the result from the HMAC context into
        // the buffer pointed to by d, running finalize if
        // needed. Once called, the context cannot be updated
        // until the context is reset.
        //
        // Inputs:
        //     d: a byte buffer that must be at least
        //     HMAC.size() in length.
        //
        // Outputs:
        //     EMSHA_NULLPTR is returned if d is the null
        //     pointer.
        //
        //     EMSHA_INVALID_STATE is returned if the HMAC
        //     context is in an invalid state, such as if there
        //     were errors in previous updates.
        //
        //     EMSHA_ROK is returned if the context was
        //     successfully finalised and the digest copied to
        //     d.
        //
        EMSHA_RESULT result(uint8_t*);

        // size returns the output size of HMAC-SHA-256, e.g.
        // the size that the buffers passed to finalize and
        // result should be.
        //
        // Outputs:
        //     A uint32_t representing the expected size
        //     of buffers passed to result and finalize.
        uint32_t size() const { return SHA256_HASH_SIZE; }

    private:
        uint8_t hstate;
        SHA256 ctx;
        uint8_t k[HMAC_KEY_LENGTH];
        uint8_t buf[SHA256_HASH_SIZE];

        EMSHA_RESULT final_result(uint8_t*);
    };

    // compute_hmac performs a single-pass HMAC computation over
    // a message.
    //
    // Inputs:
    //     k: a byte buffer containing the HMAC key.
    //
    //     kl: the length of the HMAC key.
    //
    //     m: the message data over which the HMAC is to be computed.
    //
    //     ml: the length of the message.
    //
    //     d: a byte buffer that will be used to store the resulting
    //     HMAC. It should be SHA256_HASH_SIZE bytes in size.
    //
    // Outputs:
    //     This function handles setting up the HMAC context with
    //     the given key, calling update with the message data, and
    //     then calling finalize to place the result in the output
    //     buffer. Any of the faults that can occur in these functions
    //     can be returned here, or EMSHA_ROK if the HMAC was
    //     successfully computed.
    EMSHA_RESULT compute_hmac(const uint8_t* k, uint32_t kl,
            const uint8_t* m, uint32_t ml,
            uint8_t* d);
}
