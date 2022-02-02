# Create canonical top-level directories in the build directory.
foreach(dir include;lib;bin;share)
  set(top_level_dir ${PROJECT_BINARY_DIR}/${dir})
  if (NOT EXISTS ${top_level_dir})
    execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${top_level_dir})
  endif()
endforeach()

# Make sure submodules are updated.
if (NOT WIN32) # (Visual Studio handles this, right?)
  find_program(GIT git REQUIRED)
  message(STATUS "Updating git submodules recursively...")
  execute_process(COMMAND ${GIT} submodule update --init --recursive
                  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
endif()

# Generate a config.h file for libyaml by extracting a line from its
# CMakeLists.txt.
if (WIN32)
  find_program(FINDSTR findstr REQUIRED)
  execute_process(COMMAND ${FINDSTR} \(YAML_VERSION_ ${CMAKE_CURRENT_SOURCE_DIR}\\libyaml\\CMakeLists.txt
                  OUTPUT_FILE ${PROJECT_BINARY_DIR}/libyaml.cmake)
else()
  find_program(GREP grep REQUIRED)
  execute_process(COMMAND ${GREP} \(YAML_VERSION_ ${CMAKE_CURRENT_SOURCE_DIR}/libyaml/CMakeLists.txt
                  OUTPUT_FILE ${PROJECT_BINARY_DIR}/libyaml.cmake)
endif()
