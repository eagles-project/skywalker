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

// This program tests Skywalker's ability to validate input.

#include <skywalker.h>

#include <assert.h>
#include <string.h>

static void test_nonexistent_file() {
  sw_ensemble_result_t load_result = sw_load_ensemble("/nope", "settings");
  assert(load_result.error_code == SW_YAML_FILE_NOT_FOUND);
  assert(load_result.error_message != NULL);
}

static void write_test_input(const char* yaml_text, const char* filename) {
  FILE *f = fopen(filename, "w");
  fprintf(f, "%s", yaml_text);
  fclose(f);
}

static void test_invalid_settings_block() {
  const char* bad_yaml =
    "settings:\n  a: 1\n\n"
    "input:\n  fixed:\n    x: 1\n    y: 2\n    z: 3\n";
  write_test_input(bad_yaml, "invalid_settings.yaml");
  sw_ensemble_result_t load_result =
    sw_load_ensemble("invalid_settings.yaml", "input");
  assert(load_result.error_code == SW_INVALID_SETTINGS_BLOCK);
  assert(load_result.error_message != NULL);
}

static void test_duplicate_setting() {
  const char* bad_yaml =
    "settings:\n  a: 1\n  a: 2\n"
    "input:\n  fixed:\n    x: 1\n    y: 2\n    z: 3\n";
  write_test_input(bad_yaml, "duplicate_setting.yaml");
  sw_ensemble_result_t load_result =
    sw_load_ensemble("duplicate_setting.yaml", "settings");
  assert(load_result.error_code == SW_INVALID_SETTINGS_BLOCK);
  assert(load_result.error_message != NULL);
}

static void test_missing_settings_block() {
  const char* bad_yaml =
    "no_settings:\n  a: 1\n\n"
    "input:\n  fixed:\n    x: 1\n    y: 2\n    z: 3\n";
  write_test_input(bad_yaml, "missing_settings.yaml");
  sw_ensemble_result_t load_result =
    sw_load_ensemble("missing_settings.yaml", "settings");
  assert(load_result.error_code == SW_SETTINGS_NOT_FOUND);
  assert(load_result.error_message != NULL);
}

static void test_invalid_param_name() {
  sw_ensemble_result_t load_result;

  // No names with dots.
  const char* name_with_dot_yaml =
    "settings:\n  a: 1\n\n"
    "input:\n  fixed:\n    x.y: 1\n    y: 2\n    z: 3\n";
  write_test_input(name_with_dot_yaml, "name_with_dot.yaml");
  load_result = sw_load_ensemble("name_with_dot.yaml", "settings");
  assert(load_result.error_code == SW_INVALID_PARAM_NAME);
  assert(load_result.error_message != NULL);

  // No names starting with numbers.
  const char* leading_number_name_yaml =
    "settings:\n  a: 1\n\n"
    "input:\n  fixed:\n    2x: 1\n    y: 2\n    z: 3\n";
  write_test_input(leading_number_name_yaml, "leading_number_name.yaml");
  load_result = sw_load_ensemble("leading_number_name.yaml", "settings");
  assert(load_result.error_code == SW_INVALID_PARAM_NAME);
  assert(load_result.error_message != NULL);

  // Names containing underscores are cool, though.
  const char* underscored_names_yaml =
    "settings:\n  a: 1\n\n"
    "input:\n  fixed:\n    _x: 1\n    y_0: 2\n    _z_: 3\n";
  write_test_input(underscored_names_yaml, "underscored_names.yaml");
  load_result = sw_load_ensemble("underscored_names.yaml", "settings");
  assert(load_result.error_code == SW_SUCCESS);
  assert(load_result.error_message == NULL);
  sw_ensemble_free(load_result.ensemble);
}

static void test_duplicate_param() {
  sw_ensemble_result_t load_result;

  // No names with dots.
  const char* name_with_dot_yaml =
    "settings:\n  a: 1\n\n"
    "input:\n  fixed:\n    x: 1\n    x: 2\n    z: 3\n";
  write_test_input(name_with_dot_yaml, "duplicate_param.yaml");
  load_result = sw_load_ensemble("duplicate_param.yaml", "settings");
  assert(load_result.error_code == SW_INVALID_PARAM_NAME);
  assert(load_result.error_message != NULL);
}

static void test_too_many_lattice_params() {
  const char* bad_yaml =
    "settings:\n  a: 1\n\n"
    "input:\n  lattice:\n"
    "    x1: [1, 2]\n    x2: [2, 3]\n    x3: [3,4]\n"
    "    x4: [4, 5]\n    x5: [5, 6]\n    x6: [6,7]\n"
    "    x7: [7, 8]\n    x8: [8, 9]\n";
  write_test_input(bad_yaml, "too_many_lattice_params.yaml");
  sw_ensemble_result_t load_result =
    sw_load_ensemble("too_many_lattice_params.yaml", "settings");
  assert(load_result.error_code == SW_TOO_MANY_LATTICE_PARAMS);
  assert(load_result.error_message != NULL);
}

static void test_invalid_enumeration() {
  const char* bad_yaml =
    "settings:\n  a: 1\n\n"
    "input:\n  enumerated:\n"
    "    x1: [1, 2, 3]\n    x2: [2, 3]\n    x3: [3,4]\n";
  write_test_input(bad_yaml, "invalid_enumeration.yaml");
  sw_ensemble_result_t load_result =
    sw_load_ensemble("invalid_enumeration.yaml", "settings");
  assert(load_result.error_code == SW_INVALID_ENUMERATION);
  assert(load_result.error_message != NULL);
}

static void test_empty_ensemble() {
  const char* bad_yaml =
    "settings:\n  a: 1\n\n"
    "input:\n  enumerated:\n"
    "    x1: []\n    x2: []\n    x3: []\n";
  write_test_input(bad_yaml, "empty_ensemble.yaml");
  sw_ensemble_result_t load_result =
    sw_load_ensemble("empty_ensemble.yaml", "settings");
  assert(load_result.error_code == SW_EMPTY_ENSEMBLE);
  assert(load_result.error_message != NULL);
}

static void test_negative_values_issue_33() {
  const char* bad_yaml = "\
settings:  \n\
  a: 1  \n\
input:  \n\
  enumerated:  \n\
    Ns: [[0.0009478315467], [0.0008633937165], [0.01542388755]]  \n\
    Temperature: [[-32.69480152], [-31.94781043], [-35.75495987]]  \n\
    dt: [0.0, 0.0, 0.0]  \n\
    w_vlc: [[0.2], [0.2], [0.2]]  \n\
";
  write_test_input(bad_yaml, "negative_values.yaml");
  sw_ensemble_result_t load_result =
    sw_load_ensemble("negative_values.yaml", "settings");
  if (load_result.error_message)
    printf("%s\n",load_result.error_message);
  assert(load_result.error_code == SW_SUCCESS);
  sw_ensemble_t *ensemble = load_result.ensemble;
  assert(sw_ensemble_size(ensemble) == 3);
}

int main(int argc, char **argv) {

  // We ignore command line arguments in favor of programmatically generating
  // bad inputs.

  // Print a banner with Skywalker's version info.
  sw_print_banner();

  // Now validate!
  test_nonexistent_file();
  test_invalid_settings_block();
  test_duplicate_setting();
  test_missing_settings_block();
  test_invalid_param_name();
  test_duplicate_param();
  test_too_many_lattice_params();
  test_invalid_enumeration();
  test_empty_ensemble();
  test_negative_values_issue_33();
}
