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
!
! Questions? Contact Jeffrey Johnson (jeff@cohere-llc.com)
! -------------------------------------------------------------------------

! This module contains data structures that allow Fortran modules to access
! skywalker's ensemble input and output data.
module skywalker

  use iso_c_binding

  implicit none

  ! Skywalker precision (swp): real kind used by skywalker
  integer, parameter :: swp = c_real

  ! Error codes -- see skywalker.h.in for descriptions
  integer, parameter :: sw_success = 0
  integer, parameter :: sw_yaml_file_not_found = 1
  integer, parameter :: sw_invalid_yaml = 2
  integer, parameter :: sw_invalid_param_type = 3
  integer, parameter :: sw_invalid_param_value = 4
  integer, parameter :: sw_invalid_settings_block = 5
  integer, parameter :: sw_settings_not_found = 6
  integer, parameter :: sw_invalid_param_name = 7
  integer, parameter :: sw_param_not_found = 8
  integer, parameter :: sw_too_many_lattice_params = 9
  integer, parameter :: sw_invalid_enumeration = 10
  integer, parameter :: sw_ensemble_too_large = 11
  integer, parameter :: sw_empty_ensemble = 12
  integer, parameter :: sw_write_failure = 13

  ! This type represents an ensemble that has been loaded from a skywalker input
  ! YAML file. It's an opaque type whose innards cannot be manipulated.
  type :: ensemble_t
    type(c_ptr)       :: ptr
    integer(c_size_t) :: size  ! number of members
  contains
    ! Iterates over ensemble members
    procedure :: next => ensemble_next
    ! Writes a Python module containing input/output data to a file, halting
    ! on failure
    procedure :: write => ensemble_write
    ! Writes a Python module containing input/output data to a file
    procedure :: write_module => ensemble_write_module
    ! Destroys an ensemble, freeing all allocated resources. Use at the end of
    ! a driver program, or when a fatal error has occurred.
    procedure :: free => ensemble_free
  end type ensemble_t

  ! This opaque type stores named settings intended for use with Skywalker
  ! driver programs.
  type :: settings_t
    type(c_ptr) :: ptr, ensemble_ptr
  contains
    ! Fetches a user-defined parameter.
    procedure :: has => settings_has
    procedure :: get => settings_get
    procedure :: get_param => settings_get_param
  end type

  ! This type stores the result of an attempt to fetch a setting.
  type :: settings_result_t
    character(len=255) :: value         ! fetched value (if error_code == 0)
    integer            :: error_code    ! error code indicating success or failure
    character(len=255) :: error_message ! text description of error
  end type settings_result_t

  ! Input data for simulations. Opaque type.
  type :: input_t
    type(c_ptr) :: ptr, ensemble_ptr
  contains
    ! Fetches a user-defined parameter.
    procedure :: has => input_has
    procedure :: get => input_get
    procedure :: get_param => input_get_param
    procedure :: has_array => input_has_array
    procedure :: get_array => input_get_array
    procedure :: get_array_param => input_get_array_param
  end type input_t

  ! This type stores the result of an attempt to fetch a (scalar) input
  ! parameter.
  type :: input_result_t
    real(c_real)       :: value         ! fetched value (if error_code == 0)
    integer(c_int)     :: error_code    ! error code indicating success or failure
    character(len=255) :: error_message ! text description of error
  end type input_result_t

  ! This type stores the result of an attempt to fetch a input array parameter.
  type :: input_array_result_t
    real(c_real), dimension(:), pointer :: values ! fetched values (if error_code == 0)
    integer(c_size_t)                   :: size   ! number of values (if error_code == 0)
    integer(c_int)                      :: error_code    ! error code indicating success or failure
    character(len=255)                  :: error_message ! text description of error
  end type input_array_result_t

  ! Output data for simulations. Opaque type.
  type :: output_t
    type(c_ptr) :: ptr
  contains
    ! Adds a named metric to the output data.
    procedure :: set => output_set
    ! Adds a vector of named metric to the output data.
    procedure :: set_array => output_set_array
  end type output_t

  ! This type stores the result of an attempt to store an output metric.
  type :: output_result_t
    integer            :: error_code    ! error code indicating success or failure
    character(len=255) :: error_message ! text description of error
  end type output_result_t

  ! This type contains all data loaded from an ensemble, including an error code
  ! and description of any issues encountered loading the ensemble. Do not
  ! attempt to free any of these resources.
  type :: ensemble_result_t
    ! The settings associated with the driver program
    type(settings_t) :: settings
    ! The ensemble loaded (if no error occurred)
    type(ensemble_t) :: ensemble
    ! An error code indicating any problems encountered loading the ensemble
    ! (zero = success, non-zero = failure)
    integer :: error_code
    ! A string describing any error encountered, or NULL if error_code == 0.
    character(len=255) :: error_message
  end type ensemble_result_t

  ! This type stores the result of an attempt to write an ensemble's data to
  ! a Python module.
  type :: write_result_t
    integer(c_int)     :: error_code    ! error code indicating success or failure
    character(len=255) :: error_message ! text description of error
  end type write_result_t

  interface

    subroutine sw_print_banner() bind(c)
    end subroutine

    subroutine sw_load_ensemble_f90(yaml_file, settings_block, &
                                    settings, ensemble, error_code, &
                                    error_message) bind(c)
      use iso_c_binding, only: c_ptr, c_int
      type(c_ptr), value, intent(in) :: yaml_file, settings_block
      type(c_ptr), intent(out) :: settings, ensemble, error_message
      integer(c_int), intent(out) :: error_code
    end subroutine

    logical(c_bool) function sw_settings_has(settings, name) bind(c)
      use iso_c_binding, only: c_ptr, c_bool
      type(c_ptr), value, intent(in) :: settings, name
    end function

    subroutine sw_settings_get_f90(settings, name, &
                                   value, error_code, error_message) bind(c)
      use iso_c_binding, only: c_ptr, c_int, c_double, c_float
      type(c_ptr), value, intent(in) :: settings, name
      type(c_ptr), intent(out) :: value
      integer(c_int), intent(out) :: error_code
      type(c_ptr), intent(out) :: error_message
    end subroutine

    integer(c_size_t) function sw_ensemble_size(ensemble) bind(c)
      use iso_c_binding, only: c_ptr, c_size_t
      type(c_ptr), value, intent(in) :: ensemble
    end function

    logical(c_bool) function sw_ensemble_next(ensemble, input, output) bind(c)
      use iso_c_binding, only: c_ptr, c_bool
      type(c_ptr), value, intent(in)  :: ensemble
      type(c_ptr),        intent(out) :: input, output
    end function

    logical(c_bool) function sw_input_has(input, name) bind(c)
      use iso_c_binding, only: c_ptr, c_bool
      type(c_ptr), value, intent(in) :: input, name
    end function

    logical(c_bool) function sw_input_has_array(input, name) bind(c)
      use iso_c_binding, only: c_ptr, c_bool
      type(c_ptr), value, intent(in) :: input, name
    end function

    subroutine sw_input_get_f90(input, name, &
                                value, error_code, error_message) bind(c)
      use iso_c_binding, only: c_ptr, c_int, c_double, c_float
      type(c_ptr), value, intent(in) :: input, name
      real(c_real), intent(out) :: value
      integer(c_int), intent(out) :: error_code
      type(c_ptr), intent(out) :: error_message
    end subroutine

    subroutine sw_input_get_array_f90(input, name, values, size, &
                                      error_code, error_message) bind(c)
      use iso_c_binding, only: c_ptr, c_int, c_size_t, c_double, c_float
      type(c_ptr), value, intent(in) :: input, name
      type(c_ptr), intent(out) :: values
      integer(c_size_t), intent(out) :: size
      integer(c_int), intent(out) :: error_code
      type(c_ptr), intent(out) :: error_message
    end subroutine

    subroutine sw_output_set(output, name, value) bind(c)
      use iso_c_binding, only: c_ptr, c_double, c_float
      type(c_ptr), value, intent(in) :: output
      type(c_ptr), value, intent(in) :: name
      real(c_real), value, intent(in) :: value
    end subroutine

    subroutine sw_output_set_array_f90(output, name, values, size) bind(c)
      use iso_c_binding, only: c_ptr, c_size_t
      type(c_ptr), value, intent(in) :: output
      type(c_ptr), value, intent(in) :: name
      type(c_ptr), value, intent(in) :: values
      integer(c_size_t),  intent(in) :: size
    end subroutine

    logical(c_bool) function sw_ensemble_ext(ensemble, input, output) bind(c)
      use iso_c_binding, only: c_bool, c_ptr
      type(c_ptr), value, intent(in) :: ensemble
      type(c_ptr), intent(out) :: input, output
    end function

    subroutine sw_ensemble_write_f90(ensemble, filename, error_code, &
                                     error_message) bind(c)
      use iso_c_binding, only: c_ptr, c_int
      type(c_ptr), value, intent(in) :: ensemble
      type(c_ptr), value, intent(in) :: filename
      integer(c_int), intent(out) :: error_code
      type(c_ptr), intent(out) :: error_message
    end subroutine

    subroutine sw_ensemble_free(ensemble) bind(c)
      use iso_c_binding, only: c_ptr
      type(c_ptr), value, intent(in) :: ensemble
    end subroutine

  end interface

