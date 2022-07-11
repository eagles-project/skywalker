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

// This program tests Skywalker's C interface with an ensemble that contains
// both lattice and enumerated parameters.

#include <skywalker.h>

#include <assert.h>
#include <string.h>
#include <tgmath.h>

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
  fprintf(stderr, "mixed_test: Loading ensemble from %s\n", input_file);
  sw_ensemble_result_t load_result = sw_load_ensemble(input_file, "settings");
  assert(load_result.settings != NULL);
  assert(load_result.ensemble != NULL);
  assert(load_result.error_code == SW_SUCCESS);
  assert(load_result.error_message == NULL);

  // Make sure everything is as it should be.

  // Settings
  sw_settings_t *settings = load_result.settings;
  sw_settings_result_t settings_result;

  assert(sw_settings_has(settings, "s1"));
  settings_result = sw_settings_get(settings, "s1");
  assert(settings_result.error_code == 0);
  assert(strcmp(settings_result.value, "primary") == 0);
  assert(settings_result.error_message == NULL);

  assert(sw_settings_has(settings, "s2"));
  settings_result = sw_settings_get(settings, "s2");
  assert(settings_result.error_code == 0);
  assert(strcmp(settings_result.value, "algebraic") == 0);
  assert(settings_result.error_message == NULL);

  assert(!sw_settings_has(settings, "nonexistent_setting"));

  // Ensemble data
  sw_ensemble_t *ensemble = load_result.ensemble;
  assert(sw_ensemble_size(ensemble) == 726);
  sw_input_t *input;
  sw_output_t *output;
  while (sw_ensemble_next(ensemble, &input, &output)) {
    sw_input_result_t in_result;

    // Fixed parameters
    assert(sw_input_has(input, "f1"));
    in_result = sw_input_get(input, "f1");
    assert(in_result.error_code == SW_SUCCESS);
    assert(approx_equal(in_result.value, 1.0));
    assert(in_result.error_message == NULL);

    assert(sw_input_has(input, "f2"));
    in_result = sw_input_get(input, "f2");
    assert(in_result.error_code == SW_SUCCESS);
    assert(approx_equal(in_result.value, 2.0));
    assert(in_result.error_message == NULL);

    assert(sw_input_has(input, "f3"));
    in_result = sw_input_get(input, "f3");
    assert(in_result.error_code == SW_SUCCESS);
    assert(approx_equal(in_result.value, 3.0));
    assert(in_result.error_message == NULL);

    // Lattice parameters
    assert(sw_input_has(input, "l1"));
    in_result = sw_input_get(input, "l1");
    assert(in_result.error_code == SW_SUCCESS);
    assert(in_result.value >= 0.0);
    assert(in_result.value <= 10.0);
    assert(in_result.error_message == NULL);

    assert(sw_input_has(input, "l2"));
    in_result = sw_input_get(input, "l2");
    assert(in_result.error_code == SW_SUCCESS);
    assert(in_result.value >= 1e1);
    assert(in_result.value <= 1e11);
    assert(in_result.error_message == NULL);

    // Enumerated parameters
    assert(sw_input_has(input, "e1"));
    in_result = sw_input_get(input, "e1");
    assert(in_result.error_code == SW_SUCCESS);
    assert(in_result.value >= 1.0);
    assert(in_result.value <= 6.0);
    assert(in_result.error_message == NULL);

    assert(sw_input_has(input, "e2"));
    in_result = sw_input_get(input, "e2");
    assert(in_result.error_code == SW_SUCCESS);
    assert(in_result.value >= 0.05);
    assert(in_result.value <= 0.3 + SW_EPSILON);
    assert(in_result.error_message == NULL);

    // Look for a parameter that doesn't exist.
    assert(!sw_input_has(input, "invalid_param"));
    in_result = sw_input_get(input, "invalid_param");
    assert(in_result.error_code == SW_PARAM_NOT_FOUND);
    assert(in_result.error_message != NULL);

    // Add a "qoi" metric set to 4.
    sw_output_set(output, "qoi", 4.0);

    // Add an array value.
    sw_real_t qoi_array[10];
    for (int i = 0; i < 10; ++i) {
      qoi_array[i] = i;
    }
    sw_output_set_array(output, "qoi_array", qoi_array, 10);
  }

  // Write out a Python module.
  sw_write_result_t w_result = sw_ensemble_write(ensemble, "mixed_test.py");
  if (w_result.error_code != SW_SUCCESS) {
    fprintf(stderr, "%s\n", w_result.error_message);
    exit(-1);
  }

  // Clean up.
  sw_ensemble_free(ensemble);
}
