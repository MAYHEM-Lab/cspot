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

