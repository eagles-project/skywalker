! This program tests Skywalker's Fortran 90 interface with a user-defined
! configuration that uses an "enumeration" ensemble.

module enumeration_test_mod
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
end module enumeration_test_mod

! This macro halts the program if the predicate x isn't true.
#define assert(x) if (.not. (x)) call fatal_error("Assertion failed at line", __LINE__)

program enumeration_test

  use enumeration_test_mod
  use skywalker

  implicit none

  character(len=255)      :: input_file = "enumeration_test.yaml"
  type(ensemble_result_t) :: load_result
  type(settings_t)        :: settings
  type(ensemble_t)        :: ensemble
  type(input_t)           :: input
  type(output_t)          :: output

  ! Load the ensemble. Any error encountered is fatal.
  print *, "enumeration_test: Loading ensemble from ", trim(input_file)
  load_result = load_ensemble(trim(input_file), "settings")

  ! Make sure everything is as it should be.
  if (load_result%error_code /= 0) then
    print *, "enumeration_test: ", trim(load_result%error_message)
    stop
  end if
  assert(load_result%type == sw_enumeration)

  ! check settings
  settings = load_result%settings
  assert(trim(settings%get("param1")) == "hello")
  assert(trim(settings%get("param2")) == "81")
  assert(trim(settings%get("param3")) == "3.14159265357")

  ! ensemble information
  ensemble = load_result%ensemble
  do while (ensemble%next(input, output))
    assert(approx_equal(input%get("p1"), 1.0_wp))
    assert(approx_equal(input%get("p2"), 2.0_wp))
    assert(approx_equal(input%get("p3"), 3.0_wp))
    assert(input%get("tick") >= 0.0_wp)
    assert(input%get("tick") <= 10.0_wp)
    assert(input%get("tock") >= 1e1_wp)
    assert(input%get("tock") <= 1e11_wp)

    ! Add a "qoi" metric set to 4.
    call output%set("qoi", 4.0_wp)
  end do

  ! Now we write out a Python module containing the output data.
  call ensemble%write("enumeration_test_f90.py")

end program
