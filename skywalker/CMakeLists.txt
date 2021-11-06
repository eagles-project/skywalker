include_directories(${PROJECT_BINARY_DIR})
include_directories(${PROJECT_SOURCE_DIR}/ext/klib)

# Generate skywalker.h and skywalker.cmake with the correct settings.
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/skywalker.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/skywalker.h
  @ONLY
)

# Generate skywalker.cmake with installation information
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/skywalker.cmake.in
  ${PROJECT_BINARY_DIR}/share/skywalker.cmake
  @ONLY
)

# Generate a config.h file for libyaml
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_BINARY_DIR}")
include(libyaml)
configure_file(
  ${PROJECT_SOURCE_DIR}/ext/libyaml/cmake/config.h.in
  ${PROJECT_BINARY_DIR}/config.h
  @ONLY
)

# We mash all the libyaml sources directly into our libraries to make them
# nice and self-contained.
set(LIBYAML_INCDIRS ${PROJECT_SOURCE_DIR}/ext/libyaml/include;
                    ${PROJECT_SOURCE_DIR}/ext/libyaml/src;
                    ${PROJECT_BINARY_DIR})
include_directories(${LIBYAML_INCDIRS})
foreach(src api.c dumper.c emitter.c loader.c parser.c reader.c scanner.c)
  list(APPEND LIBYAML_SOURCES ${PROJECT_SOURCE_DIR}/ext/libyaml/src/${src})
endforeach()

# Here we build the skywalker C library.
add_library(skywalker skywalker.c ${LIBYAML_SOURCES})
set_target_properties(skywalker PROPERTIES
                                OUTPUT_NAME skywalker_${SKYWALKER_REAL_TYPE}
                                COMPILE_DEFINITIONS HAVE_CONFIG_H=1)

# The Fortran library is just like the C library, but with Fortran!
add_library(skywalker_f90 skywalker.c skywalker.F90)
set_target_properties(skywalker_f90 PROPERTIES
                                    OUTPUT_NAME skywalker_f90_${SKYWALKER_REAL_TYPE}
                                    COMPILE_DEFINITIONS "SKYWALKER_F90=1;HAVE_CONFIG_H=1")

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/skywalker.h
        DESTINATION include/skywalker)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/skywalker.mod
        DESTINATION include/skywalker)
install(TARGETS skywalker skywalker_f90 DESTINATION lib)

# Install py2ncl, a post-processing script that translates Skywalker output
# Python modules into NCL data files.
install(FILES py2ncl
        PERMISSIONS OWNER_READ;GROUP_READ;WORLD_READ;
                    OWNER_WRITE;
                    OWNER_EXECUTE;GROUP_EXECUTE;WORLD_EXECUTE
        DESTINATION bin)

install(FILES ${PROJECT_BINARY_DIR}/share/skywalker.cmake
        DESTINATION share)

add_subdirectory(tests)
