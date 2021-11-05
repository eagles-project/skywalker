# Create canonical top-level directories in the build directory.
foreach(dir include;lib;bin;share)
  set(top_level_dir ${PROJECT_BINARY_DIR}/${dir})
  if (NOT EXISTS ${top_level_dir})
    execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${top_level_dir})
  endif()
endforeach()

# Make sure submodules are updated.
find_program(GIT git REQUIRED)
message(STATUS "Updating git submodules recursively...")
execute_process(COMMAND ${GIT} submodule update --init --recursive
                WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

