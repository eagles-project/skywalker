include_directories(${PROJECT_BINARY_DIR}/skywalker) # for skywalker.mod

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_BINARY_DIR}/share/")
include(skywalker) # for add_skywalker_driver function

# Copy all the input files into place.
foreach (yaml lattice_test.yaml enumeration_test.yaml)
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/${yaml}
    ${CMAKE_CURRENT_BINARY_DIR}/${yaml}
    COPYONLY)
endforeach()

# Now create the tests for each of these inputs.
foreach(test lattice_test enumeration_test)
  add_skywalker_driver(${test} ${test}.c)
  add_skywalker_driver(${test}_f90 ${test}.F90)
endforeach()
