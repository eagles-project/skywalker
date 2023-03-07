include_directories(${PROJECT_SOURCE_DIR}/include) # for skywalker.hpp
include_directories(${PROJECT_BINARY_DIR}/src) # for skywalker.mod

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_BINARY_DIR}/share/")
include(skywalker) # for add_skywalker_driver function

# Copy input files into place and create the tests for each of these inputs.
foreach(test lattice_test enumerated_test mixed_test array_param_test)
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/${test}.yaml
    ${CMAKE_CURRENT_BINARY_DIR}/${test}.yaml
    COPYONLY)
  add_skywalker_driver(${test} ${test}.c)
  add_test(${test} ${test} ${test}.yaml)
  add_skywalker_driver(${test}_cpp ${test}.cpp)
  add_test(${test}_cpp ${test}_cpp ${test}.yaml)
  if (ENABLE_FORTRAN)
    add_skywalker_driver(${test}_f90 ${test}.F90)
    add_test(${test}_f90 ${test}_f90 ${test}.yaml)
  endif()
endforeach()

# Validation tests (currently C only).
add_skywalker_driver(validation_test validation_test.c)
add_test(validation_test validation_test)
