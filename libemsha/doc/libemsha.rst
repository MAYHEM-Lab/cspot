========
libemsha
========

Version: 1.0.2

Date:    2016-01-28


-----------------
Table of Contents
-----------------

+ Introduction
+ Getting and Building the Source
+ Library Overview
+ The Hash interface
+ The SHA256 class
+ The HMAC class
+ Miscellaneous functions
+ Test Programs
+ References


-------------
Introduction
-------------

This library is an MIT-licensed compact HMAC-SHA-256 C++11 library
designed for embedded systems. It is built following the JPL `Power of
Ten <http://spinroot.com/gerard/pdf/P10.pdf>`_ rules.

This library came about as a result of a need for a standalone
SHA-256 library for an embedded system. The original goal was
to implement a wrapper around the code extracted from `RFC 6234
<https://tools.ietf.org/html/rfc6234>`_; instead a standalone
implementation was decided on.

Additional resources:

+ `Github page <https://github.com/kisom/libemsha>`_
+ `Travis CI status <https://travis-ci.org/kisom/libemsha/>`_
+ `Coverity Scan page <https://scan.coverity.com/projects/libemsha-52f2a5fd-e759-43c2-9073-cf6c2ed9abdb>`_


-------------------------------
Getting and Building the Source
-------------------------------

The source code is available via `Github
<https://github.com/kisom/libemsha/>`_; each version should be git tagged. ::

    git clone https://github.com/kisom/libemsha
    git clone git@github.com:kisom/libemsha

The current release is `1.0.0 <https://github.com/kisom/libemsha/archive/1.0.0.zip>`_.

The project is built using Autotools and ``make``.

When building from a git checkout, the `autobuild` script will bootstrap
the project from the autotools sources (e.g. via ``autoreconf -i``),
run ``configurei`` (by default to use clang), and attempt to build the library
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

----------------
Library Overview
----------------

.. cpp:namespace:: emsha

The package provides a pair of classes, :cpp:class:`SHA256` and
:cpp:class:`HMAC`, that both satisfy a common interface :cpp:class:`Hash`. All
functionality provided by this library is found under the ``emsha`` namespace.


``EMSHA_RESULT``
^^^^^^^^^^^^^^^^^

The ``EMSHA_RESULT`` enum is used to convey the result of an
operation. The possible values are:

.. cpp:enum:: _EMSHA_RESULT_ : uint8_t

::

                // All operations have completed successfully so far.
                EMSHA_ROK = 0,
                
                // A self test or unit test failed.
                EMSHA_TEST_FAILURE = 1,
                
                // A null pointer was passed in as a buffer where it
                // shouldn't have been.
                EMSHA_NULLPTR = 2,
                
                // The Hash is in an invalid state.
                EMSHA_INVALID_STATE = 3,
                
                // The input to SHA256::update is too large.
                SHA256_INPUT_TOO_LONG = 4,
                
                // The self tests have been disabled, but a self test
                // function was called.
                EMSHA_SELFTEST_DISABLED = 5

As a convenience, the following ``typedef`` is also provided.

 ``typedef enum _EMSHA_RESULT_`` :cpp:type:`EMSHA_RESULT`


------------------
The Hash interface
------------------

.. cpp:class:: emsha::Hash

   The ``Hash`` class contains a top-level interface for the objects in
   this library.

In general, a `Hash` is used along the lines of: ::

        emsha::EMSHA_RESULT
        hash_single_pass(uint8_t *m, uint32_t ml, uint8_t *digest)
        {
                // Depending on the implementation, the constructor may need
                // arguments.
                emsha::Hash         h;
                emsha::EMSHA_RESULT res;
                
                res = h.write(m, ml);
                if (emsha::EMSHA_ROK != res) {
                        return res;        
                }
        
                // digest will contain the output of the Hash, and the
                // caller MUST ensure that there is enough space in
                // the buffer.
                return h.result(d);
        }

Methods
^^^^^^^

.. cpp:function:: emsha::EMSHA_RESULT reset(void)

   reset should bring the Hash back into its initial state. That is,
   the idea is that::
   
       hash->reset();
       hash->update(...); // possibly many of these...
       hash->result(...); // should always return the same hash.
   
   is idempotent, assuming the inputs to ``update`` and ``result``
   are constant. The implications of this for a given concrete class
   should be described in that class's documentation, but in general,
   it has the effect of preserving any initial state while removing any
   data written to the Hash via the update method.

