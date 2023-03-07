include(GNUInstallDirs)

include_directories(${PROJECT_BINARY_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/ext/klib)

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
                                OUTPUT_NAME skywalker_${SKYWALKER_PRECISION}
                                COMPILE_DEFINITIONS "HAVE_CONFIG_H=1;YAML_DECLARE_STATIC")
install(TARGETS skywalker DESTINATION ${CMAKE_INSTALL_LIBDIR})

if (ENABLE_FORTRAN)
  # Here's the Fortran library.
  add_library(skywalker_f90 skywalker.F90)
  set_target_properties(skywalker_f90 PROPERTIES
                                    OUTPUT_NAME skywalker_f90_${SKYWALKER_PRECISION})
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/skywalker.mod
          DESTINATION include)
  install(TARGETS skywalker_f90 DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

add_subdirectory(tests)
