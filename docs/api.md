# The Skywalker API

Skywalker offers a simple interface for writing programs that operate on entire
ensembles. You can access everything in this interface by including the correct
C/C++ header file or using the appropriate Fortran module in your program.

=== "C"

    ``` c
    #include <skywalker.h>
    ```

=== "C++"

    ``` c++
    #include <skywalker.hpp>
    ```

=== "Fortran"

    ``` fortran
    use skywalker
    ```

Skywalker is written in C, and the C interface is the source of truth for the
Fortran and C++ bindings. The Fortran `skywalker` module exposes all the same
types and functions as the C interface. The C++ interface wraps all of the C
types in a `skywalker.hpp` header file, exposing then in the `skywalker`
namespace.

A Skywalker program typically consists of the following parts:

1. Ensemble and settings information are loaded from an input YAML file
2. Settings are used to configure the program to run the loaded ensemble
3. One by one, ensemble members are processed by the program. Ensemble input
   parameters are retrieved and used to run a simulation, and then ensemble
   output parameters are written and stored.
4. The resulting ensemble is written to a Python module. All input and output
   parameters are written and stored in a way that allows them to be
   programmatically accessed by a postprocessing script.

Here we describe the types and functions you can use to assemble your own
Skywalker program.

## Data Types

### Real number type

Skywalker supports exactly one value type that stores real-valued ensemble
parameters. By default this type is a double-precision floating point number,
but it can be configured for single precision with the `SKYWALKER_PRECISION`
CMake variable.

=== "C"

    ``` c
    typedef double sw_real_t;
    ```

=== "C++"

    ``` c++
    using Real = sw_real_t;
    ```

=== "Fortran"

    ``` fortran
    ! Skywalker precision (swp): real kind used by skywalker
    integer, parameter :: swp
    ```

In Fortran, the `swp` kind is used to store input parameters and output values,
and is set to either the `c_double` or `c_float` interoperable types defined
by the Fortran 2003 ISO C bindings.

The C interface also defines the following macros:

* `SW_EPSILON`: the "machine epsilon" value, an upper bound on the relative
  relative approximation error due to rounding in floating point arithmetic.
  This maps either to `FLT_EPSILON` or `DBL_EPSILON` as defined in `float.h`,
  depending on Skywalker's precision.
* `SW_MIN`: the minimum representable floating point number. Maps to
  `FLT_MIN` or `DBL_MIN` as defined in `float.h`.
* `SW_MAX`: the maximum representable floating point number. Maps to
  `FLT_MAX` or `DBL_MAX` as defined in `float.h`.

### Interface types

Each of the essential concepts in the library has an associated type.

* An **Ensemble** stores a set of input and output data for each of its members.

=== "C"

    ``` c
    typedef struct sw_ensemble_t sw_ensemble_t;
    ```
=== "C++"

    ``` c++
    class Ensemble final {
     public:
      // Returns the settings associated with this ensemble.
      const Settings& settings() const;

      // Iterates over all ensemble members, applying the given function f to
      // each input/output pair.
      void process(std::function<void(const Input&, Output&)> f);

      // Returns the size of the ensemble (number of members).
      size_t size() const;

      // Writes input and output data within the ensemble to a Python module stored
      // in the file with the given name.
      void write(const std::string& module_filename) const;
    };
    ```
=== "Fortran"

    ``` fortran
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
    ```

* The **Settings** type stores configuration data or metadata associated with
  your program and how it treats ensemble members. All of these data are strings
  that can be retrieved by name.

=== "C"
    ``` c
    typedef struct sw_settings_t sw_settings_t;
    ```
=== "C++"
    ``` c++
    class Settings final {
     public:

      // Returns true if the setting with the given name exists within the given
      // settings instance, false otherwise.
      bool has(const std::string& name) const;

      // Retrieves a (string-valued) setting with the given name, throwing an
      // exception if it doesn't exist.
      std::string get(const std::string& name) const;
    };
    ```
=== "Fortran"
    ``` fortran
    type :: settings_t
      type(c_ptr) :: ptr, ensemble_ptr
    contains
      procedure :: has => settings_has
      procedure :: get => settings_get
      procedure :: get_param => settings_get_param
    end type
    ```