contains

  ! Prints a banner containing Skywalker's version info to stderr.
  subroutine print_banner()
    call sw_print_banner()
  end subroutine

  ! Reads an ensemble from a YAML input file, returning a pointer to the ensemble
  ! (if the read was successful). The settings_block argument indicates the name
  ! of the YAML block to read to retrieve settings for the driver program using
  ! Skywalker.
  function load_ensemble(yaml_file, settings_block) result(e_result)
    use iso_c_binding, only: c_ptr, c_null_ptr
    implicit none

    character(len=*), intent(in)           :: yaml_file
    character(len=*), intent(in), optional :: settings_block

    type(ensemble_result_t) :: e_result
    type(c_ptr) :: c_settings_block, c_err_msg

    if (present(settings_block)) then
      c_settings_block = f_to_c_string(settings_block)
    else
      c_settings_block = c_null_ptr
    end if
    call sw_load_ensemble_f90(f_to_c_string(yaml_file), &
                              c_settings_block, &
                              e_result%settings%ptr, e_result%ensemble%ptr, &
                              e_result%error_code, c_err_msg)

    if (e_result%error_code == SW_SUCCESS) then
      e_result%ensemble%size = sw_ensemble_size(e_result%ensemble%ptr)
      ! Set the ensemble pointer on settings to allow proper destruction
      ! using settings%get().
      e_result%settings%ensemble_ptr = e_result%ensemble%ptr
    else
      e_result%error_message = c_to_f_string(c_err_msg)
    end if

  end function

  ! Returns .true. if the setting with the given name exists within the given
  ! settings instance, false otherwise.
  function settings_has(settings, name) result(has)
    use iso_c_binding, only: c_ptr
    implicit none

    class(settings_t), intent(in) :: settings
    character(len=*), intent(in)  :: name
    logical(c_bool) :: has

    has = sw_settings_has(settings%ptr, f_to_c_string(name))
  end function

  ! Retrieves the setting with the given name, returning a result that can
  ! be checked for errors that occur.
  function settings_get_param(settings, name) result(s_result)
    use iso_c_binding, only: c_ptr
    implicit none

    class(settings_t), intent(in) :: settings
    character(len=*), intent(in)  :: name

    type(settings_result_t) :: s_result
    type(c_ptr) :: c_value, c_err_msg

    call sw_settings_get_f90(settings%ptr, f_to_c_string(name), &
                             c_value, s_result%error_code, c_err_msg)
    if (s_result%error_code == SW_SUCCESS) then
      s_result%value = c_to_f_string(c_value)
    else
      s_result%error_message = c_to_f_string(c_err_msg)
    end if
  end function

  ! Retrieves the setting with the given name, halting the program if an
  ! error occurs.
  function settings_get(settings, name) result(str)
    use iso_c_binding, only: c_ptr
    implicit none

    class(settings_t), intent(in) :: settings
    character(len=*), intent(in)  :: name

    type(settings_result_t) :: s_result
    character(len=255) :: str

    s_result = settings%get_param(name)
    if (s_result%error_code /= SW_SUCCESS) then
      print *, s_result%error_message
      call sw_ensemble_free(settings%ensemble_ptr)
      stop
    else
      str = s_result%value
    end if
  end function

  ! Returns .true. if the input parameter with the given name exists within the
  ! given input instance, false otherwise.
  function input_has(input, name) result(has)
    use iso_c_binding, only: c_ptr
    implicit none

    class(input_t), intent(in) :: input
    character(len=*), intent(in)  :: name
    logical(c_bool) :: has

    has = sw_input_has(input%ptr, f_to_c_string(name))
  end function

  ! Retrieves the input parameter with the given name.
  function input_get_param(input, name) result(i_result)
    use iso_c_binding, only: c_ptr
    implicit none

    class(input_t), intent(in)   :: input
    character(len=*), intent(in) :: name

    type(input_result_t) :: i_result
    type(c_ptr) :: c_err_msg

    call sw_input_get_f90(input%ptr, f_to_c_string(name), &
                          i_result%value, i_result%error_code, &
                          c_err_msg)
    if (i_result%error_code /= SW_SUCCESS) then
      i_result%error_message = c_to_f_string(c_err_msg)
    end if
  end function

  ! Retrieves the input parameter with the given name, halting the program
  ! on failure.
  function input_get(input, name) result(val)
    use iso_c_binding, only: c_ptr, c_real
    implicit none

    class(input_t), intent(in)   :: input
    character(len=*), intent(in) :: name

    type(input_result_t) :: i_result
    real(c_real) :: val

    i_result = input%get_param(name)
    if (i_result%error_code /= SW_SUCCESS) then
      print *, i_result%error_message
      call sw_ensemble_free(input%ensemble_ptr)
      stop
    else
      val = i_result%value
    end if
  end function

  ! Returns .true. if an input array parameter with the given name exists within
  ! the given input instance, false otherwise.
  function input_has_array(input, name) result(has)
    use iso_c_binding, only: c_ptr
    implicit none

    class(input_t), intent(in) :: input
    character(len=*), intent(in)  :: name
    logical(c_bool) :: has

    has = sw_input_has_array(input%ptr, f_to_c_string(name))
  end function

  ! Retrieves the input array parameter with the given name.
  function input_get_array_param(input, name) result(i_result)
    use iso_c_binding, only: c_ptr
    implicit none

    class(input_t), intent(in)   :: input
    character(len=*), intent(in) :: name

    type(input_array_result_t) :: i_result
    type(c_ptr) :: c_values, c_err_msg

    call sw_input_get_array_f90(input%ptr, f_to_c_string(name), &
                                c_values, i_result%size, &
                                i_result%error_code, c_err_msg)
    if (i_result%error_code == SW_SUCCESS) then
      call c_f_pointer(c_values, i_result%values, [i_result%size])
    else
      i_result%error_message = c_to_f_string(c_err_msg)
    end if
  end function

  ! Retrieves the input array parameter with the given name, halting on failure.
  subroutine input_get_array(input, name, values)
    use iso_c_binding, only: c_ptr, c_real
    implicit none

    class(input_t), intent(in)   :: input
    character(len=*), intent(in) :: name
    real(c_real), allocatable, dimension(:), intent(inout) :: values

    type(input_array_result_t) :: i_result

    i_result = input%get_array_param(name)
    if (i_result%error_code /= SW_SUCCESS) then
      print *, i_result%error_message
      call sw_ensemble_free(input%ensemble_ptr)
      stop
    else
      if (allocated(values)) then
        deallocate(values)
      end if
      allocate(values(i_result%size))
      values(:) = i_result%values(:)
    end if
  end subroutine

  ! This function sets a quantity with the given name and value to the given
  ! output instance. This operation cannot fail under normal circumstances.
  subroutine output_set(output, name, value)
    use iso_c_binding, only: c_double, c_float
    implicit none

    class(output_t), intent(in)  :: output
    character(len=*), intent(in) :: name
    real(c_real), intent(in)     :: value

    call sw_output_set(output%ptr, f_to_c_string(name), value)
  end subroutine

  ! Sets an array of quantities with the given name and values to the given
  ! output instance. This operation cannot fail under normal circumstances.
  subroutine output_set_array(output, name, values)
    use iso_c_binding, only: c_double, c_float
    implicit none

    class(output_t), intent(in)  :: output
    character(len=*), intent(in) :: name
    real(c_real), target, intent(in), dimension(:) :: values
    integer(c_size_t)              :: c_values_len
    c_values_len = size(values)
    call sw_output_set_array_f90(output%ptr, f_to_c_string(name), &
      c_loc(values), c_values_len)
  end subroutine

  ! Iterates over the members of the ensemble, returning the input and output
  ! data structures for the next member.
  function ensemble_next(ensemble, input, output) result(next)
    implicit none

    class(ensemble_t), intent(in) :: ensemble
    type(input_t), intent(out)    :: input
    type(output_t), intent(out)   :: output
    logical(c_bool) :: next

    next = sw_ensemble_next(ensemble%ptr, input%ptr, output%ptr)
    ! Set the ensemble pointer on input to allow proper destruction using
    ! input%get().
    input%ensemble_ptr = ensemble%ptr
  end function

  ! Writes input and output data within the ensemble to a Python module stored
  ! in the file with the given name.
  function ensemble_write_module(ensemble, module_filename) result(w_result)
    implicit none

    class(ensemble_t), intent(in) :: ensemble
    character(len=*), intent(in)  :: module_filename

    type(write_result_t) :: w_result
    type(c_ptr) :: c_err_msg

    call sw_ensemble_write_f90(ensemble%ptr, f_to_c_string(module_filename), &
                               w_result%error_code, c_err_msg)
    if (w_result%error_code /= SW_SUCCESS) then
      w_result%error_message = c_to_f_string(c_err_msg)
    end if
  end function

  ! Writes input and output data within the ensemble to a Python module stored
  ! in the file with the given name, halting on failure.
  subroutine ensemble_write(ensemble, module_filename)
    implicit none

    class(ensemble_t), intent(in) :: ensemble
    character(len=*), intent(in)  :: module_filename

    type(write_result_t) :: w_result
    type(c_ptr) :: c_err_msg

    call sw_ensemble_write_f90(ensemble%ptr, &
                               f_to_c_string(trim(module_filename)), &
                               w_result%error_code, c_err_msg)
    if (w_result%error_code /= SW_SUCCESS) then
      w_result%error_message = c_to_f_string(c_err_msg)
      print *, w_result%error_message
      call sw_ensemble_free(ensemble%ptr)
      stop
    end if
  end subroutine

  ! Destroys an ensemble, freeing all allocated resources.
  subroutine ensemble_free(ensemble)
    implicit none

    class(ensemble_t), intent(in) :: ensemble

    call sw_ensemble_free(ensemble%ptr)
  end subroutine

  ! This helper function converts the given C string to a Fortran string.
  function c_to_f_string(c_string) result(f_string)
    use, intrinsic :: iso_c_binding
    implicit none
    type(c_ptr), value, intent(in) :: c_string
    character(len=:), pointer      :: f_ptr
    integer(c_size_t)              :: c_string_len
    character(len=255)             :: f_string

    interface
        function c_strlen(str_ptr) bind (c, name = "strlen" ) result(len)
        use, intrinsic :: iso_c_binding
            type(c_ptr), value     :: str_ptr
            integer(c_size_t)      :: len
        end function c_strlen
    end interface

    call c_f_pointer(c_string, f_ptr)
    c_string_len = c_strlen(c_string)
    f_string = f_ptr(1:c_string_len)

  end function

  ! This helper function converts the given Fortran string to a C string.
  function f_to_c_string(f_string) result(c_string)
    use, intrinsic :: iso_c_binding
    implicit none
    character(len=*), target :: f_string
    character(len=:), pointer :: f_ptr
    type(c_ptr) :: c_string

    interface
        function sw_new_c_string_f90(f_str_ptr, f_str_len) bind (c) result(c_string)
        use, intrinsic :: iso_c_binding
            type(c_ptr), value :: f_str_ptr
            integer(c_int), value :: f_str_len
            type(c_ptr) :: c_string
        end function
    end interface

    f_ptr => f_string
    c_string = sw_new_c_string_f90(c_loc(f_ptr), len(f_string))
  end function

end module
