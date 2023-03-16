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
#include <tgmath.h>

void usage(const char *prog_name) {
  fprintf(stderr, "%s: usage:\n", prog_name);
  fprintf(stderr, "%s <input.yaml>\n", prog_name);
  exit(0);
}

static bool approx_equal(sw_real_t x, sw_real_t y) {
  return (fabs(x - y) < 1e-14);
}

static void test_fixed_and_enumerated(sw_ensemble_t *ensemble) {
  assert(sw_ensemble_size(ensemble) == 11);
  sw_input_t *input;
  sw_output_t *output;
  while (sw_ensemble_next(ensemble, &input, &output)) {
    sw_input_result_t in_result;
    sw_input_array_result_t in_array_result;

    assert(sw_input_has_array(input, "p1"));
    in_array_result = sw_input_get_array(input, "p1");
    assert(in_array_result.error_code == SW_SUCCESS);
    assert(in_array_result.size == 4);
    assert(in_array_result.values[0] >= 1.0);
    assert(in_array_result.values[0] <= 11.0);
    assert(approx_equal(in_array_result.values[1], 1+in_array_result.values[0]));
    assert(approx_equal(in_array_result.values[2], 2+in_array_result.values[0]));
    assert(approx_equal(in_array_result.values[3], 3+in_array_result.values[0]));
    assert(in_array_result.error_message == NULL);

    assert(sw_input_has_array(input, "p2"));
    in_array_result = sw_input_get_array(input, "p2");
    assert(in_array_result.error_code == SW_SUCCESS);
    assert(in_array_result.size == 3);
    assert(approx_equal(in_array_result.values[0], 4.0));
    assert(approx_equal(in_array_result.values[1], 5.0));
    assert(approx_equal(in_array_result.values[2], 6.0));
    assert(in_array_result.error_message == NULL);

    assert(sw_input_has(input, "p3"));
    in_result = sw_input_get(input, "p3");
    assert(in_result.error_code == SW_SUCCESS);
    assert(approx_equal(in_result.value, 3.0));
    assert(in_result.error_message == NULL);

    // Add a "qoi" metric set to 4.
    sw_output_set(output, "qoi", 4.0);
  }
}

static void test_nonexpandable_array(sw_ensemble_t *ensemble) {
  assert(sw_ensemble_size(ensemble) == 3);
  sw_input_t *input;
  sw_output_t *output;
  int i = 0;
  while (sw_ensemble_next(ensemble, &input, &output)) {
    sw_input_result_t in_result;
    sw_input_array_result_t in_array_result;

    assert(sw_input_has_array(input, "Ns"));
    in_array_result = sw_input_get_array(input, "Ns");
    assert(in_array_result.error_code == SW_SUCCESS);
    assert(in_array_result.size == 1);
    static sw_real_t Ns[3] = {0.0009478315467, 0.0008633937165, 0.01542388755};
    assert(approx_equal(in_array_result.values[0], Ns[i]));
    assert(in_array_result.error_message == NULL);

    assert(sw_input_has_array(input, "Temperature"));
    in_array_result = sw_input_get_array(input, "Temperature");
    assert(in_array_result.error_code == SW_SUCCESS);
    assert(in_array_result.size == 1);
    static sw_real_t Temperatures[3] = {-32.69480152, -31.94781043, -35.75495987};
    assert(approx_equal(in_array_result.values[0], Temperatures[i]));
    assert(in_array_result.error_message == NULL);

    static sw_real_t dts[3] = {0.0, 0.0, 0.0};
    assert(sw_input_has(input, "dt"));
    in_result = sw_input_get(input, "dt");
    assert(in_result.error_code == SW_SUCCESS);
    assert(approx_equal(in_result.value, dts[i]));
    assert(in_result.error_message == NULL);

    assert(sw_input_has_array(input, "w_vlc"));
    in_array_result = sw_input_get_array(input, "w_vlc");
    assert(in_array_result.error_code == SW_SUCCESS);
    assert(in_array_result.size == 1);
    static sw_real_t w_vlcs[3] = {0.2, 0.2, 0.2};
    assert(approx_equal(in_array_result.values[0], w_vlcs[i]));
    assert(in_array_result.error_message == NULL);

    // Add a "qoi" metric set to 4.
    sw_output_set(output, "qoi", 4.0);

    ++i;
  }
}

int main(int argc, char **argv) {

  if (argc == 1) {
    usage((const char*)argv[0]);
  }
  const char* input_file = argv[1];

  // Print a banner with Skywalker's version info.
  sw_print_banner();

  // Load the ensemble. Any error encountered is fatal.
  fprintf(stderr, "array_param_test: Loading ensemble from %s\n", input_file);
  sw_ensemble_result_t load_result = sw_load_ensemble(input_file, "settings");
  if (load_result.error_code != SW_SUCCESS) {
    fprintf(stderr, "%s\n", load_result.error_message);
    exit(-1);
  }
  sw_ensemble_t *ensemble = load_result.ensemble;

  // Which tests are we supposed to run?
  sw_settings_t *settings = load_result.settings;
  assert(sw_settings_has(settings, "which"));
  sw_settings_result_t settings_result = sw_settings_get(settings, "which");
  if (!strcmp(settings_result.value, "fixed_and_enumerated")) {
    test_fixed_and_enumerated(ensemble);
  } else if (!strcmp(settings_result.value, "nonexpandable_array")) {
    test_nonexpandable_array(ensemble);
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
