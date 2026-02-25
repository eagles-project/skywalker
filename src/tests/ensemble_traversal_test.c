//-------------------------------------------------------------------------
// Copyright (c) 2021,
// National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//
// Under the terms of Contract DE-NA0003525 with NTESS, the U.S. Government
// retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Sandia Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//-------------------------------------------------------------------------

// This program tests Skywalker's C interface with an ensemble defined by an
// enumeration.

#include <skywalker.h>

#include <assert.h>
#include <string.h>
#include <math.h>
void usage(const char *prog_name) {
  fprintf(stderr, "%s: usage:\n", prog_name);
  fprintf(stderr, "%s <input.yaml>\n", prog_name);
  exit(0);
}

static bool approx_equal(sw_real_t x, sw_real_t y) {
  return (fabs(x - y) < 1e-14);
}

int main(int argc, char **argv) {

  if (argc == 1) {
    usage((const char*)argv[0]);
  }
  const char* input_file = argv[1];

  // Print a banner with Skywalker's version info.
  sw_print_banner();

  // Load the ensemble. Any error encountered is fatal.
  fprintf(stderr, "ensemble_traversal_test: Loading ensemble from %s\n", input_file);
  sw_ensemble_result_t load_result = sw_load_ensemble(input_file, "settings");
  if (load_result.error_code != SW_SUCCESS) {
    fprintf(stderr, "%s\n", load_result.error_message);
    exit(-1);
  }

  // Ensemble data: 4 arrays each containing 4 values, and 4 scalars.
  // We produce identically structured output containing the squares of each input value.
  sw_ensemble_t *ensemble = load_result.ensemble;
  assert(sw_ensemble_size(ensemble) == 4);
  sw_input_t *input;
  sw_output_t *output;
  while (sw_ensemble_next(ensemble, &input, &output)) {
    sw_input_result_t in_result;
    sw_input_array_result_t in_array_result;

    assert(sw_input_has(input, "scalars"));
    sw_real_t scalar = sw_input_get(input, "scalars").value;
    sw_output_set(output, "scalars_squared", scalar * scalar);

    assert(sw_input_has_array(input, "arrays"));
    in_array_result = sw_input_get_array(input, "arrays");
    assert(in_array_result.error_code == SW_SUCCESS);
    assert(in_array_result.size == 4);
    sw_real_t array_squared[4];
    for (int i = 0; i < 4; ++i) {
      array_squared[i] = in_array_result.values[i] * in_array_result.values[i];
    }
    sw_output_set_array(output, "arrays_squared", array_squared, 4);
  }

  // Now traverse the ensemble again and check its inputs and outputs.
  while (sw_ensemble_next(ensemble, &input, &output)) {
    const char *name;
    sw_real_t scalar_value;
    int num_scalars = 0;
    while (sw_input_next_scalar(input, &name, &scalar_value)) {
      assert(!strcmp(name, "scalars"));
      printf("scalars[%d] = %g\n", num_scalars, scalar_value);
      ++num_scalars;
    }
    assert(num_scalars == 1);

    num_scalars = 0;
    while (sw_output_next_scalar(output, &name, &scalar_value)) {
      assert(!strcmp(name, "scalars_squared"));
      printf("scalars_squared[%d] = %g\n", num_scalars, scalar_value);
      ++num_scalars;
    }
    assert(num_scalars == 1);

    sw_real_t *array_values;
    size_t array_size;
    int num_arrays = 0;
    while (sw_input_next_array(input, &name, &array_values, &array_size)) {
      assert(!strcmp(name, "arrays"));
      assert(array_size == 4);
      printf("arrays[%d] = {%g, %g, %g, %g}\n", num_arrays, array_values[0],
             array_values[1], array_values[2], array_values[3]);
      ++num_arrays;
    }
    assert(num_arrays == 1);

    num_arrays = 0;
    while (sw_output_next_array(output, &name, &array_values, &array_size)) {
      assert(!strcmp(name, "arrays_squared"));
      assert(array_size == 4);
      printf("arrays_squared[%d] = {%g, %g, %g, %g}\n", num_arrays, array_values[0],
             array_values[1], array_values[2], array_values[3]);
      ++num_arrays;
    }
    assert(num_arrays == 1);
  }

  // Write out a Python module.
  sw_write_result_t w_result = sw_ensemble_write(ensemble,
                                                 "array_param_test.py");
  if (w_result.error_code != SW_SUCCESS) {
    fprintf(stderr, "%s\n", w_result.error_message);
    exit(-1);
  }

  // Clean up.
  sw_ensemble_free(ensemble);
}
