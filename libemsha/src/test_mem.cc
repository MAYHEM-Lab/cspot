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


#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <chrono>
#include <cstring>
#include <algorithm>
#include <cstdio>
#include <iostream>

#include <emsha/emsha.hh>
#include <emsha/sha256.hh>
#include <emsha/hmac.hh>


// Number of test iterations.
static constexpr std::uint32_t	ITERS = 3000000;

// The key used for HMAC.
static constexpr std::uint8_t	k[] = {
	0xc5, 0xb6, 0x80, 0xac, 0xdc, 0xf4, 0xff, 0xa1,
	0x37, 0x05, 0xc0, 0x71, 0x11, 0x24, 0x31, 0x7c,
	0xa5, 0xa2, 0xcf, 0x4d, 0x33, 0x00, 0x56, 0x4f,
	0x69, 0x0f, 0x76, 0x70, 0x87, 0xd9, 0x35, 0xce,
	0xa3, 0xad, 0xa3, 0x4f, 0x30, 0xe2, 0x7c, 0x58,
	0x88, 0xd4, 0x89, 0x6a, 0xb5, 0xe0, 0x97, 0x1c,
	0x7a, 0x69, 0x65, 0xc7, 0x61, 0x0d, 0x6d, 0xb6,
	0x9b, 0x0e, 0x56, 0xd7, 0x0f, 0x5a, 0x01, 0x50,
};
static constexpr std::uint32_t	kl = sizeof(k) / sizeof(k[0]);

// The message provided to both SHA-256 and HMAC-SHA-256; it is
// "The fugacity of a constituent in a mixture of gases at a given
// temperature is proportional to its mole fraction.  Lewis-Randall Rule",
// chosen as one of the longer test vectors.
static const std::uint8_t	m[] = {
	0x54, 0x68, 0x65, 0x20, 0x66, 0x75, 0x67, 0x61,
	0x63, 0x69, 0x74, 0x79, 0x20, 0x6f, 0x66, 0x20,
	0x61, 0x20, 0x63, 0x6f, 0x6e, 0x73, 0x74, 0x69,
	0x74, 0x75, 0x65, 0x6e, 0x74, 0x20, 0x69, 0x6e,
	0x20, 0x61, 0x20, 0x6d, 0x69, 0x78, 0x74, 0x75,
	0x72, 0x65, 0x20, 0x6f, 0x66, 0x20, 0x67, 0x61,
	0x73, 0x65, 0x73, 0x20, 0x61, 0x74, 0x20, 0x61,
	0x20, 0x67, 0x69, 0x76, 0x65, 0x6e, 0x20, 0x74,
	0x65, 0x6d, 0x70, 0x65, 0x72, 0x61, 0x74, 0x75,
	0x72, 0x65, 0x20, 0x69, 0x73, 0x20, 0x70, 0x72,
	0x6f, 0x70, 0x6f, 0x72, 0x74, 0x69, 0x6f, 0x6e,
	0x61, 0x6c, 0x20, 0x74, 0x6f, 0x20, 0x69, 0x74,
	0x73, 0x20, 0x6d, 0x6f, 0x6c, 0x65, 0x20, 0x66,
	0x72, 0x61, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x2e,
	0x20, 0x20, 0x4c, 0x65, 0x77, 0x69, 0x73, 0x2d,
	0x52, 0x61, 0x6e, 0x64, 0x61, 0x6c, 0x6c, 0x20,
	0x52, 0x75, 0x6c, 0x65
};

// d is the expected result of SHA256(m).
static constexpr std::uint8_t	d[emsha::SHA256_HASH_SIZE] = {
	0x39, 0x55, 0x85, 0xce, 0x30, 0x61, 0x7b, 0x62,
	0xc8, 0x0b, 0x93, 0xe8, 0x20, 0x8c, 0xe8, 0x66,
	0xd4, 0xed, 0xc8, 0x11, 0xa1, 0x77, 0xfd, 0xb4,
	0xb8, 0x2d, 0x39, 0x11, 0xd8, 0x69, 0x64, 0x23
};

