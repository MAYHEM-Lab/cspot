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


