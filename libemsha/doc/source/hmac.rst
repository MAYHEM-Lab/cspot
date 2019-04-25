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

