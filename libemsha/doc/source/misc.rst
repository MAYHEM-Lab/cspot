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
 
