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



