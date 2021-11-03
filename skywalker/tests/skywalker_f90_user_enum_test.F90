! This program tests Skywalker's Fortran 90 interface with a user-defined
! configuration that uses an "enumeration" ensemble.

module f90_user_enum_test
  implicit none
contains
  subroutine fatal_error(message, line)
    character(len=*), intent(in) :: message
    integer :: line

    print *, message, line
    stop
  end subroutine

  function approx_equal(x, y) result(equal)
    use iso_c_binding, only: c_float, c_double
    real(c_real), intent(in) :: x, y
    logical :: equal

    if (abs(x - y) < 1e-14) then
      equal = .true.
    else
      equal = .false.
    end if
  end function
end module f90_user_enum_test

! This macro halts the program if the predicate x isn't true.
#define assert(x) if (.not. (x)) call fatal_error("Assertion failed at line", __LINE__)

program skywalker_f90_enum_test

  use f90_user_enum_test
  use skywalker
  use haero_precision, only: wp

  implicit none

  character(len=255)  :: input_file = "f90_user_enum_test.yaml"
  type(ensemble_t)    :: ensemble
  type(input_data_t)  :: input
  type(output_data_t) :: output
  integer             :: i

  ! Load the ensemble. Any error encountered is fatal.
  print *, "skywalker_f90_user_test: Loading ensemble from ", trim(input_file)
  ensemble = load_ensemble("user", trim(input_file), "user_program")

  ! Make sure everything is as it should be.

  ! process section
  assert(trim(ensemble%program_name) == "user_program")
  assert(ensemble%num_program_params == 3)
  assert(trim(ensemble%program_param_names(1)) == "param1")
  assert(trim(ensemble%program_param_values(1)) == "hello")
  assert(trim(ensemble%program_param_names(2)) == "param2")
  assert(trim(ensemble%program_param_values(2)) == "81")
  assert(trim(ensemble%program_param_names(3)) == "param3")
  assert(trim(ensemble%program_param_values(3)) == "3.14159265357")

  ! aerosol configuration -- everything should be zero
  assert(ensemble%num_modes == 0)
  assert(ensemble%num_populations == 0) ! all species in all modes
  assert(ensemble%num_gases == 0)

  ! ensemble information
  assert(ensemble%size == 11)
  do i = 1, ensemble%size
    input = ensemble%inputs(i)

    ! Time stepping parameters -- should be zero
    assert(input%dt == 0._wp)
    assert(input%total_time == 0._wp)

    ! Atmospheric parameters -- should be zero
    assert(approx_equal(input%pressure, 0._wp))
    assert(approx_equal(input%vapor_mixing_ratio, 0._wp))
    assert(approx_equal(input%height, 0._wp))
    assert(approx_equal(input%hydrostatic_dp, 0._wp))
    assert(approx_equal(input%planetary_boundary_layer_height, 0._wp))

    ! User parameters

    ! Fixed parameters
    assert(approx_equal(input%user_param("p1"), 1.0_wp))
    assert(approx_equal(input%user_param("p2"), 2.0_wp))
    assert(approx_equal(input%user_param("p3"), 3.0_wp))

    ! Ensemble parameters
    assert(input%user_param("tick") >= 0.0_wp)
    assert(input%user_param("tick") <= 10.0_wp)
    assert(input%user_param("tock") >= 1e1_wp)
    assert(input%user_param("tock") <= 1e11_wp)
  end do

  ! Process input to get output! To keep things simple, we just copy our input
  ! data to output data.
  do i = 1, ensemble%size
    input = ensemble%inputs(i)
    output = ensemble%outputs(i)
    output%interstitial_aero_nmrs = input%interstitial_aero_nmrs
    output%cloud_aero_nmrs = input%cloud_aero_nmrs
    output%interstitial_aero_mmrs = input%interstitial_aero_mmrs
    output%cloud_aero_mmrs = input%cloud_aero_mmrs

    ! Add a "qoi" metric set to 4.
    call output%add_metric("qoi", 4.0_wp)
  end do

  ! Now we write out a Python module containing the output data.
  call ensemble%write_py_module("f90_user_enum_test.py")

  ! Clean up.
  call ensemble%free()

  ! If we got here, everything is all right!
  print *, "PASS"

end program
