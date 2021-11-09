! ************************************************************************
! Skywalker: Copyright 2021, Cohere Consulting, LLC and
!            National Technology & Engineering Solutions of Sandia, LLC (NTESS)
!
! Copyright pending. Under provisional terms of Contract DE-NA0003525 with
! NTESS, the U.S. Government retains certain rights in this software.
!
! Redistribution and use in source and binary forms, with or without
! modification, are permitted provided that the following conditions are
! met:
!
! 1. Redistributions of source code must retain the above copyright
! notice, this list of conditions and the following disclaimer.
!
! 2. Redistributions in binary form must reproduce the above copyright
! notice, this list of conditions and the following disclaimer in the
! documentation and/or other materials provided with the distribution.
!
! 3. Neither the name of the Sandia Corporation nor the names of the
! contributors may be used to endorse or promote products derived from
! this software without specific prior written permission.
!
! THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
! EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
! IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
! PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
! CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
! EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
! PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
! PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
! LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
! NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
! SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
!
! Questions? Contact Jeffrey Johnson (jeff@cohere-llc.com)
! ************************************************************************

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

  character(len=255)      :: input_file
  type(ensemble_result_t) :: load_result
  type(settings_t)        :: settings
  type(ensemble_t)        :: ensemble
  type(input_result_t)    :: in_result
  type(input_t)           :: input
  type(output_t)          :: output

  if (command_argument_count() /= 1) then
    print *, "enumeration_test_f90: usage:"
    print *, "enumeration_test_f90: <input.yaml>"
    stop
  end if

  call get_command_argument(1, input_file)

  ! Print a banner with Skywalker's version info.
  call print_banner()

  ! Load the ensemble. Any error encountered is fatal.
  print *, "enumeration_test_f90: Loading ensemble from ", trim(input_file)
  load_result = load_ensemble(trim(input_file), "settings")

  ! Make sure everything is as it should be.
  if (load_result%error_code /= SW_SUCCESS) then
    print *, "enumeration_test_f90: ", trim(load_result%error_message)
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
  assert(ensemble%size == 11)
  do while (ensemble%next(input, output))
    assert(approx_equal(input%get("p1"), 1.0_wp))
    assert(approx_equal(input%get("p2"), 2.0_wp))
    assert(approx_equal(input%get("p3"), 3.0_wp))
    assert(input%get("tick") >= 0.0_wp)
    assert(input%get("tick") <= 10.0_wp)
    assert(input%get("tock") >= 1e1_wp)
    assert(input%get("tock") <= 1e11_wp)

    ! Look for a parameter that doesn't exist, checking its result by calling
    ! get_param() instead of get().
    in_result = input%get_param("invalid_param")
    assert(in_result%error_code == SW_PARAM_NOT_FOUND)

    ! Add a "qoi" metric set to 4.
    call output%set("qoi", 4.0_wp)
  end do

  ! Now we write out a Python module containing the output data.
  call ensemble%write("enumeration_test_f90.py")

  ! Clean up.
  call ensemble%free()
end program
