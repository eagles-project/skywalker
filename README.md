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
cmake -CMAKE_INSTALL_PREFIX=/path/to/install \
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
* `CMAKE_BUILD_TYPE` controls whether optimizatin is on (`Release`) or whether
  the build is instrumented for a debugger (`Debug`).
* `SKYWALKER_PRECISION` can be set to `single` or `double` (default) to control
  the precision of floating point numbers.

