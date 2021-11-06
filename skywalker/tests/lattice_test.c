// This program tests Skywalker's C interface with a lattice ensemble.

#include <skywalker.h>

#include <assert.h>
#include <stdio.h>
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

  // Load the ensemble. Any error encountered is fatal.
  fprintf(stderr, "lattice_test: Loading ensemble from %s\n", input_file);
  sw_ensemble_result_t load_result = sw_load_ensemble(input_file, "settings");
  assert(load_result.settings != NULL);
  assert(load_result.ensemble != NULL);
  assert(load_result.error_code == SW_SUCCESS);
  assert(load_result.error_message == NULL);

  // Make sure everything is as it should be.

  // Ensemble type
  assert(load_result.type == SW_LATTICE);

  // Settings
  sw_settings_t *settings = load_result.settings;
  sw_settings_result_t settings_result;
  settings_result = sw_settings_get(settings, "param1");
  assert(settings_result.error_code == 0);
  assert(strcmp(settings_result.value, "hello") == 0);
  assert(settings_result.error_message == NULL);

  settings_result = sw_settings_get(settings, "param2");
  assert(settings_result.error_code == 0);
  assert(strcmp(settings_result.value, "81") == 0);
  assert(settings_result.error_message == NULL);

  settings_result = sw_settings_get(settings, "param3");
  assert(settings_result.error_code == 0);
  assert(strcmp(settings_result.value, "3.14159265357") == 0);
  assert(settings_result.error_message == NULL);

  // Ensemble data
  sw_ensemble_t *ensemble = load_result.ensemble;
  assert(sw_ensemble_size(ensemble) == 245520);
  sw_input_t *input;
  sw_output_t *output;
  while (sw_ensemble_next(ensemble, &input, &output)) {
    sw_input_result_t in_result;

    // Fixed parameters
    in_result = sw_input_get(input, "p1");
    assert(in_result.error_code == SW_SUCCESS);
    assert(approx_equal(in_result.value, 1.0));
    assert(in_result.error_message == NULL);

    in_result = sw_input_get(input, "p2");
    assert(in_result.error_code == SW_SUCCESS);
    assert(approx_equal(in_result.value, 2.0));
    assert(in_result.error_message == NULL);

    in_result = sw_input_get(input, "p3");
    assert(in_result.error_code == SW_SUCCESS);
    assert(approx_equal(in_result.value, 3.0));
    assert(in_result.error_message == NULL);

    // Ensemble parameters
    in_result = sw_input_get(input, "tick");
    assert(in_result.error_code == SW_SUCCESS);
    assert(in_result.value >= 0.0);
    assert(in_result.value <= 10.0);
    assert(in_result.error_message == NULL);

    in_result = sw_input_get(input, "tock");
    assert(in_result.error_code == SW_SUCCESS);
    assert(in_result.value >= 1e1);
    assert(in_result.value <= 1e4);
    assert(in_result.error_message == NULL);

    in_result = sw_input_get(input, "pair");
    assert(in_result.error_code == SW_SUCCESS);
    assert(in_result.value >= 1.0);
    assert(in_result.value <= 2.0);
    assert(in_result.error_message == NULL);

    in_result = sw_input_get(input, "triple");
    assert(in_result.error_code == SW_SUCCESS);
    assert(in_result.value >= 1.0);
    assert(in_result.value <= 3.0);
    assert(in_result.error_message == NULL);

    in_result = sw_input_get(input, "quartet");
    assert(in_result.error_code == SW_SUCCESS);
    assert(in_result.value >= 1.0);
    assert(in_result.value <= 4.0);
    assert(in_result.error_message == NULL);

    in_result = sw_input_get(input, "quintet");
    assert(in_result.error_code == SW_SUCCESS);
    assert(in_result.value >= 1.0);
    assert(in_result.value <= 5.0);
    assert(in_result.error_message == NULL);

    in_result = sw_input_get(input, "sextet");
    assert(in_result.error_code == SW_SUCCESS);
    assert(in_result.value >= 1.0);
    assert(in_result.value <= 6.0);
    assert(in_result.error_message == NULL);

    // Look for a parameter that doesn't exist.
    in_result = sw_input_get(input, "invalid_param");
    assert(in_result.error_code == SW_PARAM_NOT_FOUND);
    assert(in_result.error_message != NULL);

    // Add a "qoi" metric set to 4.
    sw_output_result_t out_result = sw_output_set(output, "qoi", 4.0);
    assert(out_result.error_code == SW_SUCCESS);
    assert(out_result.error_message == NULL);
  }

  // Write out a Python module.
  sw_ensemble_write(ensemble, "lattice_test.py");
}