.. cpp:function:: emsha::EMSHA_RESULT update(const uint8_t *m, uint32_t ml)
   
   ``update`` is used to write message data into
   the Hash.

.. cpp:function:: emsha::EMSHA_RESULT finalize(uint8_t *d)

   ``finalize`` should carry out any final operations on the `Hash`;
   after a call to finalize, no more data can be written.  Additionally,
   it transfers out the resulting hash into its argument.

   Note that this library does not allocate memory, and therefore the
   caller *must* ensure that ``d`` is a valid buffer containing at least
   ``this->size()`` bytes.

.. cpp:function:: emsha::EMSHA_RESULT result(uint8_t *d)

   ``result`` is used to transfer out the hash to the argument. This implies
   that the `Hash` must keep enough state for repeated calls to ``result``
   to work.

.. cpp:function:: uint32_t size(void)

   ``size`` should return the output size of the `Hash`; this is, how large
   the buffers written to by ``result`` should be.

-----------------
The SHA256 class
-----------------

.. cpp:class:: emsha::SHA256

   SHA256 is an implementation of the :cpp:class:`emsha::Hash` interface
   implementing the SHA-256 cryptographic hash algorithm

.. cpp:function:: SHA256::SHA256()
		  
   A SHA256 context does not need any special construction. It can be
   declared and immediately start being used.


.. cpp:function:: SHA256::~SHA256()
		  
   The SHA256 destructor will clear out its internal message buffer;
   all of the members are local and not resource handles, so cleanup
   is minimal.


.. cpp:function:: emsha::EMSHA_RESULT SHA256::reset(void)

   reset clears the internal state of the `SHA256` context and returns
   it to its initial state.  It should always return ``EMSHA_ROK``.

.. cpp:function:: emsha::EMSHA_RESULT SHA256::update(const uint8_t *m, uint32_t ml)
		  
   update writes data into the context. While there is an upper limit
   on the size of data that SHA-256 can operate on, this package is
   designed for small systems that will not approach that level of
   data (which is on the order of 2 exabytes), so it is not thought to
   be a concern.

   **Inputs**:

   + ``m``: a byte array containing the message to be written. It must
     not be NULL (unless the message length is zero).
       
   + ``ml``: the message length, in bytes.
    
   **Return values**:
   
   * ``EMSHA_NULLPTR`` is returned if ``m`` is NULL and ``ml`` is nonzero.
    
   * ``EMSHA_INVALID_STATE`` is returned if the `update` is called 
     after a call to `finalize`.
    
   * ``SHA256_INPUT_TOO_LONG`` is returned if too much data has been
     written to the context.
    
   + ``EMSHA_ROK`` is returned if the data was successfully added to
     the SHA-256 context.


.. cpp:function:: emsha::EMSHA_RESULT SHA256::finalize(uint8_t *d)

   ``finalize`` completes the digest. Once this method is called, the
   context cannot be updated unless the context is reset.
    
   **Inputs**:
   
   * d: a byte buffer that must be at least ``SHA256.size()`` in
     length.
    
   **Outputs**:
   
   * ``EMSHA_NULLPTR`` is returned if ``d`` is the null pointer.
    
   * ``EMSHA_INVALID_STATE`` is returned if the SHA-256 context is in
     an invalid state, such as if there were errors in previous
     updates.
    
   * ``EMSHA_ROK`` is returned if the context was successfully
     finalised and the digest copied to ``d``.


.. cpp:function:: emsha::EMSHA_RESULT SHA256::result(uint8_t *d)
		  
   ``result`` copies the result from the SHA-256 context into the
   buffer pointed to by ``d``, running finalize if needed. Once
   called, the context cannot be updated until the context is reset.
    
   **Inputs**:
   
   * ``d``: a byte buffer that must be at least ``SHA256.size()`` in
     length.
    
   **Outputs**:
   
   * ``EMSHA_NULLPTR`` is returned if ``d`` is the null pointer.
    
   * ``EMSHA_INVALID_STATE`` is returned if the SHA-256 context is in
     an invalid state, such as if there were errors in previous
     updates.
    
   * ``EMSHA_ROK`` is returned if the context was successfully
     finalised and the digest copied to ``d``.

