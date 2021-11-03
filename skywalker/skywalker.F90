!> This module contains data structures that allow Fortran modules to access
!> skywalker's ensemble input and output data.
module skywalker

  use iso_c_binding
  use haero_precision, only: wp

  implicit none

  private

  public :: wp, input_data_t, output_data_t, ensemble_t, load_ensemble

  !> Working precision real kind
  integer, parameter :: wp = c_real

  !> This Fortran type is the equivalent of the C InputData struct.
  type :: input_data_t
    ! C pointer
    type(c_ptr) :: ptr
    ! Timestepping data
    real(c_real) :: dt, total_time
    ! atmospheric state parameters
    real(c_real) :: temperature, pressure, vapor_mixing_ratio, height, &
                    hydrostatic_dp, planetary_boundary_layer_height

    ! Modal aerosol number mixing ratios [# aero molecules / kg air]
    real(c_real), dimension(:), pointer :: interstitial_aero_nmrs, &
                                           cloud_aero_nmrs
    ! Aerosol mass mixing ratios [kg aerosol / kg air], indexed by mode and species
    real(c_real), dimension(:, :), pointer :: interstitial_aero_mmrs, &
                                              cloud_aero_mmrs
    ! Gas mass mixing ratios [kg gas / kg air]
    real(c_real), dimension(:), pointer :: gas_mmrs
  contains
    ! Fetches a user-defined parameter.
    procedure :: user_param => i_user_param
  end type input_data_t

  type :: output_data_t
    ! C pointer
    type(c_ptr) :: ptr
    ! Modal aerosol number concentrations [# aero molecules / kg air]
    real(c_real), dimension(:), pointer :: interstitial_aero_nmrs, &
                                           cloud_aero_nmrs
    ! Aerosol mass mixing ratios [kg aerosol / kg air], indexed by mode and species
    real(c_real), dimension(:, :), pointer :: interstitial_aero_mmrs, &
                                              cloud_aero_mmrs
    ! Gas mass mixing ratios [kg gas / kg air]
    real(c_real), dimension(:), pointer :: gas_mmrs
  contains
    ! Adds a named metric to the output data.
    procedure :: add_metric => o_add_metric
  end type output_data_t

  ! This type represents an ensemble and its corresponding input and output
  ! data for each member.
  type :: ensemble_t
    ! C pointer
    type(c_ptr) :: ptr
    ! Name of the aerosol process being studied by this ensemble
    character(len=255) :: program_name
    ! Number of parameters passed to the process
    integer :: num_program_params
    ! List of names of parameters passed to the process
    character(len=255), dimension(:), allocatable :: program_param_names
    ! List of values of parameters passed to the process
    character(len=255), dimension(:), allocatable :: program_param_values
    ! The number of members in the ensemble
    integer :: size
    ! Number of aerosol modes and populations, and number of gases
    integer(c_int) :: num_modes, num_populations, num_gases
    ! An array of input data for every member of the ensemble
    type(input_data_t), dimension(:), allocatable :: inputs
    ! An array of output data for every member of the ensemble
    type(output_data_t), dimension(:), allocatable :: outputs

  contains
    ! Writes a Python module containing input/output data to a file
    procedure :: write_py_module => e_write_py_module
    ! Frees resources allocated to the ensemble
    procedure :: free => e_free
  end type ensemble_t

  interface

    ! Loads and returns an ensemble from the given file, interpreting its data
    ! according to the given aerosol configuration.
    type(c_ptr) function sw_load_ensemble(aerosol_config, filename, model_impl) bind(c)
      use iso_c_binding, only: c_ptr
      type(c_ptr), value, intent(in) :: aerosol_config, filename, model_impl
    end function

    ! Returns the name of the selected process for the given ensemble.
    type(c_ptr) function sw_ensemble_program_name(ensemble) bind(c)
      use iso_c_binding, only: c_ptr, c_int
      type(c_ptr), value, intent(in) :: ensemble
    end function

    ! Returns the number of parameters passed to the selected process for the
    ! given ensemble.
    integer(c_int) function sw_ensemble_num_program_params(ensemble) bind(c)
      use iso_c_binding, only: c_ptr, c_int
      type(c_ptr), value, intent(in) :: ensemble
    end function

    ! Sets the given pointers to strings that contain the name and value for
    ! the parameter with the given index passed to an ensemble.
    subroutine sw_ensemble_get_program_param(ensemble, index, param_name, param_value) bind(c)
      use iso_c_binding, only: c_ptr, c_int
      type(c_ptr), value, intent(in) :: ensemble
      integer(c_int), value, intent(in) :: index
      type(c_ptr), intent(out) :: param_name
      type(c_ptr), intent(out) :: param_value
    end subroutine

    ! Returns the number of members in an ensemble
    integer(c_int) function sw_ensemble_size(ensemble) bind(c)
      use iso_c_binding, only: c_ptr, c_int
      type(c_ptr), value, intent(in) :: ensemble
    end function

    subroutine sw_ensemble_get_array_sizes(ensemble, num_modes, num_pops, num_gases) bind(c)
      use iso_c_binding, only: c_ptr, c_int
      type(c_ptr), value, intent(in) :: ensemble
      integer(c_int), intent(inout) :: num_modes, num_pops, num_gases
    end subroutine

    subroutine sw_ensemble_get_modal_aerosol_sizes(ensemble, aerosols_per_mode) bind(c)
      use iso_c_binding, only: c_ptr, c_int
      type(c_ptr), value, intent(in) :: ensemble
      type(c_ptr), value, intent(in) :: aerosols_per_mode
    end subroutine

    type(c_ptr) function sw_ensemble_input(ensemble, i) bind(c)
      use iso_c_binding, only: c_ptr, c_int
      type(c_ptr), value, intent(in) :: ensemble
      integer(c_int), value, intent(in) :: i
    end function

    subroutine sw_input_get_timestepping(input, dt, total_time) bind(c)
      use iso_c_binding, only: c_ptr, c_real
      type(c_ptr), value, intent(in) :: input
      real(c_real), intent(out) :: dt, total_time
    end subroutine

    subroutine sw_input_get_atmosphere(input, temperature, pressure, qv, height,&
                                       dp, pblh) bind(c)
      use iso_c_binding, only: c_ptr, c_real
      type(c_ptr), value, intent(in) :: input
      real(c_real), intent(out) :: temperature, pressure, qv, height, dp, pblh
    end subroutine

    subroutine sw_input_get_aerosols(input, int_aero_nmrs, cld_aero_nmrs, &
                                     int_aero_mmrs, cld_aero_mmrs) bind(c)
      use iso_c_binding, only: c_ptr
      type(c_ptr), value, intent(in) :: input
      type(c_ptr), value, intent(in) :: int_aero_nmrs, cld_aero_nmrs,&
                                        int_aero_mmrs, cld_aero_mmrs
    end subroutine

    subroutine sw_input_get_gases(input, gas_mmrs) bind(c)
      use iso_c_binding, only: c_ptr
      type(c_ptr), value, intent(in) :: input
      type(c_ptr), value, intent(in) :: gas_mmrs
    end subroutine

    subroutine sw_input_get_user_param(output, name, value) bind(c)
      use iso_c_binding, only: c_ptr, c_double, c_float
      type(c_ptr), value, intent(in) :: output
      type(c_ptr), value, intent(in) :: name
      real(c_real), intent(out) :: value
    end subroutine

    type(c_ptr) function sw_ensemble_output(ensemble, i) bind(c)
      use iso_c_binding, only: c_ptr, c_int
      type(c_ptr), value, intent(in) :: ensemble
      integer(c_int), value, intent(in) :: i
    end function

    subroutine sw_output_set_aerosols(output, int_aero_nmrs, cld_aero_nmrs, &
                                      int_aero_mmrs, cld_aero_mmrs) bind(c)
      use iso_c_binding, only: c_ptr
      type(c_ptr), value, intent(in) :: output
      type(c_ptr), value, intent(in) :: int_aero_nmrs, cld_aero_nmrs,&
                                        int_aero_mmrs, cld_aero_mmrs
    end subroutine

    subroutine sw_output_set_gases(output, gas_mmrs) bind(c)
      use iso_c_binding, only: c_ptr
      type(c_ptr), value, intent(in) :: output
      type(c_ptr), value, intent(in) :: gas_mmrs
    end subroutine

    subroutine sw_output_set_metric(output, name, value) bind(c)
      use iso_c_binding, only: c_ptr, c_double, c_float
      type(c_ptr), value, intent(in) :: output
      type(c_ptr), value, intent(in) :: name
      real(c_real), value, intent(in) :: value
    end subroutine

    subroutine sw_ensemble_write_py_module(ensemble, filename) bind(c)
      use iso_c_binding, only: c_ptr
      type(c_ptr), value, intent(in) :: ensemble
      type(c_ptr), value, intent(in) :: filename
    end subroutine

    subroutine sw_ensemble_free(ensemble) bind(c)
      use iso_c_binding, only: c_ptr
      type(c_ptr), value, intent(in) :: ensemble
    end subroutine

  end interface

contains

  ! Given an aerosol configuration string and a skywalker input file, fetch an
  ! ensemble's worth of input data. Here, model_impl is a string indicating
  ! which underlying aerosol mode skywalker will use to select an appropriate
  ! aerosol process. It's usually "haero" or "mam".
  function load_ensemble(aerosol_config, filename, model_impl) result(ensemble)
    use iso_c_binding, only: c_int, c_ptr, c_associated, c_loc
    use haero, only: f_to_c_string, c_to_f_string
    implicit none

    character(len=*), intent(in) :: aerosol_config
    character(len=*), intent(in) :: filename
    character(len=*), intent(in) :: model_impl

    type(ensemble_t) :: ensemble
    type(input_data_t) :: input
    type(output_data_t) :: output
    integer(c_int), dimension(:), pointer :: mode_array_sizes
    integer :: i, m, s, p, max_mode_size
    real(c_real), dimension(:), allocatable, target :: int_aero_data, cld_aero_data
    type(c_ptr) :: p_name, p_value

    ! Fetch the ensemble pointer from C.
    ensemble%ptr = sw_load_ensemble(f_to_c_string(aerosol_config), &
                                    f_to_c_string(filename), &
                                    f_to_c_string(model_impl))
    if (.not. c_associated(ensemble%ptr)) then
      print *, "Could not load a ", aerosol_config, " from ", filename
      stop
    end if

    ! Fetch the name of the process and its associated parameters
    ensemble%program_name = c_to_f_string(sw_ensemble_program_name(ensemble%ptr))
    ensemble%num_program_params = sw_ensemble_num_program_params(ensemble%ptr)
    if (ensemble%num_program_params > 0) then
      allocate(ensemble%program_param_names(ensemble%num_program_params))
      allocate(ensemble%program_param_values(ensemble%num_program_params))
      do p = 1, ensemble%num_program_params
        call sw_ensemble_get_program_param(ensemble%ptr, p, p_name, p_value)
        ensemble%program_param_names(p) = c_to_f_string(p_name)
        ensemble%program_param_values(p) = c_to_f_string(p_value)
      end do
    end if

    ! Extract metadata.
    call sw_ensemble_get_array_sizes(ensemble%ptr, ensemble%num_modes, &
                                     ensemble%num_populations, &
                                     ensemble%num_gases)
    allocate(mode_array_sizes(ensemble%num_modes))
    call sw_ensemble_get_modal_aerosol_sizes(ensemble%ptr, c_loc(mode_array_sizes))
    max_mode_size = 0
    do m = 1, ensemble%num_modes
      max_mode_size = max(max_mode_size, mode_array_sizes(m))
    end do
    allocate(int_aero_data(ensemble%num_populations))
    allocate(cld_aero_data(ensemble%num_populations))

    ! Size up the ensemble.
    ensemble%size = sw_ensemble_size(ensemble%ptr)
    allocate(ensemble%inputs(ensemble%size))
    allocate(ensemble%outputs(ensemble%size))

    ! Allocate input and output aerosol and gas arrays for the ensemble,
    ! and extract input data
    do i = 1, ensemble%size
      input = ensemble%inputs(i)
      input%ptr = sw_ensemble_input(ensemble%ptr, i)
      allocate(input%interstitial_aero_nmrs(ensemble%num_modes))
      allocate(input%cloud_aero_nmrs(ensemble%num_modes))
      allocate(input%interstitial_aero_mmrs(ensemble%num_modes, max_mode_size))
      allocate(input%cloud_aero_mmrs(ensemble%num_modes, max_mode_size))
      allocate(input%gas_mmrs(ensemble%num_gases))

      output = ensemble%outputs(i)
      output%ptr = sw_ensemble_output(ensemble%ptr, i)
      allocate(output%interstitial_aero_nmrs(ensemble%num_modes))
      allocate(output%cloud_aero_nmrs(ensemble%num_modes))
      allocate(output%interstitial_aero_mmrs(ensemble%num_modes, max_mode_size))
      allocate(output%cloud_aero_mmrs(ensemble%num_modes, max_mode_size))
      allocate(output%gas_mmrs(ensemble%num_gases))

      ! Timestepping data
      call sw_input_get_timestepping(input%ptr, input%dt, input%total_time)

      ! Atmosphere data
      call sw_input_get_atmosphere(input%ptr, input%temperature, input%pressure, &
        input%vapor_mixing_ratio, input%height, input%hydrostatic_dp, &
        input%planetary_boundary_layer_height)

      ! Aerosol data
      call sw_input_get_aerosols(input%ptr, &
        c_loc(input%interstitial_aero_nmrs), &
        c_loc(input%cloud_aero_nmrs), &
        c_loc(int_aero_data), c_loc(cld_aero_data))
      p = 1 ! population index
      do m = 1, ensemble%num_modes
        do s = 1, mode_array_sizes(m)
          input%interstitial_aero_mmrs(m, s) = int_aero_data(p)
          input%cloud_aero_mmrs(m, s) = cld_aero_data(p)
          p = p + 1
        end do
      end do

      ! Gas data
      call sw_input_get_gases(input%ptr, c_loc(input%gas_mmrs))

      ensemble%inputs(i) = input
      ensemble%outputs(i) = output
    end do

    ! Clean up.
    deallocate(int_aero_data, cld_aero_data, mode_array_sizes)
  end function

  ! Fetches a user-defined parameter.
  function i_user_param(input, name) result(val)
    use iso_c_binding, only: c_ptr, c_double, c_float
    use haero, only: f_to_c_string
    implicit none

    class(input_data_t), intent(in) :: input
    character(len=*), intent(in) :: name
    real(c_real) :: val

    call sw_input_get_user_param(input%ptr, f_to_c_string(name), val)
  end function

  ! Adds a value for a named metric to output data.
  subroutine o_add_metric(output, name, val)
    use iso_c_binding, only: c_ptr, c_double, c_float
    use haero, only: f_to_c_string
    implicit none

    class(output_data_t), intent(in) :: output
    character(len=*), intent(in) :: name
    real(c_real), intent(in) :: val

    call sw_output_set_metric(output%ptr, f_to_c_string(name), val)
  end subroutine

  ! Writes a Python module containing all input and output for the given
  ! ensemble to a file with the given name.
  subroutine e_write_py_module(ensemble, filename)
    use iso_c_binding, only: c_ptr
    use haero, only: f_to_c_string
    implicit none

    class(ensemble_t), intent(in) :: ensemble
    character(len=*), intent(in) :: filename

    integer i, m, s, p
    integer(c_int), dimension(:), pointer :: mode_array_sizes
    real(c_real), dimension(:), pointer :: int_aero_data, cld_aero_data

    ! Copy output data into place.
    allocate(int_aero_data(ensemble%num_populations))
    allocate(cld_aero_data(ensemble%num_populations))

    ! Aerosol data
    allocate(mode_array_sizes(ensemble%num_modes))
    call sw_ensemble_get_modal_aerosol_sizes(ensemble%ptr, c_loc(mode_array_sizes))
    do i = 1, ensemble%size
      p = 1 ! population index
      do m = 1, ensemble%num_modes
        do s = 1, mode_array_sizes(m)
          int_aero_data(p) = ensemble%outputs(i)%interstitial_aero_mmrs(m, s)
          cld_aero_data(p) = ensemble%outputs(i)%cloud_aero_mmrs(m, s)
          p = p + 1
        end do
      end do
      call sw_output_set_aerosols(ensemble%outputs(i)%ptr, &
        c_loc(ensemble%outputs(i)%interstitial_aero_nmrs), &
        c_loc(ensemble%outputs(i)%cloud_aero_nmrs), &
        c_loc(int_aero_data), c_loc(cld_aero_data))

      ! Gas data
      call sw_output_set_gases(ensemble%outputs(i)%ptr, &
        c_loc(ensemble%outputs(i)%gas_mmrs))
    end do

    ! Generate the Python module.
    call sw_ensemble_write_py_module(ensemble%ptr, f_to_c_string(filename))

    ! Clean up.
    deallocate(mode_array_sizes, int_aero_data, cld_aero_data)
  end subroutine

  ! Frees all resources allcated to the given ensemble object.
  subroutine e_free(ensemble)
    use iso_c_binding, only: c_ptr
    implicit none

    class(ensemble_t), intent(inout) :: ensemble
    integer :: i

    do i = 1, ensemble%size
      deallocate(ensemble%inputs(i)%interstitial_aero_nmrs)
      deallocate(ensemble%inputs(i)%cloud_aero_nmrs)
      deallocate(ensemble%inputs(i)%interstitial_aero_mmrs)
      deallocate(ensemble%inputs(i)%cloud_aero_mmrs)
      deallocate(ensemble%inputs(i)%gas_mmrs)
      deallocate(ensemble%outputs(i)%interstitial_aero_nmrs)
      deallocate(ensemble%outputs(i)%cloud_aero_nmrs)
      deallocate(ensemble%outputs(i)%interstitial_aero_mmrs)
      deallocate(ensemble%outputs(i)%cloud_aero_mmrs)
      deallocate(ensemble%outputs(i)%gas_mmrs)
    end do
    deallocate(ensemble%inputs)
    deallocate(ensemble%outputs)
    if (ensemble%num_program_params > 0) then
      deallocate(ensemble%program_param_names)
      deallocate(ensemble%program_param_values)
    end if
  end subroutine

end module