// t is the expected result of HMAC-SHA-256(k, m).
static constexpr std::uint8_t	t[emsha::SHA256_HASH_SIZE] = {
	0xbb, 0xc4, 0x7c, 0x35, 0x33, 0x4b, 0x9d, 0x90,
	0xee, 0x20, 0x88, 0x30, 0xe1, 0x1a, 0x0f, 0xf3,
	0xf4, 0x7d, 0xcc, 0xb0, 0xc5, 0xfb, 0x83, 0xe5,
	0xc2, 0xf5, 0xa7, 0x94, 0x50, 0xb6, 0xe0, 0xe0,
};

// dig is used to store the output of SHA-256 and HMAC-SHA-256.
static std::uint8_t		dig[emsha::SHA256_HASH_SIZE];


static void
init(void)
{
	std::fill(dig, dig+emsha::SHA256_HASH_SIZE, 0);
}


static void
iterate_sha(void)
{
	emsha::SHA256		ctx;
	int			cmp;
	emsha::EMSHA_RESULT	res;

	res = ctx.update(m, sizeof(m));
	assert(emsha::EMSHA_ROK == res);
	res = ctx.result(dig);
	assert(emsha::EMSHA_ROK == res);

	cmp = std::memcmp(dig, d, emsha::SHA256_HASH_SIZE);
	assert(0 == cmp);
}


static void
iterate_hmac(void)
{
	emsha::HMAC		ctx(k, kl);
	int			cmp;
	emsha::EMSHA_RESULT	res;

	res = ctx.update(m, sizeof(m));
	assert(emsha::EMSHA_ROK == res);
	res = ctx.result(dig);
	assert(emsha::EMSHA_ROK == res);

	cmp = std::memcmp(dig, t, emsha::SHA256_HASH_SIZE);
	assert(0 == cmp);
}


static void
iterate_sha_sp(void)
{
	int		cmp;

	assert(emsha::EMSHA_ROK == emsha::sha256_digest(m, sizeof(m), dig));
	cmp = std::memcmp(dig, d, emsha::SHA256_HASH_SIZE);
	assert(0 == cmp);
}


static void
iterate_hmac_sp(void)
{
	int			cmp;
	emsha::EMSHA_RESULT	res;

	res = emsha::compute_hmac(k, kl, m, sizeof(m), dig);
	assert(emsha::EMSHA_ROK == res);

	cmp = std::memcmp(dig, t, emsha::SHA256_HASH_SIZE);
	assert(0 == cmp);
}


static void
iterate(std::string label, void(iteration)(void))
{
	std::cout << "=== " << label << " ===" << std::endl;
	auto start = std::chrono::steady_clock::now();

	for (std::uint32_t i = 0; i < ITERS; i++)
		iteration();

	auto end = std::chrono::steady_clock::now();
	auto delta = (end - start );

	std::cout << "Total time: "
	    << std::chrono::duration <double, std::milli>(delta).count()
	    << " ms" << std::endl;
	std::cout << "Average over " << ITERS << " tests: "
	    << std::chrono::duration <double, std::nano>(delta).count() / ITERS
	    << " ns" << std::endl;
}


static void
cold_start(void)
{
	std::cout << "=== SHA-256 cold start ===\n";
	auto start = std::chrono::steady_clock::now();

	iterate_sha();

	auto end = std::chrono::steady_clock::now();
	auto delta = (end - start );

	std::cout << "Total time: "
	    << std::chrono::duration <double, std::nano>(delta).count()
	    << " ns" << std::endl;
}

int
main(void)
{
	init();

	cold_start();
	iterate("SHA-256", iterate_sha);
	iterate("SHA-256 single-pass", iterate_sha_sp);
	iterate("HMAC-SHA-256", iterate_hmac);
	iterate("HMAC-SHA-256 single-pass", iterate_hmac_sp);
}
