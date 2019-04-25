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


#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>

#include "test_utils.hh"

using std::uint8_t;
using std::uint32_t;
using std::string;
using std::cout;
using std::cerr;
using std::endl;


void
dump_hexstring(string& hs, uint8_t *s, uint32_t sl)
{
	uint32_t	 bl = (2 * sl) + 1;
	char		*buf = new char[bl];
	string		 tmp;

	memset(buf, 0, bl);
	emsha::hexstring((uint8_t *)buf, s, sl);
	tmp = string(buf);
	hs.swap(tmp);
	delete[] buf;
}


emsha::EMSHA_RESULT
run_hmac_test(struct hmac_test test, string label)
{
	emsha::HMAC		h(test.key, test.keylen);
	emsha::EMSHA_RESULT	res;
	uint8_t			dig[emsha::SHA256_HASH_SIZE];
	string			hs = "";

	res = h.update((uint8_t *)test.input.c_str(), test.input.size());
	if (emsha::EMSHA_ROK != res) {
		goto exit;
	}

	for (uint32_t n = 0; n < RESULT_ITERATIONS; n++) {
		res = h.result(dig);
		if (emsha::EMSHA_ROK != res) {
			goto exit;
		}

		dump_hexstring(hs, dig, emsha::SHA256_HASH_SIZE);
		if (hs != test.output) {
			res = emsha::EMSHA_TEST_FAILURE;
			goto exit;
		}
		memset(dig, 0, emsha::SHA256_HASH_SIZE);
	}

	// Ensure that a reset and update gives the same results.
	h.reset();

	res = h.update((uint8_t *)test.input.c_str(), test.input.size());
	if (emsha::EMSHA_ROK != res) {
		goto exit;
	}

	for (uint32_t n = 0; n < RESULT_ITERATIONS; n++) {
		res = h.result(dig);
		if (emsha::EMSHA_ROK != res) {
			goto exit;
		}

		dump_hexstring(hs, dig, emsha::SHA256_HASH_SIZE);
		if (hs != test.output) {
			res = emsha::EMSHA_TEST_FAILURE;
			goto exit;
		}
		memset(dig, 0, emsha::SHA256_HASH_SIZE);
	}

	// Test that the single-pass function works.
	res = emsha::compute_hmac(test.key, test.keylen,
	    (uint8_t *)test.input.c_str(), test.input.size(),
	    dig);
	if (emsha::EMSHA_ROK != res) {
		cerr << "(running single pass function test)\n";
		goto exit;
	}

	dump_hexstring(hs, dig, emsha::SHA256_HASH_SIZE);
	if (hs != test.output) {
		cerr << "(comparing single pass function output)\n";
		res = emsha::EMSHA_TEST_FAILURE;
		goto exit;
	}
	memset(dig, 0, emsha::SHA256_HASH_SIZE);

	res = emsha::EMSHA_ROK;

exit:
	if (emsha::EMSHA_ROK != res) {
		cerr << "FAILED: " << label << endl;
		cerr << "\tinput: " << test.input << endl;
		cerr << "\twanted: " << test.output << endl;
		cerr << "\thave:   " << hs << endl;
	}

	return res;
}


int
run_hmac_tests(struct hmac_test *tests, uint32_t ntests, string label)
{
	for (uint32_t i = 0; i < ntests; i++) {
		if (emsha::EMSHA_ROK != run_hmac_test(*(tests + i), label)) {
			return -1;
		}
	}
	cout << "PASSED: " << label << " (" << ntests << ")" << endl;
	return 0;
}


