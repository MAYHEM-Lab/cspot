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


#include <iostream>

#include <emsha/emsha.hh>

#include "test_utils.hh"


using namespace std;


#ifndef EMSHA_NO_HEXSTRING
static void
hexstring_test(void)
{
	uint8_t	buf[32];
	uint8_t	out[65];
	string	expected = "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f";

	out[64] = 0;
	for (uint32_t i = 0; i < 32; i++) {
		buf[i] = (uint8_t)i;
	}

	emsha::hexstring(out, buf, emsha::SHA256_HASH_SIZE);
	string	outs(reinterpret_cast<const char *>(out));
	if (outs != expected) {
		cerr << "FAILED: hexstring" << endl;
		cerr << "\twanted: " << expected << endl;
		cerr << "\thave:   " << out << endl;
		exit(1);
	}

	cout << "PASSED: hexstring ";
#ifdef EMSHA_NO_HEXLUT
	cout << "(small LUT)";
#else // #ifdef EMSHA_NO_HEXLUT
	cout << "(large LUT)";
#endif // #ifdef EMSHA_NO_HEXLUT
	cout << endl;
}
#endif // #ifndef EMSHA_NO_HEXSTRING


// TODO(kyle): build a test harness around this to verify times between
// runs.
static void
hash_equal_test(void)
{
	uint8_t		a[emsha::SHA256_HASH_SIZE];
	uint8_t		b[emsha::SHA256_HASH_SIZE];

	for (uint32_t i = 0; i < emsha::SHA256_HASH_SIZE; i++) {
		a[i] = static_cast<uint8_t>(i);
		b[i] = static_cast<uint8_t>(i);
	}

	if (!(emsha::hash_equal(a, b))) {
		string	s;
		cerr << "FAILED: hash_equal\n";
		cerr << "\thash_equal should have succeeded comparing a and b.\n";
		dump_hexstring(s, a, emsha::SHA256_HASH_SIZE);
		cerr << "\ta <- " << s << std::endl;
		dump_hexstring(s, b, emsha::SHA256_HASH_SIZE);
		cerr << "\tb <- " << s << std::endl;
		exit(1);
	}

	for (uint32_t i = 0; i < emsha::SHA256_HASH_SIZE; i++) {
		a[i] = static_cast<uint8_t>(i);
		b[i] = static_cast<uint8_t>(emsha::SHA256_HASH_SIZE - i);
	}

	if (emsha::hash_equal(a, b)) {
		string	s;
		cerr << "FAILED: hash_equal\n";
		cerr << "\thash_equal should not have succeeded comparing a and b.\n";
		dump_hexstring(s, a, emsha::SHA256_HASH_SIZE);
		cerr << "\ta <- " << s << std::endl;
		dump_hexstring(s, b, emsha::SHA256_HASH_SIZE);
		cerr << "\tb <- " << s << std::endl;
		exit(1);
	}

	// This catches the bug in the initial version where the code was
	//         res = a[i] ^ b[i];
	// instead of
	//         res += a[i] ^ b[i];
	for (uint32_t i = 0; i < emsha::SHA256_HASH_SIZE; i++) {
		a[i] = static_cast<uint8_t>(i);
		b[i] = static_cast<uint8_t>(i+1);
	}

	b[emsha::SHA256_HASH_SIZE - 1]--;
	if (emsha::hash_equal(a, b)) {
		string	s;
		cerr << "FAILED: hash_equal\n";
		cerr << "\tREGRESSION: hash_equal should not have succeeded comparing a and b.\n";
		dump_hexstring(s, a, emsha::SHA256_HASH_SIZE);
		cerr << "\ta <- " << s << std::endl;
		dump_hexstring(s, b, emsha::SHA256_HASH_SIZE);
		cerr << "\tb <- " << s << std::endl;
		exit(1);
	}



	cout << "PASSED: hash_equal\n";
}


int
main(void)
{
#ifndef EMSHA_NO_HEXSTRING
	hexstring_test();
#endif
	hash_equal_test();
}
