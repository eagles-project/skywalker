include_directories(${PROJECT_BINARY_DIR}/include) # for skywalker.h
include_directories(${PROJECT_SOURCE_DIR}/include) # for skywalker.hpp
include_directories(${PROJECT_BINARY_DIR}/src) # for skywalker.mod

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_BINARY_DIR}/share/")
include(skywalker) # for add_skywalker_driver function

# Copy all the input files into place.
foreach (yaml ideal_gas.yaml)
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/${yaml}
    ${CMAKE_CURRENT_BINARY_DIR}/${yaml}
    COPYONLY)
endforeach()

add_skywalker_driver(T_gas_c T_gas.c)
add_test(ideal_gas_c T_gas_c ideal_gas.yaml)
