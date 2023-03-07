include_directories(${PROJECT_BINARY_DIR}/include) # for skywalker.h
include_directories(${PROJECT_SOURCE_DIR}/include) # for skywalker.hpp
include_directories(${PROJECT_BINARY_DIR}/src) # for skywalker.mod

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_BINARY_DIR}/share/")
include(skywalker) # for add_skywalker_driver function

# Copy all the input files into place.
foreach (file plot_isotherms.py ideal_gas_isotherms.yaml n2_gas_isotherms.yaml
              co2_gas_isotherms.yaml)
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/${file}
    ${CMAKE_CURRENT_BINARY_DIR}/${file}
    COPYONLY)
endforeach()

# Build the C/C++/Fortran drivers and add tests.
if (ENABLE_FORTRAN)
  set(F90 "F90")
endif()
foreach (lang c cpp ${F90})
  add_skywalker_driver(isotherms_${lang} isotherms.${lang})
  add_test(ideal_gas_isotherms_${lang} isotherms_${lang} ideal_gas_isotherms.yaml)
  add_test(n2_gas_isotherms_${lang} isotherms_${lang} n2_gas_isotherms.yaml)
  add_test(co2_gas_isotherms_${lang} isotherms_${lang} co2_gas_isotherms.yaml)
endforeach()