.. cpp:function:: uint32_t SHA256::size(void)

   ``size`` returns the output size of SHA256, e.g.
   the size that the buffers passed to ``finalize``
   and ``result`` should be.
    
   **Outputs**:

   * a ``uint32_t`` representing the expected size of buffers passed
     to ``result`` and ``finalize``.


--------------
The HMAC class
--------------


.. cpp:class:: emsha::HMAC

   HMAC is an implementation of the :cpp:class:`emsha::Hash` interface
   implementing the HMAC keyed-hash message authentication code as
   defined in FIPS 198-1, using SHA-256 internally.

.. cpp:function:: HMAC::HMAC(const uint8_t *key, uint32_t keylen)
		  
   An HMAC context must be initialised with a key.


.. cpp:function:: HMAc::~HMAC()
		  
   The HMAC destructor will attempt to wipe the key and reset the
   underlying SHA-256 context.


.. cpp:function:: emsha::EMSHA_RESULT HMAC::reset(void)

   reset clears the internal state of the `HMAC` context and returns
   it to its initial state.  It should always return ``EMSHA_ROK``.
   This function will **not** wipe the key; an `HMAC` object that has
   `reset` called it can be used immediately after.


.. cpp:function:: emsha::EMSHA_RESULT HMAC::update(const uint8_t *m, uint32_t ml)
		  
   update writes data into the context. While there is an upper limit on
   the size of data that the underlying SHA-256 context can operate on,
   this package is designed for small systems that will not approach
   that level of data (which is on the order of 2 exabytes), so it is
   not thought to be a concern.

   **Inputs**:

   + ``m``: a byte array containing the message to be written. It must
     not be NULL (unless the message length is zero).
       
   + ``ml``: the message length, in bytes.
    
   **Return values**:
   
   * ``EMSHA_NULLPTR`` is returned if ``m`` is NULL and ``ml`` is nonzero.
    
   * ``EMSHA_INVALID_STATE`` is returned if the `update` is called 
     after a call to `finalize`.
    
   * ``SHA256_INPUT_TOO_LONG`` is returned if too much data has been
     written to the context.
    
   + ``EMSHA_ROK`` is returned if the data was successfully added to
     the HMAC context.


.. cpp:function:: emsha::EMSHA_RESULT SHA256::finalize(uint8_t *d)

   ``finalize`` completes the digest. Once this method is called, the
   context cannot be updated unless the context is reset.
    
   **Inputs**:
   
   * d: a byte buffer that must be at least ``SHA256.size()`` in
     length.
    
   **Outputs**:
   
   * ``EMSHA_NULLPTR`` is returned if ``d`` is the null pointer.
    
   * ``EMSHA_INVALID_STATE`` is returned if the HMAC context is in
     an invalid state, such as if there were errors in previous
     updates.
    
   * ``EMSHA_ROK`` is returned if the context was successfully
     finalised and the digest copied to ``d``.


.. cpp:function:: emsha::EMSHA_RESULT SHA256::result(uint8_t *d)
		  
   ``result`` copies the result from the HMAC context into the
   buffer pointed to by ``d``, running finalize if needed. Once
   called, the context cannot be updated until the context is reset.
    
   **Inputs**:
   
   * ``d``: a byte buffer that must be at least ``HMAC.size()`` in
     length.
    
   **Outputs**:
   
   * ``EMSHA_NULLPTR`` is returned if ``d`` is the null pointer.
    
   * ``EMSHA_INVALID_STATE`` is returned if the HMAC context is in
     an invalid state, such as if there were errors in previous
     updates.
    
   * ``EMSHA_ROK`` is returned if the context was successfully
     finalised and the digest copied to ``d``.

.. cpp:function:: uint32_t SHA256::size(void)

   ``size`` returns the output size of HMAC, e.g.  the size that the
   buffers passed to ``finalize`` and ``result`` should be.
    
   **Outputs**:

   * a ``uint32_t`` representing the expected size of buffers passed
     to ``result`` and ``finalize``.

-----------------------
Miscellaneous functions
-----------------------

