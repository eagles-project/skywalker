# Installation

We've tried to make Skywalker as easy as possible to use with your current
software. To build it, you need

+ [CMake v3.10+](https://cmake.org/)
+ GNU Make
+ Reliable C and C++ compilers (like GCC and Clang)
+ A decent Fortran compiler (like GFortran or Intel's `ifort`)

## Clone the Repository

First, go get the [source code](https://github.com/eagles-project/skywalker)
at GitHub:

=== "SSH"
    ```
    git clone git@github.com:eagles-project/skywalker.git
    ```
=== "HTTPS"
    ```
    git clone https://github.com/eagles-project/skywalker.git
    ```

This places a `skywalker` folder into your current path.

## Configure Skywalker

Skywalker uses CMake as its build system. CMake accepts a few options that
specify how Skywalker should be built. Here are the most important options for
you to consider.

* `CMAKE_INSTALL_PREFIX` sets the path to which the Skywalker C, C++, and
  Fortran headers/module and libraries are installed. This is often something
  like `/usr/local` by default.
* `CMAKE_BUILD_TYPE` controls whether optimization is on (`Release`) or whether
  the build is instrumented for a debugger (`Debug`).
* `SKYWALKER_PRECISION` can be set to `single` or `double` (default) to control
  the precision of floating point numbers.
* `CMAKE_C_COMPILER` sets the C compiler that is used to build Skywalker.
  Usually, the default compiler is fine, but if you want to use an MPI-capable
  compiler or a specific vendor compiler, you can specify it with this option.
* `CMAKE_Fortran_COMPILER` sets the Fortran compiler that is used to build
  Skywalker's Fortran interface.

From the top-level `skywalker` directory, create a "build" directory
(e.g. `build`). This is where you'll configure and build Skywalker. For example,
to configure a debuggable build of Skywalker that uses double precision, do the
following:

```
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/path/to/install \
      -DCMAKE_BUILD_TYPE=Debug \
      -DSKYWALKER_PRECISION=double \
      ..
```

This creates a set of `Makefile`s in your build directory. Now you're ready
to build.

## Build, Test, and Install Skywalker

To build skywalker, make sure you're in your build directory and type

```
make -j
```

If this process succeeds, you can run the tests and examples included with
Skywalker by typing

```
make test
```

You should see several tests run (and hopefully pass!). Now, to install
Skywalker to the path you specified with `CMAKE_INSTALL_PREFIX`, type

```
make install
```

This process installs the following artifacts, which you can use to build your
own Skywalker programs:

* `PREFIX/lib/libskywalker_<precision>.a`, a library you can use with a C or C++
  Skywalker program.
* `PREFIX/lib/libskywalker_f90_<precision>.a`, a library you can use with a
  Fortran Skywalker program.
* `PREFIX/include/skywalker.h`, a C header file that provides Skywalker's C
  interface.
* `PREFIX/include/skywalker.hpp`, a C++ header file that provides Skywalker's
  C++ interface.
* `PREFIX/include/skywalker.mod`, a Fortran 90 module header file that provides
  Skywalker's Fortran interface.
* `PREFIX/share/skywalker.cmake`, a CMake file that includes installation
  information for Skywalker, plus a function called `add_skywalker_driver` you
  can use to build your own driver programs.

Here, `PREFIX` stands for the path you passed to `CMAKE_INSTALL_PREFIX`. These
files are all you need to build Skywalker programs.

## Try It Out!

At this point, you're ready to start using Skywalker. Look at the
[Quick Start](quick_start.md) section to get started.
