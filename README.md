# Skywalker

Skywalker is a simple tool for running parameter studies and code-to-code
comparisons.

More to come soon!

## Installation

To install Skywalker, make sure you have the following:

+ [CMake v3.10+](https://cmake.org/)
+ GNU Make
+ Reliable C and C++ compilers
+ GFortran or Intel's Fortran compiler

Clone this repository, and from the top-level ѕource directory, create a build
directory, in which you can configure and compile it. For example, to configure
a debuggable build of Skywalker that uses double precision, do the following:

```
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/path/to/install \
      -DCMAKE_BUILD_TYPE=Debug \
      -DSKYWALKER_PRECISION=double \
      ..
make -j
make install
```

Above we've used the three most useful configuration options:

* `CMAKE_INSTALL_PREFIX` sets the path to which the Skywalker C, C++, and
  Fortran headers/module and libraries are installed. This is often something
  like `/usr/local` by default.
* `CMAKE_BUILD_TYPE` controls whether optimization is on (`Release`) or whether
  the build is instrumented for a debugger (`Debug`).
* `SKYWALKER_PRECISION` can be set to `single` or `double` (default) to control
  the precision of floating point numbers.

This process builds and installs the following artifacts, which you can use to
build your own Skywalker-powered programs:

* `PREFIX/lib/libskywalker_<precision>.a`, a library you can use with a C or C++
  driver program.
* `PREFIX/lib/libskywalker_f90_<precision>.a`, a library you can use with a
  Fortran driver program.
* `PREFIX/include/skywalker.h`, a C header file that provides Skywalker's C
  interface.
* `PREFIX/include/skywalker.hpp`, a C++ header file that provides Skywalker's
  C++ interface.
* `PREFIX/include/skywalker.mod`, a Fortran 90 module header file that provides
  Skywalker's Fortran interface.
* `PREFIX/share/skywalker.cmake`, a CMake file that includes installation
  information for Skywalker, plus a function called `add_skywalker_driver` you
  can use to build your own driver programs.

These files are all you need to build driver programs for conducting parameter
studies using Skywalker.

## External (third-party) dependencies

Skywalker uses the following libraries, which are provided as submodules in the
`ext` directory:

* [klib](https://github.com/attractivechaos/klib) - a library of C data
  structures
* [libyaml](https://github.com/yaml/libyaml) - a simple YAML parser

The source code for each library is covered by its license.

The Skywalker library includes everything it needs from these libraries, so you
don't have to build them or link your driver program to them.
