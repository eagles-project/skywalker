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

#include <skywalker.h>

#include <string.h>
#include <tgmath.h>

void usage() {
  fprintf(stderr, "isotherms_c: calculates the pressure of a Van der Waals gas "
                  "given its volume and temperature.\n");
  fprintf(stderr, "isotherms_c: usage:\n");
  fprintf(stderr, "isotherms_c <input.yaml>\n");
  exit(-1);
}

// Retrieves the value with the given name from the given input, exiting
// on failure.
sw_real_t get_value(sw_input_t *input, const char *name) {
  sw_input_result_t in_result = sw_input_get(input, name);
  if (in_result.error_code != SW_SUCCESS) {
    fprintf(stderr, "isotherms_c: %s", in_result.error_message);
    exit(-1);
  }
  return in_result.value;
}

// Determines the output_file name corresponding to the given name of the
// input file.
const char* output_file_name(const char *input_file) {
  static char output_file_[FILENAME_MAX];
  size_t dot_index = strlen(input_file);
  char *dot = strstr(input_file, ".");
  if (dot) {
    dot_index = (const char*)dot - input_file;
  }
  memcpy(output_file_, input_file, sizeof(char) * dot_index);
  memcpy(&output_file_[dot_index], "_c.py\0", sizeof(char) * 6);
  return (const char*)output_file_;
}

int main(int argc, char **argv) {

  if (argc == 1) {
    usage();
  }
  const char *input_file = argv[1];

  // Load the ensemble. Any error encountered is fatal.
  printf("isotherms_c: Loading ensemble from %s...\n", input_file);
  sw_ensemble_result_t load_result = sw_load_ensemble(input_file, NULL);
  if (load_result.error_code != SW_SUCCESS) {
    fprintf(stderr, "isotherms_c: %s", load_result.error_message);
    exit(-1);
  }

  sw_ensemble_t *ensemble = load_result.ensemble;
  printf("isotherms_c: found %zd ensemble members.\n", sw_ensemble_size(ensemble));

  // Iterate over all members of the ensemble.
  sw_input_t *input;
  sw_output_t *output;
  while (sw_ensemble_next(ensemble, &input, &output)) {
    // Fetch inputs.
    sw_real_t V = get_value(input, "V"); // gas (molar) volume [m3]
    sw_real_t T = get_value(input, "T"); // gas temperature [K]

    // Fetch Van der Waals parameters if they're present.
    sw_real_t a = 0.0, b = 0.0;
    if (sw_input_has(input, "a")) {
      a = get_value(input, "a");
    }
    if (sw_input_has(input, "b")) {
      b = get_value(input, "b");
    }

    // Compute p(V, T).
    static const sw_real_t R = 8.31446261815324;
    sw_real_t p = R * T / (V - b) - a/(V*V);

    // Stash the computed pressure in the member's output.
    sw_output_set(output, "p", p);
  }

  // Write out a Python module.
  const char *output_file = output_file_name(input_file);
  printf("isotherms_c: Writing data to %s...\n", output_file);
  sw_write_result_t w_result = sw_ensemble_write(ensemble, output_file);
  if (w_result.error_code != SW_SUCCESS) {
    fprintf(stderr, "isotherms_c: %s\n", w_result.error_message);
    exit(-1);
  }

  // Clean up.
  sw_ensemble_free(ensemble);
}
