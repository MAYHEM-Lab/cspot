libemsha
========

.. image:: https://travis-ci.org/kisom/libemsha.svg?branch=master
    :target: https://travis-ci.org/kisom/libemsha

.. image:: https://scan.coverity.com/projects/7318/badge.svg
    :target: https://scan.coverity.com/projects/libemsha-52f2a5fd-e759-43c2-9073-cf6c2ed9abdb

This library is an MIT-licensed HMAC-SHA-256 C++11 library designed
for embedded systems. It is built following the JPL `Power of Ten
<http://spinroot.com/gerard/pdf/P10.pdf>`_ rules. It was written in
response to a need for a standalone HMAC-SHA-256 package that could run
on several platforms.


-------------------------------
Getting and Building the Source
-------------------------------

The source code is available via `Github
<https://github.com/kisom/libemsha/>`_; each version should be git tagged. ::

    git clone https://github.com/kisom/libemsha
    git clone git@github.com:kisom/libemsha

The current release is `1.0.1 <https://github.com/kisom/libemsha/releases/tag/v1.0.1>`_.

The project is built using Autotools and ``make``.

When building from a git checkout, the `autobuild` script will bootstrap
the project from the autotools sources (e.g. via ``autoreconf -i``),
run ``configure`` (by default to use clang), and attempt to build the library
and run the unit tests.

Once the autotools infrastructure has been bootstrapped, the following
should work: ::

    ./configure && make && make check && make install

There are three flags to ``configure`` that might be useful:

+ ``--disable-hexstring`` disables the provided ``hexstring`` function;
  while this might be useful in many cases, it also adds extra size to
  the code.

+ ``--disable-hexlut`` disables the larger lookup table used by
  ``hexstring``, which can save around a kilobyte of program space. If
  the ``hexstring`` function is disabled, this option has no effect.

+ ``--disable-selftest`` disables the internal self-tests, which can
  reclaim some additional program space.


-------------
Documentation
-------------

Documentation is currently done with `Sphinx <http://sphinx-doc.org/>`_.
See ``doc/``.


See also
--------

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
