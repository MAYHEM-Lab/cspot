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


#ifndef __EMSHA_TEST_UTILS_HH
#define __EMSHA_TEST_UTILS_HH


#include <cstdint>
#include <string>

#include <emsha/emsha.hh>
#include <emsha/sha256.hh>
#include <emsha/hmac.hh>


// How many times should a test result be checked? The goal is to
// ensure that the 'result' method is idempotent. My science teachers
// always said 5 data points was the minimum you needed, so there you
// go. Thanks, Mr. Franklin.
constexpr uint32_t	RESULT_ITERATIONS = 5;


// Test data structures.

struct hash_test {
	std::string	output;
	std::string	input;
};


struct hmac_test {
	std::uint8_t	key[256];
	std::uint32_t	keylen;
	std::string	input;
	std::string	output;
};


// General-purpose debuggery.
void	dump_hexstring(std::string&, std::uint8_t *, std::uint32_t);
void	dump_pair(std::uint8_t *, std::uint8_t *);


// SHA-256 testing functions.
emsha::EMSHA_RESULT	run_hash_test(struct hash_test, std::string);
int			run_hash_tests(struct hash_test *, std::uint32_t,
				      std::string);


// HMAC-SHA-256 testery.
emsha::EMSHA_RESULT	run_hmac_test(struct hmac_test, std::string);
int			run_hmac_tests(struct hmac_test *, std::uint32_t,
				       std::string);


#ifdef EMSHA_NO_HEXSTRING
namespace emsha {
	void	hexstring(uint8_t *dest, uint8_t *src, uint32_t srclen);
}
#endif // EMSHA_NO_HEXSTRING


#endif // __EMSHA_TEST_UTILS_HH
