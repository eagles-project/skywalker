if (HAERO_FORTRAN)

  # Copy all the input files into place.
  foreach (yaml f90_test.yaml f90_user_test.yaml f90_user_enum_test.yaml)
    configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/${yaml}
      ${CMAKE_CURRENT_BINARY_DIR}/${yaml}
      COPYONLY
    )
  endforeach()

  # Now create the tests for each of these inputs.
  foreach(test skywalker_f90_test skywalker_f90_user_test skywalker_f90_user_enum_test)
    add_executable(${test} ${test}.F90)
    target_link_libraries(${test} skywalker;yaml_cpp;${HAERO_LIBRARIES})
    set_target_properties(${test} PROPERTIES LINKER_LANGUAGE Fortran)
    add_test(${test} ${test})
    set_tests_properties(${test} PROPERTIES PASS_REGULAR_EXPRESSION "PASS")
  endforeach()
endif()
