# Quick Start

Once you've successfully [installed Skywalker](installation.md), it's time to
write your first Skywalker program. Sometimes we refer to such a program as a
*driver*, since it's really just a way to run an algorithm or parameterization
that we want to study.

Let's get started.

## Step 1: Design an Experiment

For simplicity, we are going to study a function of two variables

$$
f(x, y) = y \sin(x) - x \cos(y)
$$

over the domain $[-2\pi, 2\pi]\times[−2\pi, 2\pi]$. Our input parameters are
$x$ and $y$, and our output variable is $f$. We will sample points on a uniform
grid of evenly-spaced $x$ and $y$ values.

Suppose we want to sample $f$ at 100 $x$ values and 100 $y$ values, tracing out
the surface it represents above the euclidean plane. Here's a YAML file that
sets up the calculation of $f$ on these $(x, y)$ points:

=== "surface.yaml"
    ``` linenums="1"
    input:
      lattice:
        x: [-6.2831853, 6.2831853, 0.0628]
        y: [-6.2831853, 6.2831853, 0.0628]
    ```

Line 2 indicates that all values of $x$ are combined with all values of $y$.
Lines 3 and 4 define 100 uniformly-spaced values between $−2\pi$ and $2\pi$ for
$x$ and $y$. You can find detailed explanations of all these things in the
[Input Format (YAML)](input.md) section.

This YAML input file reflects the design of our experiment. Now we just need a
Skywalker program that computes $f$ at each of the points we've specified.

## Step 2: Write a Skywalker Program

A Skywalker program creates an **ensemble** of complete sets of inputs (in this
case, the point $(x, y)$ in $\mathbb{R}^2$). Each point represents a **member**
of the ensemble. The program loops over each ensemble members $(x, y)$ and
computes the output $f$ for it. Finally, it writes all of its ensemble data
(all values of $x$, $y$, and $f$) to a [Python module](output.md) that can
be used for postprocessing. The data in the module is ordered in such a way that
each output is associated with its inputs.

Here's a program that does all of these things, written in C, C++, and Fortran.
Pick your favorite language, paste the corresponding code into a text editor,
and save it to a file with the indicated name.

=== "C (surface.c)"
    ``` c linenums="1"
    #include <skywalker.h>
    #include <math.h>

    int main(int argc, char **argv) {
      // Specify the name of the input file.
      const char *input_file = "surface.yaml";

      // Load the ensemble, exiting if the operation fails.
      sw_ensemble_result_t load_result = sw_load_ensemble(input_file, NULL);
      if (load_result.error_code != SW_SUCCESS) {
        fprintf(stderr, "Error: %s", load_result.error_message);
        exit(-1);
      }

      // Iterate over all members of the ensemble.
      sw_ensemble_t *ensemble = load_result.ensemble;
      sw_input_t *input;
      sw_output_t *output;
      while (sw_ensemble_next(ensemble, &input, &output)) {
        // Fetch input values.
        sw_input_result_t result = sw_input_get(input, "x");
        if (result.error_code != SW_SUCCESS) {
          fprintf(stderr, "Error: %s", load_result.error_message);
          exit(-1);
        }
        sw_real_t x = result.value;
        result = sw_input_get(input, "y");
        if (result.error_code != SW_SUCCESS) {
          fprintf(stderr, "Error: %s", load_result.error_message);
          exit(-1);
        }
        sw_real_t y = result.value;

        // Compute f(x, y).
        sw_real_t f = y * sin(x) - x * cos(y);

        // Store f as an output variable.
        sw_output_set(output, "f", f);
      }

      // Write a Python module containing the input/output data.
      const char *output_file = "surface.py";
      sw_write_result_t w_result = sw_ensemble_write(ensemble, output_file);
      if (w_result.error_code != SW_SUCCESS) {
        fprintf(stderr, "Error: %s", load_result.error_message);
        exit(-1);
      }

      // Clean up.
      sw_ensemble_free(ensemble);

      return 0;
    }
    ```
=== "C++ (surface.cpp)"
    ``` c++ linenums="1"
    #include <skywalker.hpp>
    #include <cmath>
    #include <iostream>

    int main(int argc, char **argv) {
      // Specify the name of the input file.
      std::string input_file = "surface.yaml";

      // Load the ensemble, exiting if the operation fails.
      skywalker::Ensemble *ensemble = nullptr;
      try {
        ensemble = skywalker::load_ensemble(input_file);

        // Iterate over all members of the ensemble. We define a lambda function
        // that operates on the input and output variables for each member.
        ensemble->process([](const skywalker::Input &input,
                             skywalker::Output &output) {
          // Fetch input values.
          skywalker::Real x = input.get("x");
          skywalker::Real y = input.get("y");

          // Compute f(x, y).
          skywalker::Real f = y * std::sin(x) - x * std::cos(y);

          // Store f as an output variable.
          output.set("f", f);
        });

        // Write a Python module containing the input/output data.
        std::string output_file = "surface.py";
        ensemble->write(output_file);

        // Clean up.
        delete ensemble;
      } catch (skywalker::Exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        exit(-1);
      }

      return 0;
    }
    ```
