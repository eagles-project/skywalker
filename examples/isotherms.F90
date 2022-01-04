!-------------------------------------------------------------------------
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
!-------------------------------------------------------------------------

module isotherms_mod
  use iso_c_binding, only: c_float, c_double
  use skywalker
  implicit none

contains

  subroutine usage()
    print *, "isotherm_f90: calculates the pressure of a Van der Waals gas given"
    print *, "its volume and temperature."
    print *, "isotherm_f90: usage:"
    print *, "isotherm_f90 <input.yaml>"
    stop
  end subroutine

  subroutine fatal_error(message, line)
    character(len=*), intent(in) :: message
    integer :: line

    print *, message, line
    stop
  end subroutine

  ! Determines the output_file name corresponding to the given name of the
  ! input file.
  function output_file_name(input_file) result(output_name)
    character(len=*), intent(in) :: input_file

    character(len=255) :: output_name
    integer            :: dot_index

    ! Generate an output file name based on the name of the input file.
    dot_index = index(trim(input_file), ".")
    if (dot_index > 0) then
      output_name = input_file(1:dot_index-1) // "_f90.py"
    else
      output_name = input_file // "_f90.py"
    end if
  end function

end module isotherms_mod

program isotherms
  use isotherms_mod
  use skywalker
  implicit none

  ! Universal gas constant
  real(swp), parameter    :: R = 8.31446261815324_swp

  character(len=255)      :: input_file, output_file
  type(ensemble_result_t) :: load_result
  type(ensemble_t)        :: ensemble
  type(input_t)           :: input
  type(output_t)          :: output
  real(swp)               :: V, T, p, a, b

  if (command_argument_count() /= 1) then
    call usage()
  end if
  call get_command_argument(1, input_file)

  ! Load the ensemble. Any error encountered is fatal.
  print *, "isotherms_f90: Loading ensemble from ", trim(input_file), "..."
  load_result = load_ensemble(trim(input_file))
  if (load_result%error_code /= SW_SUCCESS) then
    print *, "isotherms_f90: ", trim(load_result%error_message)
    stop
  end if

  ensemble = load_result%ensemble
  print *, "isotherms_f90: found ", ensemble%size, " ensemble members."

  ! Iterate over all members of the ensemble.
  do while (ensemble%next(input, output))
    ! Fetch inputs.
    V = input%get("V") ! gas (molar) volume [m3]
    T = input%get("T") ! gas temperature [K]

    ! Fetch Van der Waals parameters if they're present.
    a = 0.0_swp
    b = 0.0_swp
    if (input%has("a")) a = input%get("a")
    if (input%has("b")) b = input%get("b")

    ! Compute p(V, T).
    p = R * T / (V - b) - a/(V**2)

    ! Stash the computed pressure in the member's output.
    call output%set("p", p);
  end do

  ! Write out a Python module.
  output_file = output_file_name(input_file);
  print *, "isotherms_f90: Writing data to ", output_file, "..."
  call ensemble%write(output_file)

  ! Clean up.
  call ensemble%free();
end program
