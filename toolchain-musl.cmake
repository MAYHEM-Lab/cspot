set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_C_COMPILER /opt/musl-cross/bin/x86_64-linux-musl-gcc)
set(CMAKE_CXX_COMPILER /opt/musl-cross/bin/x86_64-linux-musl-g++)
set(CMAKE_AR /opt/musl-cross/bin/x86_64-linux-musl-ar)
set(CMAKE_RANLIB /opt/musl-cross/bin/x86_64-linux-musl-ranlib)

# Make everything statically linked by default
set(CMAKE_EXE_LINKER_FLAGS "-static")
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set(BUILD_SHARED_LIBS OFF)