* **Ensemble member input** is stored in a set of named ensemble parameters
  within a dedicated type. Input data cannot be modified--it can only be read.

=== "C"
    ``` c
    typedef struct sw_input_t sw_input_t;
    ```
=== "C++"
    ``` c++
    class Input final {
     public:
      Input();

      // Returns true if the input parameter with the given name exists within
      // the given input instance, false otherwise.
      bool has(const std::string& name) const;

      // Retrieves a (real-valued) parameter with the given name, throwing an
      // exception if it doesn't exist.
      Real get(const std::string& name) const;

      // Returns true if an input array parameter with the given name exists
      // within the given input instance, false otherwise.
      bool has_array(const std::string& name) const;

      // Retrieves a (real-valued) array parameter with the given name, throwing
      // an exception if it doesn't exist.
      std::vector<Real> get_array(const std::string& name) const;
    };
    ```
=== "Fortran"
    ``` fortran
    type :: input_t
      type(c_ptr) :: ptr, ensemble_ptr
    contains
      procedure :: has => input_has
      procedure :: get => input_get
      procedure :: get_param => input_get_param
      procedure :: has_array => input_has_array
      procedure :: get_array => input_get_array
      procedure :: get_array_param => input_get_array_param
    end type input_t
    ```

* **Ensemble member output** is also stored in a set of named ensemble
  parameters within a dedicated type. Unlike input data, output ensemble
  parameters can only be written.

=== "C"
    ``` c
    typedef struct sw_output_t sw_output_t;
    ```
=== "C++"
    ``` c++
    class Output final {
     public:
      Output();

      // Sets a (real-valued) parameter with the given name, throwing an
      // exception if not successful.
      void set(const std::string& name, Real value) const;

      // Sets (real-valued) parameters with the given name, throwing an
      // exception if not successful.
      void set(const std::string& name, const std::vector<Real> &values) const;
    };
    ```
=== "Fortran"
    ``` fortran
    type :: output_t
      type(c_ptr) :: ptr
    contains
      ! Adds a named metric to the output data.
      procedure :: set => output_set
      ! Adds a vector of named metric to the output data.
      procedure :: set_array => output_set_array
    end type output_t
    ```

