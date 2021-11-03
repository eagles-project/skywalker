include(ExternalProject)
include(GNUInstallDirs)
include(${PROJECT_SOURCE_DIR}/cmake/cmake_helpers.cmake)
include_directories(${PROJECT_BINARY_DIR}/include)

# Create canonical top-level directories in the build directory.
foreach(dir include;lib;bin;share)
  set(top_level_dir ${PROJECT_BINARY_DIR}/${dir})
  if (NOT EXISTS ${top_level_dir})
    execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${top_level_dir})
  endif()
endforeach()

# Keep track of whether we've updated our submodules.
set(SUBMODULES_UPDATED FALSE)
macro(update_submodules)
  if (NOT SUBMODULES_UPDATED OR HAERO_DISABLE_SUBMODULE_CHECKS)
    message(STATUS "Updating git submodules recursively...")
    execute_process(COMMAND ${GIT} submodule update --init --recursive
                    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
    set(SUBMODULES_UPDATED TRUE)
  endif()
endmacro()

#----------------------------------------------------------------------------
# libyaml - a YAML parsing/emitting library
#----------------------------------------------------------------------------
update_submodules()
set(LIBYAML_CONFIG_OPTS
    --prefix=${PROJECT_BINARY_DIR}
    --static
    --libdir=${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
ExternalProject_Add(libyaml_proj
                    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/libyaml
                    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libyaml
                    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/libyaml
                    INSTALL_DIR ${PROJECT_BINARY_DIR}
                    CONFIGURE_COMMAND env CC=${CMAKE_C_COMPILER} CFLAGS=${CMAKE_C_FLAGS} ${CMAKE_CURRENT_SOURCE_DIR}/libyaml/configure ${LIBYAML_CONFIG_OPTS}
                    LOG_CONFIGURE TRUE
                    BUILD_COMMAND ${MAKE} -j
                    LOG_BUILD TRUE
                    INSTALL_COMMAND ${MAKE} install
                    LOG_INSTALL TRUE)
set(YAML_LIBRARY ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}/libz.a)
add_library(yaml STATIC IMPORTED GLOBAL)
set_target_properties(yaml PROPERTIES IMPORTED_LOCATION ${YAML_LIBRARY})
add_dependencies(yaml hdf5_proj)
set(SKYWALKER_EXT_LIBRARIES yaml;${SKYWALKER_EXT_LIBRARIES})

# Add all the libraries to the build system at large.
set(SKYWALKER_LIBRARIES ${SKYWALKER_EXT_LIBRARIES};${SKYWALKER_LIBRARIES} PARENT_SCOPE)
list(REMOVE_DUPLICATES SKYWALKER_EXT_INCDIRS)
set(SKYWALKER_INCDIRS ${SKYWALKER_INCDIRS};${SKYWALKER_EXT_INCDIRS} PARENT_SCOPE)
set(SKYWALKER_DRIVER_LIBRARIES ${SKYWALKER_EXT_DRIVER_LIBRARIES} PARENT_SCOPE)
set(SKYWALKER_DRIVER_INCDIRS ${SKYWALKER_EXT_DRIVER_INCDIRS} PARENT_SCOPE)

# Installation targets
install(DIRECTORY ${PROJECT_BINARY_DIR}/include/ DESTINATION include)

install(FILES ${YAML_LIBRARY} DESTINATION lib)
install(FILES ${PROJECT_BINARY_DIR}/include/yaml.h DESTINATION include)
