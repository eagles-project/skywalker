cmake_minimum_required (VERSION 3.12.0)

option(ENABLE_FORTRAN "Enable Skywalker Fortran library" ON)

enable_language(C)
enable_language(CXX)
if (ENABLE_FORTRAN)
  enable_language(Fortran)
endif()

project (skywalker)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

message(STATUS "Generating project files in build directory: ${PROJECT_BINARY_DIR}")
message(STATUS "C compiler is ${CMAKE_C_COMPILER} (${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION})")
message(STATUS "C++ compiler is ${CMAKE_CXX_COMPILER} (${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION})")
if (ENABLE_FORTRAN)
  message(STATUS "Fortran compiler is ${CMAKE_Fortran_COMPILER} (${CMAKE_Fortran_COMPILER_ID} ${CMAKE_Fortran_COMPILER_VERSION})")
endif()

# Version numbers.
set (SKYWALKER_MAJOR_VERSION 0)
set (SKYWALKER_MINOR_VERSION 4)
set (SKYWALKER_PATCH_VERSION 0)

set (SKYWALKER_VERSION "${SKYWALKER_MAJOR_VERSION}.${SKYWALKER_MINOR_VERSION}.${SKYWALKER_PATCH_VERSION}")
message(STATUS "Configuring skywalker v${SKYWALKER_VERSION}")

# Precision of floating point numbers.
if (SKYWALKER_PRECISION STREQUAL "single")
  set(SKYWALKER_REAL_TYPE "float")
  set(SKYWALKER_REAL_KIND "sp")
  set(SKYWALKER_EPSILON "FLT_EPSILON")
  set(SKYWALKER_MIN "FLT_MIN")
  set(SKYWALKER_MAX "FLT_MAX")
elseif(SKYWALKER_PRECISION STREQUAL "double" OR NOT SKYWALKER_PRECISION)
  set(SKYWALKER_PRECISION "double")
  set(SKYWALKER_REAL_TYPE "double")
  set(SKYWALKER_REAL_KIND "dp")
  set(SKYWALKER_EPSILON "DBL_EPSILON")
  set(SKYWALKER_MIN "DBL_MIN")
  set(SKYWALKER_MAX "DBL_MAX")
else()
  message(FATAL_ERROR "Invalid precision: ${SKYWALKER_PRECISION} (must be single or double)")
endif()
message(STATUS "Using ${SKYWALKER_PRECISION} precision floating point numbers")

# We build static libraries only.
set(BUILD_SHARED_LIBS OFF)

# Figure out the system type.
if (APPLE)
  set(SYS_FLAGS "-DAPPLE=1")
  message(STATUS "Configuring on Mac.")
elseif(UNIX)
  set(SYS_FLAGS "-DLINUX=1")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread")
  set(CMAKE_Fortran_STANDARD_LIBRARIES "-no-pie")
  message(STATUS "Configuring on Linux.")
elseif(WIN32)
  set(SYS_FLAGS "/DWINDOWS=1")
  message(STATUS "Configuring on Windows.")
else()
  message(FATAL_ERROR "Unknown system type! Currently only Mac, Linux, and Windows are supported")
endif()

# C compiler flags.
if (WIN32)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /std:c11 /Wall /MD")
  # Suppress warnings from Microsoft's C++ zealots
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4820 /wd4100 /wd4255 /wd4456 /wd4996 /wd5045")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4061 /wd4242 /wd4244 /wd4245 /wd4267 /wd4457 /wd4706")
else()
  if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-format-truncation")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unknown-pragmas")
    if (HAVE_DOUBLE_PRECISION)
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wfloat-conversion")
    endif()
    if (UNIX AND NOT APPLE)
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
    endif()
  elseif (CMAKE_C_COMPILER_ID MATCHES "Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unknown-pragmas")
    # Clang proper behaves differently from Apple Clang, and requires
    # some different flags.
    if (NOT CMAKE_C_COMPILER_ID MATCHES "AppleClang")
      # tolower() is a recursive macro, which causes issues.
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-disabled-macro-expansion")
    endif()
  elseif (CMAKE_C_COMPILER_ID MATCHES "Intel")
    set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
    message(STATUS "Using Intel compilers and MKL linear algebra libraries")
  endif()
endif()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SYS_FLAGS}")

# C++ compiler flags.
if (WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++11 /Wall /Za")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4820 /wd4514")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()

if (ENABLE_FORTRAN)
  # Fortran compiler flags
  if (WIN32)
    set(CMAKE_Fortran_FLAGS "/fpp /Dc_real=c_${SKYWALKER_REAL_TYPE} /MD")
  else()
    set(CMAKE_Fortran_FLAGS "-cpp -Dc_real=c_${SKYWALKER_REAL_TYPE}")
    if (CMAKE_Fortran_COMPILER_ID STREQUAL "GNU")
      if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -fbacktrace")
      endif()
      set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -Wall -ffree-form")
      set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -Wno-unused-dummy-argument")
      set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -Werror=tabs")
      set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -Wno-maybe-uninitialized")
      if (APPLE)
        # Mac-specific stuff goes here.
      endif()
    elseif (CMAKE_Fortran_COMPILER_ID MATCHES "Intel")
      if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -traceback")
      endif()
    endif()
  endif()
endif()

# Report the installation prefix.
message(STATUS "Installation prefix is ${CMAKE_INSTALL_PREFIX}")

# Third-party libs
add_subdirectory(ext)

# Support for valgrind (Linux only).
if (UNIX AND NOT APPLE)
  find_program(VALGRIND_EXE valgrind)
  if (NOT VALGRIND_EXE MATCHES "NOTFOUND")
    set(MEMORYCHECK_COMMAND ${Valgrind_EXE})
    # Add "--gen-suppressions=all" to MEMORYCHECK_COMMAND_OPTIONS to generate
    # suppressions for Valgrind's false positives. The suppressions show up
    # right in the MemoryChecker.*.log files.
    set(MEMORYCHECK_COMMAND_OPTIONS "--leak-check=full --show-leak-kinds=all --errors-for-leak-kinds=definite,possible --track-origins=yes --error-exitcode=1 --trace-children=yes" CACHE STRING "Options passed to Valgrind." FORCE)

    # make memcheck target
    message(STATUS "Found valgrind. Enabling `make memcheck` target.")
    add_custom_target(memcheck ctest -T memcheck USES_TERMINAL)
  endif()
endif()

# Unit testing.
include(CTest)
enable_testing()

# Generate skywalker.h with the correct settings.
configure_file(
  ${PROJECT_SOURCE_DIR}/include/skywalker.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/include/skywalker.h
  @ONLY
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/skywalker.h
        DESTINATION include)
install(FILES ${PROJECT_SOURCE_DIR}/include/skywalker.hpp
        DESTINATION include)

# Generate skywalker.cmake with installation information
configure_file(
  ${PROJECT_SOURCE_DIR}/cmake/skywalker.cmake.in
  ${PROJECT_BINARY_DIR}/share/skywalker.cmake
  @ONLY
)
install(FILES ${PROJECT_BINARY_DIR}/share/skywalker.cmake
        DESTINATION share)

# Source directory.
add_subdirectory(src)

# Tools / utilities.
add_subdirectory(tools)

# Examples.
add_subdirectory(examples)