.. cpp:function:: emsha::EMSHA_RESULT sha256_self_test(void)

   If the library was `compiled with support for self tests
   <./building.html>`_ (the default), this function will run a few self
   tests on the SHA-256 functions to validate that they are working
   correctly.

   **Outputs**:

   * ``EMSHA_ROK`` if the self-test completed successfully.

   * ``EMSHA_TEST_FAILURE`` if the SHA-256 functions did not produce
     the expected value.

   * ``EMSHA_SELFTEST_DISABLED`` if the library was built without
     support for the self test.

   * If an error occurs in the SHA-256 code, the resulting error code
     will be returned.


.. cpp:function:: emsha::EMSHA_RESULT sha256_digest(const uint8_t *m, uint32_t ml, uint8_t *d)

   The ``sha256_digest`` function will compute the digest on the
   ``ml``-byte octet string stored in ``m``, returning the result
   in ``d``. This is a convenience function implemented as: ::

    EMSHA_RESULT
    sha256_digest(const uint8_t *m, uint32_t ml, uint8_t *d)
    {
            SHA256          h;
            EMSHA_RESULT    ret;
    
            if (EMSHA_ROK != (ret = h.update(m, ml))) {
                    return ret;
            }
    
            return h.finalize(d);
    }

.. cpp:function:: emsha::EMSHA_RESULT compute_hmac(const uint8_t *k, uint32_t kl, const uint8_t *m, uint32_t ml, uint8_t *d)

   The ``compute_hmac`` function computes the MAC on the ``ml``-byte
   octet string stored in``m``, using the ``kl``-length key ``k``. The
   result is stored in ``d``. This is a convenience function implemented
   as: ::

    EMSHA_RESULT
    compute_hmac(const uint8_t *k, uint32_t kl, const uint8_t *m, uint32_t ml,
                 uint8_t *d)
    {
            EMSHA_RESULT    res;
            HMAC            h(k, kl);
    
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

.. cpp:function:: bool hash_equal(const uint8_t *a, const uint8_t *b)

   ``hash_equal`` performs a constant-time comparison of the first
   ``emsha::SHA256_HASH_SIZE`` bytes in the two byte array arguments.

   **Inputs**:

   * ``a``, ``b``: byte arrays at least ``emsha::SHA256_HASH_SIZE``
     bytes in length.

   ** Outputs**:

   * true *iff* the first ``emsha::SHA256_HASH_SIZE`` bytes match in
     both arrays.

   * false otherwise.


.. cpp:function::  void hexstring(uint8_t *dest, uint8_t *src, uint32_t srclen)

   **Note**: this function is only present if the library was
   `built with support <./building.html>`_ for the hexstring functionality.

   **Inputs**:

   * dest:   a byte array that is 2 * ``srclen``.
    
   * src:    a byte array containing the data to process.
    
   * srclen: the size of ``src``.

   **Outputs**:

   When the function returns, the hex-encoded string will be placed in
   ``dest``.
 
-------------
Test Programs
-------------

Running ``make check`` builds and runs the test programs. These are:

* ``emsha_core_test`` runs the core tests.
* ``emsha_sha256_test`` runs test vectors on the SHA-256 code.
* ``emsha_hmac_test`` runs test vectors on the HMAC code.

Additionally, the following test programs are built but not run. These
programs do not link with the library as the above programs do; instead,
they compile the object files in to avoid the libtool dance before the
library is installed.

* ``emsha_mem_test`` and ``emsha_static_mem_test`` are for memory
  profiling (e.g., with `Valgrind <http://valgrind.org/>`_ during
  development.

* ``emsha_static_sha256_test`` and ``emsha_static_hmac_test`` are used
  to facilitate testing and debugging the library. These programs run
  the same tests as the ``emsha_sha256_test`` and ``emsha_hmac_test``
  programs.


Core Tests
^^^^^^^^^^

There are three tests run in the core tests: a hexstring test (if
`support is built in <./building.html>`_) and the constant time
check. The constant time test does not validate that the function
is constant time, only that it correctly verifies that two byte
arrays are equal.


SHA-256 Tests
^^^^^^^^^^^^^

The SHA-256 checks take a number of test vectors from the Go standard
library's SHA-256 library.


HMAC Tests
^^^^^^^^^^

The HMAC checks apply the `RFC 4231 <http://tools.ietf.org/html/rfc4231>`_
test vectors to the HMAC code.



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
