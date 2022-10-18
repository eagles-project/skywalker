! -------------------------------------------------------------------------
! Copyright (c) 2021,
! National Technology & Engineering Solutions of Sandia, LLC (NTESS).
!
! Under the terms of Contract DE-NA0003525 with NTESS, the U.S. Government
! retains certain rights in this software.
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
! -------------------------------------------------------------------------

! This program tests Skywalker's Fortran 90 interface and its support for
! array parameters.

module array_param_test_mod
  use iso_c_binding, only: c_float, c_double
  implicit none

contains
  subroutine fatal_error(message, line)
    character(len=*), intent(in) :: message
    integer :: line

    print *, message, line
    stop
  end subroutine

  function approx_equal(x, y) result(equal)
    use skywalker, only: swp
    real(swp), intent(in) :: x, y
    logical :: equal

    if (abs(x - y) < 1e-14) then
      equal = .true.
    else
      equal = .false.
    end if
  end function
end module array_param_test_mod

! This macro halts the program if the predicate x isn't true.
#define assert(x) if (.not. (x)) call fatal_error("Assertion failed at line", __LINE__)

program array_param_test

  use array_param_test_mod
  use skywalker

  implicit none

  character(len=255)                   :: input_file
  type(ensemble_result_t)              :: load_result
  type(ensemble_t)                     :: ensemble
  real(swp), allocatable, dimension(:) :: values
  type(input_t)                        :: input
  type(output_t)                       :: output

  if (command_argument_count() /= 1) then
    print *, "array_param_test_f90: usage:"
    print *, "array_param_test_f90: <input.yaml>"
    stop
  end if

  call get_command_argument(1, input_file)

  ! Print a banner with Skywalker's version info.
  call print_banner()

  ! Load the ensemble. Any error encountered is fatal.
  print *, "array_param_test_f90: Loading ensemble from ", trim(input_file)
  load_result = load_ensemble(trim(input_file), "settings")

  ! Make sure everything is as it should be.
  if (load_result%error_code /= SW_SUCCESS) then
    print *, "array_param_test_f90: ", trim(load_result%error_message)
    stop
  end if

  ! ensemble information
  ensemble = load_result%ensemble
  assert(ensemble%size == 11)
  do while (ensemble%next(input, output))
    assert(input%has_array("p1"))
    call input%get_array("p1", values)
    assert(size(values) == 4)
    assert(values(1) >= 1.0_swp);
    assert(values(1) <= 11.0_swp);
    assert(approx_equal(values(2), 1.0_swp+values(1)));
    assert(approx_equal(values(3), 2.0_swp+values(1)));
    assert(approx_equal(values(4), 3.0_swp+values(1)));
    deallocate(values)

    assert(input%has_array("p2"))
    call input%get_array("p2", values)
    assert(size(values) == 3)
    assert(approx_equal(values(1), 4.0_swp))
    assert(approx_equal(values(2), 5.0_swp))
    assert(approx_equal(values(3), 6.0_swp))
    deallocate(values)

    assert(input%has("p3"))
    assert(approx_equal(input%get("p3"), 3.0_swp))

    ! Add a "qoi" metric set to 4.
    call output%set("qoi", 4.0_swp)
  end do

  ! Now we write out a Python module containing the output data.
  call ensemble%write("array_param_test_f90.py")

  ! Clean up.
  call ensemble%free()
end program
