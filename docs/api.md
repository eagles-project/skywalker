# Skywalker's API

Skywalker offers a simple interface for writing programs that operate on entire
ensembles. You can access everything in this interface by including the correct
C/C++ header file or using the appropriate Fortran module in your program.

=== "C"

    ```
    #include <skywalker.h>
    ```

=== "C++"

    ```
    #include <skywalker.hpp>
    ```

=== "Fortran"

    ```
    use skywalker
    ```

## Data Types

## Error Handling

Skywalker's C interface types that store the result of each function call. These
types provide access to an error code and a descriptive error string that can
be used to properly handle any issues that occur.

The Fortran interface offers similar result types, but also offers "shorthand"
versions of functions that simply halt the program when an error occurs.

The C++ interface does not define types to store the results of its functions.
Instead, it directly returns the data requested, throwing an exception
containing a string description if any issue occurs.

## Reading Input

=== "C"

    ```
    sw_ensemble_result_t sw_load_ensemble(const char *yaml_file,
                                          const char *settings_block);
    ```

=== "C++"

    ```
    namespace skywalker {
      Ensemble* load_ensemble(const std::string& yaml_file,
                              const std::string& settings_block);
    }
    ```

=== "Fortran"

    ```
    function load_ensemble(yaml_file, settings_block) result(e_result)
      character(len=*), intent(in) :: yaml_file
      character(len=*), intent(in) :: settings_block
      type(ensemble_result_t) :: e_result
    end function
    ```

## Processing an Ensemble Member

## Writing Input

## Miscellaneous

