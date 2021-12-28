include_directories(${PROJECT_BINARY_DIR}/include) # for skywalker.h
include_directories(${PROJECT_SOURCE_DIR}/include) # for skywalker.hpp
include_directories(${PROJECT_BINARY_DIR}/src) # for skywalker.mod

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_BINARY_DIR}/share/")
include(skywalker) # for add_skywalker_driver function

# Copy all the input files into place.
foreach (file plot_isotherms.py ideal_gas.yaml n2_gas.yaml co2_gas.yaml)
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/${file}
    ${CMAKE_CURRENT_BINARY_DIR}/${file}
    COPYONLY)
endforeach()

add_skywalker_driver(p_gas_c p_gas.c)
add_test(ideal_gas_c p_gas_c ideal_gas.yaml)
add_test(n2_gas_c p_gas_c n2_gas.yaml)
add_test(co2_gas_c p_gas_c co2_gas.yaml)
