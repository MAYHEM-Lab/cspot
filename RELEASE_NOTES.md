# CSPOT Release Notes

## Version 3.0

**Release Date: September 2026**

CSPOT 3.0 is a substantially new release of the CSPOT implementation and build system.

### Highlights

- added [CAPLets](https://sites.cs.ucsb.edu/~rich/publications/caplets21.pdf) security token support for per-message authorization
- added support for MQTT message transport (experimental)
- added senspot-file versioned file transport
- added binary software distribution and update for x86, arm64 and armv6l
- added build environment for handler compilation using binary distribution
- performance enhancements for handler dispatch
- unified x86, arm64, armv6l, build environments (ubuntu and debian only)
- added apple silicon support (experimental)
- depricated docker support
- bug fixes galore

### Backward compatibility

Release 3.0 is API compatible with the previous releases but the internal WOOF format
has changed to accommodate multiple architectures.  It is possible to upgrade existing WOOFs
but the process, if it fails, can leave them in an unrecoverable state.  It is best to dump 
the WOOF contents, reinitialize them using the latest release, and reload them when upgrading
and existing namespace.

However, the wire format has not changed so older installations and the latest release
can coexist in the same deployment.  Upgrade (or reinitialization) is necessary for the new
woofc-name-space-platform to be able to access the WOOFs in a namespace.

### Static linking of binaries

CSPOT 3.0 (like CSPOT 2.X) generates statically linked binaries.  Because static linking is no
longer supported by the default C and C++ environments that ship for the x86, building
CSPOT from source requires [MUSL](https://musl.libc.org/about.html).  At the time of this release, however,
Linux for the Raspberry Pi (which is debian based) still supports static linking with the gcc toolchain.  The
build environment attempts to autodetect whether MUSL is installed an will use it when it is.  

### Network transport

Like previous releases, CSPOT 3.0 uses [ZeroMQ](https://zeromq.org) as the default message transport.
However, it builds it from scratch using an older fork so that the resulting library can be
statically linked.  It is possible to replace the ZeroMQ dependency with a simple, socket-based
transport called CMQ that ships with the software but they both cannot be active simultaneously.  Thus
all CSPOT binaries in a deployment need to be configured either to use ZeroMQ or CMQ with sockets.
