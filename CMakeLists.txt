cmake_minimum_required (VERSION 3.12.0)

enable_language(C)
enable_language(Fortran)
project (skywalker)

set(CMAKE_C_STANDARD 11)

message(STATUS "Generating project files in build directory: ${PROJECT_BINARY_DIR}")
message(STATUS "C compiler is ${CMAKE_C_COMPILER} (${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION})")
message(STATUS "Fortran compiler is ${CMAKE_Fortran_COMPILER} (${CMAKE_Fortran_COMPILER_ID} ${CMAKE_Fortran_COMPILER_VERSION})")

# Version numbers.
set (SKYWALKER_MAJOR_VERSION 0)
set (SKYWALKER_MINOR_VERSION 1)
set (SKYWALKER_PATCH_VERSION 0)

set (SKYWALKER_VERSION "${SKYWALKER_MAJOR_VERSION}.${SKYWALKER_MINOR_VERSION}.${SKYWALKER_PATCH_VERSION}")
message(STATUS "Configuring skywalker v${SKYWALKER_VERSION}")

# Precision of floating point numbers.
if (SKYWALKER_PRECISION STREQUAL "single")
  set(SKYWALKER_REAL_TYPE "float")
  set(SKYWALKER_REAL_KIND "sp")
  set(SKYWALKER_DOUBLE_PRECISION 0)
elseif(SKYWALKER_PRECISION STREQUAL "double" OR NOT SKYWALKER_PRECISION)
  set(SKYWALKER_REAL_TYPE "double")
  set(SKYWALKER_REAL_KIND "dp")
  set(SKYWALKER_DOUBLE_PRECISION 1)
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
else()
  message(FATAL_ERROR "Unknown system type! Currently only Mac and Linux are supported")
endif ()

# C compiler flags.
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_C_FLAGS_RelWithDebInfo "-g -O2 -DNDEBUG")
if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-format-truncation -Wfloat-equal")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unknown-pragmas")
  if (HAVE_DOUBLE_PRECISION)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wfloat-conversion")
  endif()
  if (LINUX)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
  endif()
elseif (CMAKE_C_COMPILER_ID MATCHES "Clang")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unknown-pragmas")
  # Clang proper behaves differently from Apple Clang, and requires
  # more suppressions.
  if (NOT CMAKE_C_COMPILER_ID MATCHES "AppleClang")
    # tolower() is a recursive macro, which causes issues.
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-disabled-macro-expansion")
  endif()
elseif (CMAKE_C_COMPILER_ID MATCHES "Intel")
  set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mkl -static-intel")
  message(STATUS "Using Intel compilers and MKL linear algebra libraries")
endif()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SYS_FLAGS}")

# Fortran compiler flags
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

# Report the installation prefix.
message(STATUS "Installation prefix is ${CMAKE_INSTALL_PREFIX}")

# Third-party libs
add_subdirectory(ext)

# Unit testing.
include(CTest)
enable_testing()

# Generate skywalker.h and skywalker.cmake with the correct settings.
configure_file(
  ${PROJECT_SOURCE_DIR}/include/skywalker.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/include/skywalker.h
  @ONLY
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/skywalker.h
        DESTINATION include/skywalker)

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