=== "Fortran (surface.F90)"
    ``` fortran linenums="1"
    program surface
      use skywalker
      implicit none

      character(len=255)      :: input_file, output_file
      type(ensemble_result_t) :: load_result
      type(ensemble_t)        :: ensemble
      type(input_t)           :: input
      type(output_t)          :: output
      real(swp)               :: x, y, f

      ! Specify the name of the input file.
      input_file = "surface.yaml"

      ! Load the ensemble, exiting if the operation fails.
      load_result = load_ensemble(trim(input_file))
      if (load_result%error_code /= SW_SUCCESS) then
        print *, "Error: ", trim(load_result%error_message)
        stop
      end if

      ! Iterate over all members of the ensemble.
      ensemble = load_result%ensemble
      do while (ensemble%next(input, output))
        ! Fetch inputs.
        x = input%get("x")
        y = input%get("y")

        ! Compute f(x, y).
        f = y * sin(x) - x * cos(y)

        ! Store f as an output variable.
        call output%set("f", f);
      end do

      ! Write out a Python module.
      output_file = "surface.py"
      call ensemble%write(output_file)

      ! Clean up.
      call ensemble%free();
    end program
    ```

The [API](api.md) section discusses each of the elements in this program. For
now, let's just try to build it and run it.

## Step 3: Build the Program

Everyone has their own setup for building programs, so let's keep it simple:
we'll compile the program and link it against the appropriate Skywalker library
(or libraries) in one step:

=== "C"
    ```
    cc surface.c -I/path/to/include -L/path/to/lib -lskywalker_double -o surface
    ```
=== "C++"
    ```
    c++ surface.cpp -std=c++11 -I/path/to/include -L/path/to/lib -lskywalker_double -o surface
    ```
=== "Fortran"
    ```
    gfortran surface.F90 -I/path/to/include -L/path/to/lib -lskywalker_f90_double -lskywalker_double -o surface
    ```

If you use a different compiler, substitute it above. A few things to note:

* The C++ Skywalker program uses lambda functions, which requires at least
  C++11.

* The `-I` flags above indicate the location of the Skywalker headers
  (`skywalker.h`, `skywalker.hpp`) and modules (`skywalker.mod`) you installed
  in the [Installation](installation.md) section. So if you set Skywalker's
  `CMAKE_INSTALL_PREFIX` to `/usr/local`, you would use `-I/usr/local/include`.

* Likewise, the `-L` flags tell the linker where to find the Skywalker
  libraries. If your `CMAKE_INSTALL_PREFIX` is `/usr/local`, indicate this with
  `-L/usr/local/lib`.

* The `-l` parameter indicates the library to link your program against. C and
  C++ programs must be linked against `libskywalker_double.a`, while Fortran
  programs must additionally use `libskywalker_f90_double.a`. If you've
  configured Skywalker to use single precision floating point numbers with
  `-DSKYWALKER_PRECISION=single`, replace `double` with `single`.

If all goes well, you'll end up with a `surface` executable.

## Step 4: Run the Experiment

Now it's time to see the program in action. Make sure your `surface`
executable is in the same directory as your `surface.yaml` input file,
and run it without arguments:

```
./surface
```

Hopefully, the program runs to completion, generating a `surface.py` text
file containing the data. You can open up this file in an editor to see how it
looks. The format of this file is described [here](output.md).

## Step 5: Analyze the Results

Here's a simple Python 3 program you can run in the same directory as your
`surface.py` file to generate a surface plot of $f(x, y)$ over the domain
$[-2\pi, 2\pi] \times [-2\pi, 2\pi]$.

=== "plot_surface.py"
    ``` python
    import matplotlib.pyplot as plt
    import numpy as np

    # Extract data from surface.py.
    import surface
    x = np.array(surface.input.x)
    y = np.array(surface.input.y)
    f = np.array(surface.output.f)

    # Plot the contours of f(x, y).
    plt.tricontour(x, y, f)
    plt.colorbar()

    # Display the plot.
    plt.show()
    ```

The script uses [matplotlib](https://matplotlib.org), which you can often
install with a command like

```
pip3 install matplotlib
```

Run the script to see the plot.

```
python3 plot_surface.py
```

Here's how it looks:

![f(x,y)](images/surface.png)

That's it. Congratulations--you've successfully used Skywalker to sample a
multivariate function over regularly spaced intervals. If you like, you can
continue through the rest of the documentation to learn how things work. You can
also take a look at [a more involved example](example.md) if that's your style.
