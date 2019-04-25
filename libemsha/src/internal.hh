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


#ifndef __EMSHA_INTERNAL_HH
#define __EMSHA_INTERNAL_HH


#include <cstdint>

namespace emsha {


static inline uint32_t
rotr32(uint32_t x, uint8_t n)
{
	return ((x >> n) | (x << (32 - n)));
}


static inline uint32_t
sha_ch(uint32_t x, uint32_t y, uint32_t z)
{
	return ((x & y) ^ ((~x) & z));
}


static inline uint32_t
sha_maj(uint32_t x, uint32_t y, uint32_t z)
{
	return (x & y) ^ (x & z) ^ (y & z);
}


static inline uint32_t
sha_Sigma0(uint32_t x)
{
	return rotr32(x, 2) ^ rotr32(x, 13) ^ rotr32(x, 22);
}


static inline uint32_t
sha_Sigma1(uint32_t x)
{
	return rotr32(x, 6) ^ rotr32(x, 11) ^ rotr32(x, 25);
}


static inline uint32_t
sha_sigma0(uint32_t x)
{
	return rotr32(x, 7) ^ rotr32(x, 18) ^ (x >> 3);
}


static inline uint32_t
sha_sigma1(uint32_t x)
{
	return rotr32(x, 17) ^ rotr32(x, 19) ^ (x >> 10);
}



} // end of namespace emsha


#endif	// __EMSHA_INTERNAL_HH
