----------
References
----------

* `FIPS 180-4, the Secure Hash Standard <http://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf>`_
* `FIPS 198-1, The Keyed-Hash Message Authentication Code (HMAC) <http://csrc.nist.gov/publications/fips/fips198-1/FIPS-198-1_final.pdf>`_
* `RFC 2014, HMAC: Keyed-Hashing for Message Authentication <https://tools.ietf.org/html/rfc2104>`_
* `RFC 6234, US Secure Hash Algorithms (SHA and SHA-based HMAC and HKDF) <https://tools.ietf.org/html/rfc6234>`_\ [#f1]_
* The behaviour of this package was cross-checked using the Go 1.5.1
  linux/amd64 standard library's `crypto/sha256 <https://golang.org/src/crypto/sha256/>`_
  package.

.. rubric:: Footnotes

.. [#f1] This library came about after extracting the relevant C code
         from RFC 6234, and needing a C++ version. It draws heavy
         inspiration from that code base.