Notice that many of the Fortran types are actually classes with bound
procedures with implementations, indicated by the `procedure => implementation`
ѕyntax. Below, we refer to the functions and subroutines that implement the
procedures. The [examples](https://github.com/eagles-project/skywalker/tree/main/examples)
and [tests](https://github.com/eagles-project/skywalker/tree/main/src/tests)
illustrate how these procedures are invoked, as this syntax is unfortunately not
very clear.

### Result types

The C and Fortran interfaces define types that store the results of operations
like loading ensembles from input, fetching input parameters, and writing
output parameters. These "result" types contain information that can be used
for handling errors, and each type has `result_t` at the end of its name.

## Error Handling

The types in Skywalker's C interface contain fields that can be used to properly
handle any errors that occur while reading input or constructing ensembles:

* `error_code`: an integer that identifies a specific type of error that you
  can use to programmatically handle error conditions. The integer can be
  compared with the fields in the `sw_error_code_t` enumerated type in
  `skywalker.h` or the integer parameters defined in the Fortran module.
* `error_message`: a descriptive error string that can be printed to tell a user
  what went wrong

The Fortran interface offers similar result types with these same fields, but
also offers "shorthand" versions of functions that simply halt the program with
the Fortran `STOP` command when an error occurs.

The C++ interface does not define types to store the results of its functions.
Instead, it directly returns the data requested, throwing an exception (of type
`skywalker::Exception`, a subclass of `std::exception`) containing a string
description if any issue occurs.

The [examples](https://github.com/eagles-project/skywalker/tree/main/examples)
and [tests](https://github.com/eagles-project/skywalker/tree/main/src/tests)
in the repository demonstrate how these error handling mechanisms work in
their respective languages.

## Loading an Ensemble and Settings

You can load an ensemble and settings for your program from a YAML input
file with a single function call.

=== "C"

    ``` c
    sw_ensemble_result_t sw_load_ensemble(const char *yaml_file,
                                          const char *settings_block);
    ```

=== "C++"

    ``` c++
    namespace skywalker {
      Ensemble* load_ensemble(const std::string& yaml_file,
                              const std::string& settings_block = "");
    }
    ```

=== "Fortran"

    ``` fortran
    function load_ensemble(yaml_file, settings_block) result(e_result)
      character(len=*), intent(in) :: yaml_file
      character(len=*), intent(in), optional :: settings_block
      type(ensemble_result_t) :: e_result
    end function
    ```

* `yaml_file` is the name of the YAML input file to be read. It can be
  an absolute or relative path to a file on disk.
* `settings_block` is the name of the block in the YAML file from which settings
  are read. This can be the name of your program, or just `"settings"` if you
  want a mneumonic name. This argument is optional: If this string is `NULL` or
  blank in C, or not given in C++ or Fortran, Skywalker does not attempt to read
  any settings from the file.

Settings can be used to create a single YAML input file that defines ensembles
for several Skywalker programs (e.g. for cross validating different methods or
models). Each program can define its own settings in a program-specific block
whose name is indicated using the `settings_block` parameter.

The C and Fortran interfaces return a result that contains the ensemble and
settings, as well as error handling information.

=== "C"

    ``` c
    typedef struct sw_ensemble_result_t {
      // The settings associated with the driver program
      sw_settings_t *settings;
      // The ensemble loaded (or NULL on failure)
      sw_ensemble_t *ensemble;
      // An error code indicating any problems encountered loading the ensemble
      // (zero = success, non-zero = failure)
      int error_code;
      // A string describing any error encountered, or NULL if error_code == 0.
      const char* error_message;
    } sw_ensemble_result_t;
    ```

=== "Fortran"

    ``` fortran
    type :: ensemble_result_t
      ! The settings associated with the driver program
      type(settings_t) :: settings
      ! The ensemble loaded (if no error occurred)
      type(ensemble_t) :: ensemble
      ! The ensemble's type
      integer :: type
      ! An error code indicating any problems encountered loading the ensemble
      ! (zero = success, non-zero = failure)
      integer :: error_code
      ! A string describing any error encountered, or NULL if error_code == 0.
      character(len=255) :: error_message
    end type ensemble_result_t
    ```

The C++ interface returns a pointer to an `Ensemble` instance. Settings are
available within this instance. If an error occurs, a `skywalker::Exception`
is thrown containing an error message string identical to the `error_message`
field of the result type returned by the C and Fortran interfaces.

## Applying Program Settings

If your program can run in more than one configuration, you can select a
configuration using the data specified in the settings you've loaded. Settings
are stored in strings, and you can retrieve a setting by name from the settings
variable you've loaded.

Settings can be queried with a `has` function that returns true if a setting
with the given name is found and false otherwise.

=== "C"
    ``` c
    // Returns true if the setting with the given name exists within the given
    // settings instance, false otherwise.
    bool sw_settings_has(sw_settings_t *settings, const char* name);
    ```
=== "C++"
    ``` c++
    class Settings final {
      ...
      // Returns true if the setting with the given name exists within the given
      // settings instance, false otherwise.
      bool has(const std::string& name) const;
      ...
    };
    ```
=== "Fortran"
    ``` fortran
    ! Returns .true. if the setting with the given name exists within the given
    ! settings instance, false otherwise.
    function settings_has(settings, name) result(has)
      class(settings_t), intent(in) :: settings
      character(len=*), intent(in)  :: name
      logical(c_bool) :: has
    end function
    ```

A setting with the given name can be fetched.

=== "C"
    ``` c
    // Retrieves the setting with the given name.
    sw_settings_result_t sw_settings_get(sw_settings_t *settings,
                                         const char *name);
    ```
=== "C++"
    ``` c++
    class Settings final {
      ...
      // Retrieves a (string-valued) setting with the given name, throwing an
      // exception if it doesn't exist.
      std::string get(const std::string& name) const {
      ...
    };
    ```
=== "Fortran"
    ``` fortran
    ! Retrieves the setting with the given name, returning a result that can
    ! be checked for errors that occur.
    function settings_get_param(settings, name) result(s_result)
      class(settings_t), intent(in) :: settings
      character(len=*), intent(in)  :: name
      type(settings_result_t) :: s_result
    end function
    ```

Fetching a setting fails if a setting with the given name doesn't exist. The
C and Fortran interfaces define a result type that allows this situation to
be handled.

=== "C"
    ``` c
    typedef struct sw_setting_result_t {
      const char* value;         // fetched value (if error_code == 0)
      int error_code;            // error code indicating success or failure
      const char* error_message; // text description of error
    } sw_settings_result_t;
    ```
=== "Fortran"
    ``` fortran
    type :: settings_result_t
      character(len=255) :: value         ! fetched value (if error_code == 0)
      integer            :: error_code    ! error code indicating success or failure
      character(len=255) :: error_message ! text description of error
    end type settings_result_t
    ```

On failure, the C++ interface throws a `skywalker::Exception` with a string
description identical to the `error_message` description in the C and Fortran
result types. For brevity, the Fortran interface also offers a function that
halts your program if a setting is not found:

=== "Fortran"
    ``` fortran
    ! Retrieves the setting with the given name, halting the program if an
    ! error occurs.
    function settings_get(settings, name) result(str)
      class(settings_t), intent(in) :: settings
      character(len=*), intent(in)  :: name
      character(len=255) :: str
    end function
    ```

## Processing an Ensemble Member

The bulk of your Skywalker program is concerned with processing each member of
an ensemble. To do this, you must

* loop over the ensemble, retrieving the input and output variables for each
  member
* read input parameters from the member's input variable
* use the input parameters to compute output.
* set the output parameters in the member's output variable

### Looping over ensemble members

Skywalker handles the process of looping over the ensemble for you. In C and
Fortran, you can construct a loop that calls a function to get the input and
output parameters for each member, terminating when there are no members left.

=== "C"
    ``` c
    // Iterates over the inputs and outputs in an ensemble, making them available
    // one at a time for computation. This function returns true once for each
    // member of an ensemble and false once the ensemble's members have been
    // traversed. Use it as a predicate in a while loop in which inputs and outputs
    // are processed.
    bool sw_ensemble_next(sw_ensemble_t *ensemble,
                          sw_input_t **input,
                          sw_output_t **output);
    ```
=== "Fortran"
    ``` fortran
    ! Iterates over the inputs and outputs in an ensemble, making them available
    ! one at a time for computation. This function returns true once for each
    ! member of an ensemble and false once the ensemble's members have been
    ! traversed. Use it as a predicate in a do while loop in which inputs and
    ! outputs are processed.
    function ensemble_next(ensemble, input, output) result(next)
      class(ensemble_t), intent(in) :: ensemble
      type(input_t), intent(out)    :: input
      type(output_t), intent(out)   :: output
      logical(c_bool) :: next
    end function
    ```

In C++, you can write your own function that performs the processing, and pass
that function to a method on your `Ensemble` object. This executes your function
on the input and output data for each member. The function you define takes a
`const` reference to an `Input` object and a non-`const` reference to an
`Output` object, and returns nothing.

=== "C++"
    ``` c++
    class Ensemble final {
      ...
      // Iterates over all ensemble members, applying the given function f to
      // each input/output pair.
      void process(std::function<void(const Input&, Output&)> f);
      ...
    };
    ```

The [examples](https://github.com/eagles-project/skywalker/tree/main/examples)
and [tests](https://github.com/eagles-project/skywalker/tree/main/src/tests)
illustrate how this is done.

### Reading input parameters

To read an input parameter from an ensemble member, you can retrieve its value
using its name.

It's easy to check whether the ensemble member has a given parameter:

=== "C"
    ``` c
    // Returns true if a (scalar) input parameter with the given name exists
    // within the given input instance, false otherwise.
    bool sw_input_has(sw_input_t *input, const char* name);
    ```
=== "C++"
    ``` c++
    class Input final {
      ...
      // Returns true if the input parameter with the given name exists within
      // the given input instance, false otherwise.
      bool has(const std::string& name) const;
      ...
    };
    ```
=== "Fortran"
    ``` fortran
    ! Returns .true. if the input parameter with the given name exists within the
    ! given input instance, false otherwise.
    function input_has(input, name) result(has)
      class(input_t), intent(in) :: input
      character(len=*), intent(in)  :: name
      logical(c_bool) :: has
    end function
    ```

Similarly, it's easy to fetch the parameter:

=== "C"
    ``` c
    // Retrieves the (scalar) input parameter with the given name.
    sw_input_result_t sw_input_get(sw_input_t *input, const char *name);
    ```
=== "C++"
    ``` c++
    class Input final {
      ...
      // Retrieves a (real-valued) parameter with the given name, throwing an
      // exception if it doesn't exist.
      Real get(const std::string& name) const;
      ...
    };
    ```
=== "Fortran"
    ``` fortran
    ! Retrieves the input parameter with the given name.
    function input_get_param(input, name) result(i_result)
      class(input_t), intent(in)   :: input
      character(len=*), intent(in) :: name
      type(input_result_t) :: i_result
    end function
    ```

Fetching a parameter fails if the parameter doesn't exist within the ensemble.
The C and Fortran interfaces return a result type that allows you to check
whether the operation succeeded.

=== "C"
    ``` c
    typedef struct sw_input_result_t {
      sw_real_t value;           // fetched value (if error_code == 0)
      int error_code;            // error code indicating success or failure
      const char* error_message; // text description of error
    } sw_input_result_t;
    ```
=== "Fortran"
    ``` fortran
    type :: input_result_t
      real(c_real)       :: value         ! fetched value (if error_code == 0)
      integer(c_int)     :: error_code    ! error code indicating success or failure
      character(len=255) :: error_message ! text description of error
    end type input_result_t
    ```

In C++, a `skywalker::Exception` is thrown in the case of failure, with an
error string identical to the `error_message` field of the corresponding C and
Fortran result types.

The Fortran interface also offers you a "shortcut" that directly fetches your
input parameter, halting your program with `STOP` on failure.

=== "Fortran"
    ``` fortran
    ! Retrieves the input parameter with the given name, halting the program
    ! on failure.
    function input_get(input, name) result(val)
      class(input_t), intent(in)   :: input
      character(len=*), intent(in) :: name
      real(c_real) :: val
    end function
    ```

### Reading input array parameters

Skywalker offers you the ability to work with arrays of real-valued input
parameters identified by a single name. These array parameters work the same as
their scalar counterparts. Their memory is managed by Skywalker, so there's no
need for you to perform any memory allocation.

As with scalar input parameters, you can check to see whether an input array
parameter exists within an ensemble member.

=== "C"
    ``` c
    // Returns true if an input array parameter with the given name exists within
    // the given input instance, false otherwise.
    bool sw_input_has_array(sw_input_t *input, const char* name);
    ```
=== "C++"
    ``` c++
    class Input final {
      ...
      // Returns true if an input array parameter with the given name exists within
      // the given input instance, false otherwise.
      bool has_array(const std::string& name) const;
      ...
    };
    ```
=== "Fortran"
    ``` fortran
    ! Returns .true. if an input array parameter with the given name exists within
    ! the given input instance, false otherwise.
    function input_has_array(input, name) result(has)
      class(input_t), intent(in) :: input
      character(len=*), intent(in)  :: name
      logical(c_bool) :: has
    end function
    ```

The process of retrieving input array parameters works the same way as it does
for scalar input parameters.

=== "C"
    ``` c
    // Retrieves the (array-valued) input parameter with the given name.
    sw_input_array_result_t sw_input_get_array(sw_input_t *input, const char *name);
    ```
=== "C++"
    ``` c++
    class Input final {
      ...
      // Retrieves a (real-valued) array parameter with the given name, throwing an
      // exception if it doesn't exist.
      std::vector<Real> get_array(const std::string& name) const;
    };
    ```
=== "Fortran"
    ``` fortran
    ! Retrieves the input array parameter with the given name.
    function input_get_array_param(input, name) result(i_result)
      class(input_t), intent(in)   :: input
      character(len=*), intent(in) :: name
      type(input_array_result_t) :: i_result
    end function
    ```

The C and Fortran interface define result types for fetching input array
parameters.

=== "C"
    ``` c
    typedef struct sw_input_array_result_t {
      sw_real_t *values;         // fetched values (if error_code == 0)
      size_t size;               // number of values (if error_code == 0)
      int error_code;            // error code indicating success or failure
      const char* error_message; // text description of error
    } sw_input_array_result_t;
    ```
=== "Fortran"
    ``` fortran
    type :: input_array_result_t
      real(c_real), dimension(:), pointer :: values ! fetched values (if error_code == 0)
      integer(c_size_t)                   :: size   ! number of values (if error_code == 0)
      integer(c_int)                      :: error_code    ! error code indicating success or failure
      character(len=255)                  :: error_message ! text description of error
    end type input_array_result_t
    ```

As with scalar input parameters, the C++ interface throws a
`skywalker::Exception` if it cannot retrieve a given input array parameter. And,
as in the scalar case, the Fortran interface also defines a subroutine that
tries to fetch an array parameter and halts with `STOP` on failure.

=== "Fortran"
    ``` fortran
    ! Retrieves the input array parameter with the given name, halting on
    ! failure.
    subroutine input_get_array(input, name, values)
      class(input_t), intent(in)   :: input
      character(len=*), intent(in) :: name
      real(c_real), allocatable, dimension(:), intent(inout) :: values
    end subroutine
    ```

If you use this subroutine, you must deallocate the `values` array when you're
finished with it.

### Computing output parameters from input parameters

This is where you do your thing. Nobody knows your job better than you! Remember
that all input and output parameters are floating point numbers, so if your
work uses integer inputs or outputs, you must perform all necessary type
casting.

### Setting output parameters

When you've computed an output value from one or more input values, you'll want
to store that value by name in the ensemble member's output data. To do this,
you can call a function to set the value of a named output parameter.

=== "C"
    ``` c
    // This function sets a quantity with the given name and value within the given
    // output instance. This operation cannot fail under normal circumstances.
    void sw_output_set(sw_output_t *output, const char *name, sw_real_t value);
    ```
=== "C++"
    ``` c++
    class Output final {
      ...
      // Sets a (real-valued) parameter with the given name. This operation
      // cannot fail under normal circumstances.
      void set(const std::string& name, Real value) const;
      ...
    };

    ```
=== "Fortran"
    ``` fortran
    ! Sets a quantity with the given name and value within the given output
    ! instance. This operation cannot fail under normal circumstances.
    subroutine output_set(output, name, value)
      class(output_t), intent(in)  :: output
      character(len=*), intent(in) :: name
      real(c_real), intent(in)     :: value
    end subroutine
    ```

The operation of setting an output parameter can't fail under normal
circumstances (sufficient memory, good hardware, etc). Skywalker handles all
the bookkeeping details.

### Setting output array parameters

Output array parameters work the same way as input array parameters: they're
just output values that share a common name. You can set an array-valued output
parameter by calling the appropriate function or subroutine.

=== "C"
    ``` c
    // This function sets an array of quantities with the given name and values
    // within the given output instance. This operation cannot fail under normal
    // circumstances.
    void sw_output_set_array(sw_output_t *output, const char *name,
                             const sw_real_t *values, size_t size);
    ```
=== "C++"
    ``` c++
    class Output final {
      ...
      // Sets (real-valued) parameters in an array with the given name. This
      // operation cannot fail under normal circumstances.
      void set(const std::string& name, const std::vector<Real> &values) const;
      ...
    };
    ```
=== "Fortran"
    ``` fortran
    ! Sets an array of quantities with the given name and values to the given
    ! output instance. This operation cannot fail under normal circumstances.
    subroutine output_set_array(output, name, values)
      class(output_t), intent(in)  :: output
      character(len=*), intent(in) :: name
      real(c_real), target, intent(in), dimension(:) :: values
    end subroutine
    ```

As with scalar output parameters, the operation of setting an output array
parameter cannot fail under normal circumstances.

## Writing Ensemble Output

At the end of your program, you can call a function to write all your ensemble
data to a Python module that be postprocessed.

=== "C"
    ``` c
    // Writes input and output data within the ensemble to a Python module stored
    // in the file with the given name.
    sw_write_result_t sw_ensemble_write(sw_ensemble_t *ensemble,
                                        const char *module_filename);
    ```
=== "C++"
    ``` c++
    class Ensemble final {
      ...
      // Writes input and output data within the ensemble to a Python module stored
      // in the file with the given name.
      void write(const std::string& module_filename) const;
      ...
    };
    ```
=== "Fortran"
    ``` fortran
    ! Writes input and output data within the ensemble to a Python module stored
    ! in the file with the given name.
    function ensemble_write_module(ensemble, module_filename) result (w_result)
      class(ensemble_t), intent(in) :: ensemble
      character(len=*), intent(in)  :: module_filename
      type(write_result_t) :: w_result
    end subroutine
    ```

Because it attempts to write a file, this operation can fail. The C and Fortran
interfaces define a result type that can capture the information needed to
handle this failure.

=== "C"
    ``` c
    typedef struct sw_write_result_t {
      int error_code;            // error code indicating success or failure
      const char* error_message; // text description of error
    } sw_write_result_t;
    ```
=== "Fortran"
    ``` fortran
    type :: write_result_t
      integer(c_int)     :: error_code    ! error code indicating success or failure
      character(len=255) :: error_message ! text description of error
    end type write_result_t

    ```

As usual, the C++ interface throws a `skywalker::Exception` containing a
string identical to the `error_message` field found in the corresponding C and
Fortran result types. And, as usual, there is a Fortran subroutine that attempts
to write the ensemble data to the given file and halts on failure.

=== "Fortran"
    ``` fortran
    ! Writes input and output data within the ensemble to a Python module stored
    ! in the file with the given name, halting on failure.
    subroutine ensemble_write(ensemble, module_filename)
      class(ensemble_t), intent(in) :: ensemble
      character(len=*), intent(in)  :: module_filename
    end subroutine
    ```

### Cleanup

After you've written the Python module, you should free the resources your
ensemble uses by destroying it. In C and Fortran, you can do this with a
simple function call.

=== "C"
    ``` c
    // Destroys an ensemble, freeing its allocated resources. Use this at the end
    // of your driver program, or when a fatal error has been encountered.
    void sw_ensemble_free(sw_ensemble_t *ensemble);
    ```
=== "Fortran"
    ``` fortran
    ! Destroys an ensemble, freeing all allocated resources.
    subroutine ensemble_free(ensemble)
      class(ensemble_t), intent(in) :: ensemble
    end subroutine
    ```

In C++, you can simply `delete` the `Ensemble` pointer you obtained by calling
`skywalker::load_ensemble`, or you can use a smart pointer to store the
ensemble.

## Miscellaneous

In addition to the types and interfaces we've described, there are a few extra
functions you might find handy in your Skywalker program.

### Printing a banner

You can write a banner to the standard error strstream (`stderr` in C and C++)
with a call to the appropriate function.

=== "C"
    ``` c
    // Prints a banner containing Skywalker's version info to stderr.
    void sw_print_banner(void);
    ```
=== "C++"
    ``` c++
    namespace skywalker {
    ...
    // Prints a banner containing Skywalker's version info to stderr.
    void print_banner() {
    ...
    } // namespace skywalker
    ```
=== "Fortran"
    ``` fortran
    ! Prints a banner containing Skywalker's version info to stderr.
    subroutine print_banner()
    end subroutine
    ```

This banner prints out the version of Skywalker used by your program. This can
be helpful if you think you've encountered a bug in Skywalker, or if you're
trying to use a newer feature of the library.

### Getting the ensemble's size

Sometimes it helps to know how many members an ensemble contains.

=== "C"
    ``` c
    // Returns the size of the given ensemble.
    size_t sw_ensemble_size(sw_ensemble_t* ensemble);
    ```
=== "C++"
    ``` c++
    class Ensemble {
      ...
      // Returns the size of the ensemble (number of members).
      size_t size() const;
      ...
    };
    ```
=== "Fortran"
    The ensemble's size is stored in the `size` field of the `ensemble_t`
    derived type.

