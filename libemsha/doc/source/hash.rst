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