emsha::EMSHA_RESULT
run_hash_test(struct hash_test test, string label)
{
	emsha::SHA256		ctx;
	emsha::EMSHA_RESULT	res;
	uint8_t			dig[emsha::SHA256_HASH_SIZE];
	string			hs;

	res = ctx.update((uint8_t *)test.input.c_str(), test.input.size());
	if (emsha::EMSHA_ROK != res) {
		goto exit;
	}

	for (uint32_t n = 0; n < RESULT_ITERATIONS; n++) {
		res = ctx.result(dig);
		if (emsha::EMSHA_ROK != res) {
			goto exit;
		}

		dump_hexstring(hs, dig, emsha::SHA256_HASH_SIZE);
		if (hs != test.output) {
			res = emsha::EMSHA_TEST_FAILURE;
			goto exit;
		}
		memset(dig, 0, emsha::SHA256_HASH_SIZE);
	}

	// Ensure that a reset and update gives the same results.
	ctx.reset();

	res = ctx.update((uint8_t *)test.input.c_str(), test.input.size());
	if (emsha::EMSHA_ROK != res) {
		goto exit;
	}

	for (uint32_t n = 0; n < RESULT_ITERATIONS; n++) {
		res = ctx.result(dig);
		if (emsha::EMSHA_ROK != res) {
			goto exit;
		}

		dump_hexstring(hs, dig, emsha::SHA256_HASH_SIZE);
		if (hs != test.output) {
			res = emsha::EMSHA_TEST_FAILURE;
			goto exit;
		}
		memset(dig, 0, emsha::SHA256_HASH_SIZE);
	}

	// Test that the single-pass function works.
	res = emsha::sha256_digest((uint8_t *)test.input.c_str(),
	    test.input.size(), dig);
	if (emsha::EMSHA_ROK != res) {
		cerr << "(running single pass function test)\n";
		goto exit;
	}

	dump_hexstring(hs, dig, emsha::SHA256_HASH_SIZE);
	if (hs != test.output) {
		cerr << "(comparing single pass function output)\n";
		res = emsha::EMSHA_TEST_FAILURE;
		goto exit;
	}
	memset(dig, 0, emsha::SHA256_HASH_SIZE);
	res = emsha::EMSHA_ROK;

exit:
	if (emsha::EMSHA_ROK != res) {
		cerr << "FAILED: " << label << endl;
		cerr << "\tinput: '" << test.input << "'" << endl;
		cerr << "\twanted: " << test.output << endl;
		cerr << "\thave:   " << hs << endl;
	}
	return res;
}


int
run_hash_tests(struct hash_test *tests, uint32_t ntests, string label)
{
	for (uint32_t i = 0; i < ntests; i++) {
		if (emsha::EMSHA_ROK != run_hash_test(*(tests + i), label)) {
			return -1;
		}
	}
	cout << "PASSED: " << label << " (" << ntests << ")" << endl;
	return 0;
}


#ifdef EMSHA_NO_HEXSTRING

// If the library was built without hex string support, include it in
// the test code.
namespace emsha {
static void
write_hex_char(uint8_t *dest, uint8_t src)
{
	static constexpr uint8_t lut[256][3] = {
		"00", "01", "02", "03", "04", "05", "06", "07",
		"08", "09", "0a", "0b", "0c", "0d", "0e", "0f",
		"10", "11", "12", "13", "14", "15", "16", "17",
		"18", "19", "1a", "1b", "1c", "1d", "1e", "1f",
		"20", "21", "22", "23", "24", "25", "26", "27",
		"28", "29", "2a", "2b", "2c", "2d", "2e", "2f",
		"30", "31", "32", "33", "34", "35", "36", "37",
		"38", "39", "3a", "3b", "3c", "3d", "3e", "3f",
		"40", "41", "42", "43", "44", "45", "46", "47",
		"48", "49", "4a", "4b", "4c", "4d", "4e", "4f",
		"50", "51", "52", "53", "54", "55", "56", "57",
		"58", "59", "5a", "5b", "5c", "5d", "5e", "5f",
		"60", "61", "62", "63", "64", "65", "66", "67",
		"68", "69", "6a", "6b", "6c", "6d", "6e", "6f",
		"70", "71", "72", "73", "74", "75", "76", "77",
		"78", "79", "7a", "7b", "7c", "7d", "7e", "7f",
		"80", "81", "82", "83", "84", "85", "86", "87",
		"88", "89", "8a", "8b", "8c", "8d", "8e", "8f",
		"90", "91", "92", "93", "94", "95", "96", "97",
		"98", "99", "9a", "9b", "9c", "9d", "9e", "9f",
		"a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",
		"a8", "a9", "aa", "ab", "ac", "ad", "ae", "af",
		"b0", "b1", "b2", "b3", "b4", "b5", "b6", "b7",
		"b8", "b9", "ba", "bb", "bc", "bd", "be", "bf",
		"c0", "c1", "c2", "c3", "c4", "c5", "c6", "c7",
		"c8", "c9", "ca", "cb", "cc", "cd", "ce", "cf",
		"d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
		"d8", "d9", "da", "db", "dc", "dd", "de", "df",
		"e0", "e1", "e2", "e3", "e4", "e5", "e6", "e7",
		"e8", "e9", "ea", "eb", "ec", "ed", "ee", "ef",
		"f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7",
		"f8", "f9", "fa", "fb", "fc", "fd", "fe", "ff"
	};

	memcpy(dest, lut[src], 2);
}


void
hexstring(uint8_t *dest, uint8_t *src, uint32_t srclen)
{
	uint8_t	*dp = dest;

	for (uint32_t i = 0; i < srclen; i++, dp += 2) {
		write_hex_char(dp, src[i]);
	}
}


}

#endif // EMSHA_NO_HEXSTRING
