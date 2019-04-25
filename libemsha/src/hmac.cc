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


#include <algorithm>
#include <cassert>
#include <cstdint>


#include <emsha/emsha.hh>
#include <emsha/sha256.hh>
#include <emsha/hmac.hh>


namespace emsha {

// These constants are used to keep track of the state of the HMAC.

// HMAC is in a clean-slate state following a call to reset().
constexpr uint8_t	HMAC_INIT = 0;

// The ipad constants have been XOR'd into the key and written to the
// SHA-256 context.
constexpr uint8_t	HMAC_IPAD = 1;

// The opad constants have been XOR'd into the key and written to the
// SHA-256 context.
constexpr uint8_t	HMAC_OPAD = 2;

// HMAC has been finalised
constexpr uint8_t	HMAC_FIN = 3;

// HMAC is in an invalid state.
constexpr uint8_t	HMAC_INVALID = 4;


static constexpr uint8_t ipad = 0x36;
static constexpr uint8_t opad = 0x5c;


HMAC::HMAC(const uint8_t *ik, uint32_t ikl)
    :hstate(), ctx()
{
	this->hstate = HMAC_INIT;
	std::fill(this->k, this->k + emsha::HMAC_KEY_LENGTH, 0);

	if (ikl < HMAC_KEY_LENGTH) {
		std::copy(ik, ik + ikl, this->k);
		while (ikl < HMAC_KEY_LENGTH) {
			this->k[ikl++] = 0;
		}
	} else if (ikl > HMAC_KEY_LENGTH) {
		this->ctx.update(ik, ikl);
		this->ctx.result(this->k);
		this->ctx.reset();
	} else {
		std::copy(ik, ik + ikl, this->k);
	}

	this->reset();
}

EMSHA_RESULT
HMAC::reset()
{
	EMSHA_RESULT	res;

	// Following a reset, both SHA-256 contexts and result buffer should be
	// zero'd out for a clean slate. The HMAC state should be reset
	// accordingly.
	this->ctx.reset();
	std::fill(this->buf, this->buf + SHA256_HASH_SIZE, 0);

	// Set up the k0 ⊕ ipad construction, and write it into the
	// SHA-256 context.
	uint8_t	key[HMAC_KEY_LENGTH];
	for (uint32_t i = 0; i < HMAC_KEY_LENGTH; i++) {
		key[i] = this->k[i] ^ ipad;
	}

	res = this->ctx.update(key, HMAC_KEY_LENGTH);
	if (EMSHA_ROK != res) {
		this->hstate = HMAC_INVALID;
		return res;
	}

	// This key is considered sensitive material and should be wiped.
	std::fill(key, key + HMAC_KEY_LENGTH, 0);

	this->hstate = HMAC_IPAD;
	return EMSHA_ROK;
}


EMSHA_RESULT
HMAC::update(const uint8_t *m, uint32_t ml)
{
	EMSHA_RESULT	res;
	SHA256&		hctx = this->ctx;

	EMSHA_CHECK(HMAC_IPAD == this->hstate, EMSHA_INVALID_STATE);

	// Write the message to the SHA-256 context.
	res = hctx.update(m, ml);
	if (EMSHA_ROK != res) {
		this->hstate = HMAC_INVALID;
		return res;
	}
	assert(HMAC_IPAD == this->hstate);

	return EMSHA_ROK;
}


inline EMSHA_RESULT
HMAC::final_result(uint8_t *d)
{
	if (nullptr == d) {
		return EMSHA_NULLPTR;
	}

	// If the HMAC has already been finalised, skip straight to
	// copying the result.
	if (HMAC_FIN == this->hstate) {
		std::copy(this->buf, this->buf + SHA256_HASH_SIZE, d);
		return EMSHA_ROK;
	}

	EMSHA_CHECK(HMAC_IPAD == this->hstate, EMSHA_INVALID_STATE);

	EMSHA_RESULT	res;

	// Use the result buffer as an intermediate buffer to store the result
	// of the inner hash.
	res = this->ctx.result(this->buf);
	if (EMSHA_ROK != res) {
		this->hstate = HMAC_INVALID;
		return EMSHA_INVALID_STATE;
	}
	assert(HMAC_IPAD == this->hstate);

	// The SHA-256 context needs to be reset so that it may be
	// re-used for the outer digest.
	this->ctx.reset();

	// Set up the k0 ⊕ opad construction, and write it into the
	// SHA-256 context.
	uint8_t	key[HMAC_KEY_LENGTH];
	for (uint32_t i = 0; i < HMAC_KEY_LENGTH; i++) {
		key[i] = this->k[i] ^ opad;
	}

	res = this->ctx.update(key, HMAC_KEY_LENGTH);
	if (EMSHA_ROK != res) {
		this->hstate = HMAC_INVALID;
		return res;
	}
	this->hstate = HMAC_OPAD;

	// This key is considered sensitive material and should be wiped.
	std::fill(key, key + HMAC_KEY_LENGTH, 0);

	// Write the inner hash result into the outer hash.
	res = this->ctx.update(this->buf, SHA256_HASH_SIZE);
	if (EMSHA_ROK != res) {
		this->hstate = HMAC_INVALID;
		return res;
	}

	// Write the outer hash result into the working buffer.
	res = this->ctx.finalize(this->buf);
	if (EMSHA_ROK != res) {
		this->hstate = HMAC_INVALID;
		return res;
	}
	assert(HMAC_OPAD == this->hstate);

	std::copy(this->buf, this->buf + SHA256_HASH_SIZE, d);
	this->hstate = HMAC_FIN;
	return EMSHA_ROK;
}


EMSHA_RESULT
HMAC::finalize(uint8_t *d)
{
	return this->final_result(d);
}


EMSHA_RESULT
HMAC::result(uint8_t *d)
{
	return this->final_result(d);
}


EMSHA_RESULT
compute_hmac(const uint8_t *k, uint32_t kl, const uint8_t *m, uint32_t ml,
	     uint8_t *d)
{
	EMSHA_RESULT	res;
	HMAC		h(k, kl);

	res = h.update(m, ml);
	if (EMSHA_ROK != res) {
		return res;
	}

	res = h.result(d);
	if (EMSHA_ROK != res) {
		return res;
	}

	return res;
}


} // end of namespace emsha
