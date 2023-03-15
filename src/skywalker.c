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

#include <khash.h>
#include <klist.h>
#include <kvec.h>
#include <yaml.h>

#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

void sw_print_banner() {
  fprintf(stderr, "Skywalker v%d.%d.%d\n", SKYWALKER_MAJOR_VERSION,
          SKYWALKER_MINOR_VERSION, SKYWALKER_PATCH_VERSION);
}

// Some basic data structures.

// A list of C strings.
#define free_string(x) free((char*)x->data)
KLIST_INIT(string_list, const char*, free_string)

// A hash table whose keys are C strings and whose values are also C strings.
KHASH_MAP_INIT_STR(string_map, const char*)

// A hash table whose keys are C strings and whose values are real numbers.
KHASH_MAP_INIT_STR(param_map, sw_real_t)

// A hash table whose keys are C strings and whose values are real number arrays.
typedef kvec_t(sw_real_t) real_vec_t;
KHASH_MAP_INIT_STR(array_param_map, real_vec_t)

// A list of strings to be deallocated by skywalker.
static klist_t(string_list)* sw_strings_ = NULL;

// This function cleans up all maintained strings at the end of a process.
static void free_strings() {
  kl_destroy(string_list, sw_strings_);
}

// This function appends a string to the list of maintained strings, setting
// things up when it's called for the first time.
static void append_string(const char *s) {
  if (!sw_strings_) {
    sw_strings_ = kl_init(string_list);
    atexit(free_strings);
  }
  const char ** s_p = kl_pushp(string_list, sw_strings_);
  *s_p = s;
}

// Here we implement a portable version of the non-standard vasprintf
// function (see https://stackoverflow.com/questions/40159892/using-asprintf-on-windows).
static int sw_vscprintf(const char *format, va_list ap) {
  va_list ap_copy;
  va_copy(ap_copy, ap);
  int retval = vsnprintf(NULL, 0, format, ap_copy);
  va_end(ap_copy);
  return retval;
}

static int sw_vasprintf(char **strp, const char *format, va_list ap) {
  int len = sw_vscprintf(format, ap);
  if (len == -1)
    return -1;
  char *str = (char*)malloc((size_t) len + 1);
  if (!str)
    return -1;
  int retval = vsnprintf(str, len + 1, format, ap);
  if (retval == -1) {
    free(str);
    return -1;
  }
  *strp = str;
  return retval;
}

// This function constructs a string in the manner of sprintf, but allocates
// and returns a string of sufficient size. Strings created this way are freed
// when the program exits,
static const char* new_string(const char *fmt, ...) {
  char *s;
  va_list ap;
  va_start(ap, fmt);
  sw_vasprintf(&s, fmt, ap);
  va_end(ap);
  append_string(s);
  return s;
}

// This function duplicates the given string and appends it to the list of
// strings to be freed when the program exits.
static const char* dup_string(const char *s) {
  size_t len = strlen(s);
  char *dup = malloc(sizeof(char)*(len+1));
  strcpy(dup, s);
  append_string(dup);
  return (const char*)dup;
}

struct sw_settings_t {
  khash_t(string_map) *params;
};

// Creates a settings instance.
static sw_settings_t *sw_settings_new() {
  sw_settings_t *settings = malloc(sizeof(sw_settings_t));
  settings->params = kh_init(string_map);
  return settings;
}

// Destroys a settings instance, freeing all allocated resources.
static void sw_settings_free(sw_settings_t *settings) {
  kh_destroy(string_map, settings->params);
  free(settings);
}

static void sw_settings_set(sw_settings_t *settings, const char *name,
                            const char *value) {
  const char* n = dup_string(name);
  const char* v = dup_string(value);
  int ret;
  khiter_t iter = kh_put(string_map, settings->params, n, &ret);
  assert(ret == 1);
  kh_value(settings->params, iter) = v;
}

bool sw_settings_has(sw_settings_t *settings, const char* name) {
  khiter_t iter = kh_get(string_map, settings->params, name);
  return (iter != kh_end(settings->params));
}

sw_settings_result_t sw_settings_get(sw_settings_t *settings,
                                    const char* name) {
  khiter_t iter = kh_get(string_map, settings->params, name);
  sw_settings_result_t result = {.error_code = SW_SUCCESS};
  if (iter != kh_end(settings->params)) {
    result.value = kh_val(settings->params, iter);
  } else {
    result.error_code = SW_PARAM_NOT_FOUND;
    const char *s = new_string("The setting '%s' was not found.", name);
    result.error_message = s;
  }
  return result;
}

struct sw_input_t {
  khash_t(param_map) *params;
  khash_t(array_param_map) *array_params;
};

static void sw_input_set(sw_input_t *input, const char *name, sw_real_t value) {
  const char* n = dup_string(name);
  int ret;
  khiter_t iter = kh_put(param_map, input->params, n, &ret);
  assert(ret == 1);
  kh_value(input->params, iter) = value;
}

static void sw_input_set_array(sw_input_t *input, const char *name,
                               real_vec_t values) {
  const char* n = dup_string(name);
  int ret;
  // Make a copy of the array values.
  real_vec_t my_values;
  kv_init(my_values);
  for (size_t i = 0; i < kv_size(values); ++i) {
    kv_push(sw_real_t, my_values, kv_A(values, i));
  }

  // Insert the copy.
  khiter_t iter = kh_put(array_param_map, input->array_params, n, &ret);
  assert(ret == 1);
  kh_value(input->array_params, iter) = my_values;
}

bool sw_input_has(sw_input_t *input, const char *name) {
  khiter_t iter = kh_get(param_map, input->params, name);
  return (iter != kh_end(input->params));
}

sw_input_result_t sw_input_get(sw_input_t *input, const char *name) {
  khiter_t iter = kh_get(param_map, input->params, name);
  sw_input_result_t result = {.error_code = SW_SUCCESS};
  if (iter != kh_end(input->params)) {
    result.value = kh_val(input->params, iter);
  } else {
    result.error_code = SW_PARAM_NOT_FOUND;
    const char *s = new_string("The input parameter '%s' was not found.", name);
    result.error_message = s;
  }
  return result;
}

bool sw_input_has_array(sw_input_t *input, const char *name) {
  khiter_t iter = kh_get(array_param_map, input->array_params, name);
  return (iter != kh_end(input->array_params));
}

sw_input_array_result_t sw_input_get_array(sw_input_t *input, const char *name) {
  khiter_t iter = kh_get(array_param_map, input->array_params, name);
  sw_input_array_result_t result = {.error_code = SW_SUCCESS};
  if (iter != kh_end(input->array_params)) {
    real_vec_t values = kh_val(input->array_params, iter);
    result.size = kv_size(values);
    result.values = values.a;
  } else {
    result.error_code = SW_PARAM_NOT_FOUND;
    const char *s = new_string("The input array parameter '%s' was not found.",
                               name);
    result.error_message = s;
  }
  return result;
}

struct sw_output_t {
  khash_t(param_map) *metrics;
  khash_t(array_param_map) *array_metrics;
};

void sw_output_set(sw_output_t *output, const char *name, sw_real_t value) {
  const char* n = dup_string(name);
  int ret;
  khiter_t iter = kh_put(param_map, output->metrics, n, &ret);
  assert(ret == 1);
  kh_value(output->metrics, iter) = value;
}

void sw_output_set_array(sw_output_t *output, const char *name,
                         const sw_real_t *values, size_t size) {
  const char* n = dup_string(name);
  int ret;
  khiter_t iter = kh_put(array_param_map, output->array_metrics, n, &ret);
  assert(ret == 1);
  real_vec_t array;
  kv_init(array);
  for (size_t i=0; i<size; ++i)
    kv_push(sw_real_t, array, values[i]); // append
  kh_value(output->array_metrics, iter) = array;
}

// ensemble type
struct sw_ensemble_t {
  size_t size, position;
  sw_input_t *inputs;
  sw_output_t *outputs;
  sw_settings_t *settings; // for writing and freeing
};

//------------------------------------------------------------------------
//                              YAML parsing
//------------------------------------------------------------------------

// A YAML-specific string pool.
static klist_t(string_list)* yaml_strings_ = NULL;

// This function cleans up all maintained strings at the end of a process.
static void free_yaml_strings() {
  if (yaml_strings_) {
    kl_destroy(string_list, yaml_strings_);
    yaml_strings_ = NULL;
  }
}

// This function creates a copy of a string encountered in the YAML parser,
// placing it in the YAML string pool.
static const char* dup_yaml_string(const char *s) {
  // Copy the string.
  size_t len = strlen(s);
  char *dup = malloc(sizeof(char)*(len+1));
  strcpy(dup, s);

  // Stick it in the YAML string pool.
  if (!yaml_strings_) {
    yaml_strings_ = kl_init(string_list);
  }
  const char ** s_p = kl_pushp(string_list, yaml_strings_);
  *s_p = dup;

  return (const char*)dup;
}

// A hash table whose keys are C strings and whose values are real numbers.
KHASH_MAP_INIT_STR(yaml_param_map, real_vec_t)

// A vector of vectors containing real numbers.
typedef kvec_t(real_vec_t) real_vec_vec_t;

// A hash table whose keys are C strings and whose values are arrays of real
// numbers.
KHASH_MAP_INIT_STR(yaml_array_param_map, real_vec_vec_t)

// A hash table representing a set of C strings. Used to guarantee that input
// parameter/setting names are unique.
KHASH_SET_INIT_STR(yaml_name_set);

// This type stores data parsed from YAML.
typedef struct yaml_data_t {
  sw_settings_t *settings;
  khash_t(yaml_param_map) *fixed_input, *lattice_input, *enumerated_input;
  khash_t(yaml_array_param_map) *fixed_array_input, *lattice_array_input,
                                *enumerated_array_input;
  khash_t(yaml_name_set) *setting_names;
  khash_t(yaml_name_set) *param_names;
  size_t num_enumerated_inputs;
  int error_code;
  const char *error_message;
} yaml_data_t;

// This frees any resources allocated for the yaml data struct.
static void free_yaml_data(yaml_data_t data) {
  // Destroy parsed scalars.
  {
    real_vec_t values;
    kh_foreach_value(data.fixed_input, values,
      kv_destroy(values);
    );
    kh_foreach_value(data.lattice_input, values,
      kv_destroy(values);
    );
    kh_foreach_value(data.enumerated_input, values,
      kv_destroy(values);
    );
  }
  kh_destroy(yaml_param_map, data.fixed_input);
  kh_destroy(yaml_param_map, data.lattice_input);
  kh_destroy(yaml_param_map, data.enumerated_input);

  // Destroy parsed arrays.
  {
    real_vec_vec_t values;
    kh_foreach_value(data.fixed_array_input, values,
      for (size_t i = 0; i < kv_size(values); ++i)
        kv_destroy(kv_A(values, i));
      kv_destroy(values);
    );
    kh_foreach_value(data.lattice_array_input, values,
      for (size_t i = 0; i < kv_size(values); ++i)
        kv_destroy(kv_A(values, i));
      kv_destroy(values);
    );
    kh_foreach_value(data.enumerated_array_input, values,
      for (size_t i = 0; i < kv_size(values); ++i)
        kv_destroy(kv_A(values, i));
      kv_destroy(values);
    );
  }
  kh_destroy(yaml_array_param_map, data.fixed_array_input);
  kh_destroy(yaml_array_param_map, data.lattice_array_input);
  kh_destroy(yaml_array_param_map, data.enumerated_array_input);

  kh_destroy(yaml_name_set, data.setting_names);
  kh_destroy(yaml_name_set, data.param_names);

  if (data.settings) sw_settings_free(data.settings);
}

// This type keeps track of the state of the YAML parser.
typedef struct parser_state_t {
  const char *settings_block;
  bool parsing_settings;
  const char *current_setting;
  bool parsing_input;
  bool parsing_fixed_params;
  bool parsing_lattice_params;
  bool parsing_enumerated_params;
  bool parsing_input_sequence;
  bool parsing_input_array_sequence;
  const char *current_param;
} parser_state_t;

// Returns true if the given input parameter name is valid, false otherwise,
// based on whether the parameter is an array or a scalar.
static bool is_valid_input_name(const char *name, bool is_array_value) {
  assert(name);

  // A name must begin with an alphabetical character or an underscore.
  if (!isalpha(name[0]) && (name[0] != '_'))
    return false;

  // All other characters must be alphanumeric (or underscores) OR must be one
  // of the allowed substrings.
  size_t len = strlen(name);
  bool log10_opened = false;
  for (size_t i = 1; i < len; ++i) {
    if (!isalnum(name[i]) && (name[i] != '_')) {
      if (is_array_value) { // array values can't have non-alphanumerics
        return false;
      } else {
        // Is this from a log10(x) expression?
        if (!log10_opened && (name[i] == '(')) {
          const char* log10 = strstr(name, "log10(");
          if (log10 && ((size_t)(log10 - name) < i)) // yes, it's a log10 expression
            log10_opened = true;
          else // nope
            return false;
        } else if (!(log10_opened && name[i] == ')')) {
          return false;
        }
      }
    }
  }

  return true;
}

// Handles a YAML event, populating our data instance.
static void handle_yaml_event(yaml_event_t *event,
                              parser_state_t* state,
                              yaml_data_t *data)
{
  if (event->type == YAML_SCALAR_EVENT) {
    const char *value = (const char*)(event->data.scalar.value);
    // settings block
    if (!state->parsing_settings && state->settings_block &&
        (state->settings_block[0] != '\0') && // exclude blank strings
        !strcmp(value, state->settings_block)) {
      assert(!state->current_setting);
      data->settings = sw_settings_new();
      state->parsing_settings = true;
    } else if (state->parsing_settings) {
      if (!state->current_setting) {
        // Have we seen this setting name before?
        khiter_t iter = kh_get(yaml_name_set, data->setting_names, value);
        if (iter != kh_end(data->setting_names)) { // yes
          data->error_code = SW_INVALID_SETTINGS_BLOCK;
          data->error_message = new_string(
            "Setting %s appears more than once!", value);
          return;
        }

        // Set the current setting name and add it to our set of tracked
        // names.
        state->current_setting = dup_yaml_string(value);
        int ret;
        iter = kh_put(yaml_name_set, data->setting_names,
                      state->current_setting, &ret);
        assert(ret == 1);
      } else {
        sw_settings_set(data->settings, state->current_setting, value);
        state->current_setting = NULL;
      }

    // input block
    } else if (!state->parsing_input && !strcmp(value, "input")) {
      state->parsing_input = true;
    } else if (state->parsing_input) {
      if (!state->parsing_fixed_params &&
          !state->parsing_lattice_params &&
          !state->parsing_enumerated_params) {
        if (!strcmp(value, "fixed")) {
          state->parsing_fixed_params = true;
        } else if (!strcmp(value, "lattice")) {
          state->parsing_lattice_params = true;
        } else if (!strcmp(value, "enumerated")) {
          state->parsing_enumerated_params = true;
        } else {
          data->error_code = SW_INVALID_PARAM_TYPE;
          data->error_message = new_string("Invalid parameter type: %s", value);
        }
      } else { // handle input name/value
        if (!state->current_param) { // parse the input parameter name
          // Have we seen this parameter name before?
          khiter_t iter = kh_get(yaml_name_set, data->param_names, value);
          if (iter != kh_end(data->param_names)) { // yes
            data->error_code = SW_INVALID_PARAM_NAME;
            data->error_message = new_string(
                "Input parameter %s appears more than once!", value);
            return;
          }

          // Is the name valid?
          if (!is_valid_input_name(value, state->parsing_input_array_sequence)) {
            data->error_code = SW_INVALID_PARAM_NAME;
            data->error_message = new_string(
                "Invalid input parameter name: %s", value);
            return;
          }

          // Set the current parameter name and add it to our set of tracked
          // names.
          state->current_param = dup_yaml_string(value);
          int ret;
          iter = kh_put(yaml_name_set, data->param_names,
                        state->current_param, &ret);
          assert(ret == 1);
        } else { // we have an input name; parse its value
          // Try to interpret the value as a real number.
          char *endp;
          sw_real_t real_value = strtod(value, &endp);
          if (endp == value) { // invalid value!
            data->error_code = SW_INVALID_PARAM_VALUE;
            data->error_message = new_string(
                "Invalid input value for fixed parameter %s: %s",
                state->current_param, value);
            return;
          } else { // valid real value
            // If we're parsing array input, append this value to it.
            if ((state->parsing_input_sequence &&
                 state->parsing_fixed_params) ||
                (state->parsing_input_array_sequence)) {
              khash_t(yaml_array_param_map) *array_input;
              if (state->parsing_fixed_params) {
                array_input = data->fixed_array_input;
              } else if (state->parsing_lattice_params) {
                array_input = data->lattice_array_input;
              } else {
                assert(state->parsing_enumerated_params);
                array_input = data->enumerated_array_input;
              }
              khiter_t iter = kh_get(yaml_array_param_map, array_input,
                                     state->current_param);
              if (iter == kh_end(array_input)) { // name not yet encountered
                // Create an array of arrays containing one empty array.
                real_vec_vec_t arrays;
                kv_init(arrays);
                real_vec_t array;
                kv_init(array);
                kv_push(real_vec_t, arrays, array);

                // Add it to the array parameter map.
                int ret;
                iter = kh_put(yaml_array_param_map, array_input,
                    state->current_param, &ret);
                assert(ret == 1);
                kh_value(array_input, iter) = arrays;
              }
              // Append this value to the last array in the list of arrays for
              // this input.
              real_vec_vec_t arrays = kh_value(array_input, iter);
              size_t index = kv_size(arrays)-1;
              real_vec_t last_array = kv_A(arrays, index);
              kv_push(sw_real_t, last_array, real_value);
              kv_A(arrays, index) = last_array;
              kh_value(array_input, iter) = arrays;
            } else { // not in the middle of an array sequence
              // Otherwise, append the value to the list of inputs with this name.
              khash_t(yaml_param_map) *input;
              if (state->parsing_fixed_params) {
                input = data->fixed_input;
              } else if (state->parsing_lattice_params) {
                input = data->lattice_input;
              } else {
                assert(state->parsing_enumerated_params);
                input = data->enumerated_input;
              }
              khiter_t iter = kh_get(yaml_param_map, input, state->current_param);
              if (iter == kh_end(input)) { // name not yet encountered
                int ret;
                iter = kh_put(yaml_param_map, input, state->current_param, &ret);
                assert(ret == 1);
                kv_init(kh_value(input, iter));
              }
              kv_push(sw_real_t, kh_value(input, iter), real_value);
            }
          } // valid value

          // Clear the current input if we're parsing a scalar.
          if (!state->parsing_input_sequence) {
            state->current_param = NULL;
          }
        } // handle input value
      } // handle input name/value
    } // parsing input

  // validation for mappings
  } else if (event->type == YAML_MAPPING_START_EVENT) {
    if (state->current_param) { // we're already parsing an input value
      data->error_code = SW_INVALID_PARAM_VALUE;
      data->error_message = new_string(
        "Mapping encountered in input parameter %s", state->current_param);
    }
  } else if (event->type == YAML_MAPPING_END_EVENT) {
    state->parsing_fixed_params = false;
    state->parsing_lattice_params = false;
    state->parsing_enumerated_params = false;
    state->parsing_settings = false;
  } else if (event->type == YAML_SEQUENCE_START_EVENT) {
    if (state->parsing_input_array_sequence) {
      data->error_code = SW_INVALID_PARAM_VALUE;
      data->error_message = new_string(
        "Cannot parse a sequence of array sequences for input parameter %s",
        state->current_param);
    } else if (state->parsing_input_sequence) {
      if (state->parsing_fixed_params) {
        data->error_code = SW_INVALID_PARAM_VALUE;
        data->error_message = new_string(
          "Cannot parse a sequence of arrays for fixed input parameter %s",
          state->current_param);
        return;
      }
      state->parsing_input_array_sequence = true;
      khash_t(yaml_array_param_map) *array_input;
      if (state->parsing_lattice_params) {
        array_input = data->lattice_array_input;
      } else {
        assert(state->parsing_enumerated_params);
        array_input = data->enumerated_array_input;
      }
      khiter_t iter = kh_get(yaml_array_param_map, array_input,
                             state->current_param);
      if (iter != kh_end(array_input)) { // name already encountered
        // Add new array to the array of arrays.
        real_vec_vec_t arrays = kh_value(array_input, iter);
        real_vec_t array;
        kv_init(array);
        kv_push(real_vec_t, arrays, array);
        kh_value(array_input, iter) = arrays;
      }
    } else if (state->parsing_input) {
      state->parsing_input_sequence = true;
    }
  } else if (event->type == YAML_SEQUENCE_END_EVENT) {
    if (state->parsing_input_array_sequence) {
      state->parsing_input_array_sequence = false;
    } else { // sequence of scalar or array inputs
      if (!state->parsing_fixed_params) {
        // Make sure the sequence has more than one value, whatever it is.
        khash_t(yaml_param_map) *input;
        khash_t(yaml_array_param_map) *array_input;
        size_t num_values = 0;
        if (state->parsing_lattice_params) {
          input = data->lattice_input;
          array_input = data->lattice_array_input;
        } else {
          input = data->enumerated_input;
          array_input = data->enumerated_array_input;
        }
        khiter_t iter = kh_get(yaml_param_map, input, state->current_param);
        if (iter != kh_end(input)) { // it's a scalar
          num_values = kv_size(kh_value(input, iter));
        } else { // is it an array?
          iter = kh_get(yaml_array_param_map, array_input, state->current_param);
          if (iter != kh_end(array_input)) {
            num_values = kv_size(kh_value(array_input, iter));
          } else { // the parameter is an empty sequence, maybe
            data->error_code = SW_EMPTY_ENSEMBLE;
            data->error_message = new_string(
              "Lattice or enumerated parameter %s has no values. Generated "
              "ensemble is empty!", state->current_param);
          }
        }
        if (num_values == 1) {
          data->error_code = SW_INVALID_PARAM_VALUE;
          data->error_message = new_string(
            "Lattice or enumerated parameter %s has only a single value.",
            state->current_param);
          return;
        }
      }
      state->parsing_input_sequence = false;
      state->current_param = NULL;
    }
  }
}

// Postprocess non-array input parameters.
static void postprocess_params(khash_t(yaml_param_map) **params,
                               int *error_code,
                               const char **error_message) {
  // Expand any relevant 3-parameter lists.
  for (khiter_t iter = kh_begin(*params);
      iter != kh_end(*params); ++iter) {

    if (!kh_exist(*params, iter)) continue;

    real_vec_t values = kh_value(*params, iter);

    if (kv_size(values) == 3) {
      sw_real_t v1 = kv_A(values, 0),
                v2 = kv_A(values, 1),
                v3 = kv_A(values, 2);
      if ((v1 < v2) &&
          (((0 < v3) && (v3 < v2)) ||
           ((v2 < 0) && ((0 < v3) && (v3 < (v2 - v1)/2))))) {
        real_vec_t expanded_values;
        kv_init(expanded_values);
        size_t size = (size_t)(ceil((v2 - v1) / v3) + 1);
        for (size_t i = 0; i < size; ++i) {
          kv_push(sw_real_t, expanded_values, v1 + i * v3);
        }
        kh_value(*params, iter) = expanded_values;
        kv_destroy(values);
      }
    }
  }

  // Exponentiate any log10 values, renaming "log10(x)" to "x".
  khash_t(yaml_param_map) *renamed_input = kh_init(yaml_param_map);
  for (khiter_t iter = kh_begin(*params);
      iter != kh_end(*params); ++iter) {

    if (!kh_exist(*params, iter)) continue;

    const char *param_name = kh_key(*params, iter);
    size_t pname_len = strlen(param_name);
    real_vec_t values = kh_value(*params, iter);

#ifdef __STDC_NO_VLA__
    char *new_param_name = malloc(sizeof(char)*(pname_len+1));
#else
    char new_param_name[pname_len+1];
#endif
    if (strstr(param_name, "log10(") == param_name) {
      if (param_name[pname_len-1] != ')') { // Did we close our parens?
        *error_code = SW_INVALID_PARAM_NAME;
        *error_message = new_string("Unclosed parens in parameter %s.",
          param_name);
        return;
      }
      if (!renamed_input)
        renamed_input = kh_init(yaml_param_map);

      memcpy(new_param_name, &param_name[6], pname_len-7);
      new_param_name[pname_len-7] = '\0';

      for (size_t i = 0; i < kv_size(values); ++i) {
        kv_A(values, i) = pow(10.0, kv_A(values, i));
      }
    } else {
      strcpy(new_param_name, param_name);
    }

    int ret;
    khiter_t r_iter = kh_put(yaml_param_map, renamed_input,
                             dup_yaml_string(new_param_name), &ret);
    assert(ret == 1);
    kh_value(renamed_input, r_iter) = values;

#ifdef __STDC_NO_VLA__
    free(new_param_name);
#endif
  }

  kh_destroy(yaml_param_map, *params);
  *params = renamed_input;
}

// Postprocess array input parameters.
static void postprocess_array_params(khash_t(yaml_array_param_map) *params) {
  // Expand any relevant 3-parameter lists.
  for (khiter_t iter = kh_begin(params); iter != kh_end(params); ++iter) {

    if (!kh_exist(params, iter)) continue;

    real_vec_vec_t array_values = kh_value(params, iter);

    if (kv_size(array_values) == 3) {
      real_vec_t array_val0 = kv_A(array_values, 0),
                 array_val1 = kv_A(array_values, 1),
                 array_val2 = kv_A(array_values, 2);
      size_t len = kv_size(array_val0);
      if (len != kv_size(array_val1)) len = 0;
      if (len != kv_size(array_val2)) len = 0;
      size_t size = INT_MAX;
      for (size_t l = 0; l < len; ++l) {
        // find minimum distance from low to high.
        sw_real_t val0 = kv_A(array_val0, l),
                  val1 = kv_A(array_val1, l),
                  val2 = kv_A(array_val2, l);
        if (val2 > 0) {
          if ((val0 < val1) && (val2 < val1)) {
            size_t s = (size_t)(ceil((val1 - val0) / val2) + 1);
            if (s < size || INT_MAX == size) size = s;
          } else {
            size = 0;
          }
        }
      }
      if (size > 0 && size != INT_MAX) {
        real_vec_vec_t expanded_array_values;
        kv_init(expanded_array_values);
        for (size_t i = 0; i < size; ++i) {
          real_vec_t expanded_values;
          kv_init(expanded_values);
          for (size_t l = 0; l < len; ++l) {
            sw_real_t val0 = kv_A(array_val0, l),
                      val2 = kv_A(array_val2, l);
            kv_push(sw_real_t, expanded_values, val0 + i * val2);
          }
          kv_push(real_vec_t, expanded_array_values, expanded_values);
        }
        kh_value(params, iter) = expanded_array_values;

        // Destroy the unprocessed array.
        for (size_t i = 0; i < kv_size(array_values); ++i) {
          kv_destroy(kv_A(array_values, i));
        }
        kv_destroy(array_values);
      }
    }
  }
}

static void validate_enumerated_params(khash_t(yaml_param_map) *params,
                                       khash_t(yaml_array_param_map) *array_params,
                                       size_t *num_inputs,
                                       int *error_code,
                                       const char **error_message) {
  *num_inputs = 0;
  const char *first_name = NULL;

  // Scalar-valued enumerated parameters
  for (khiter_t iter = kh_begin(params); iter != kh_end(params); ++iter) {

    if (!kh_exist(params, iter)) continue;

    const char *name = kh_key(params, iter);
    real_vec_t values = kh_value(params, iter);

    if (*num_inputs == 0) {
      *num_inputs = kv_size(values);
      first_name = name;
    } else if (*num_inputs != kv_size(values)) {
      *error_code = SW_INVALID_ENUMERATION;
      *error_message = new_string(
        "Invalid enumeration: Parameter %s has a different number of values (%ld)"
        " than %s (%ld)", name, kv_size(values), first_name, *num_inputs);
    }
  }
  // Array-valued enumerated parameters
  for (khiter_t iter = kh_begin(array_params);
       iter != kh_end(array_params); ++iter) {

    if (!kh_exist(array_params, iter)) continue;

    const char *name = kh_key(array_params, iter);
    real_vec_vec_t values = kh_value(array_params, iter);

    if (*num_inputs == 0) {
      *num_inputs = kv_size(values);
      first_name = name;
    } else if (*num_inputs != kv_size(values)) {
      *error_code = SW_INVALID_ENUMERATION;
      *error_message = new_string(
        "Invalid enumeration: Parameter %s has a different number of values (%ld)"
        " than %s (%ld)", name, kv_size(values), first_name, *num_inputs);
    }
  }
}

// Parses a YAML file, returning the results.
static yaml_data_t parse_yaml(FILE* file, const char* settings_block) {
  yaml_data_t data = {.error_code = 0};

  data.fixed_input = kh_init(yaml_param_map);
  data.lattice_input = kh_init(yaml_param_map);
  data.enumerated_input = kh_init(yaml_param_map);
  data.fixed_array_input = kh_init(yaml_array_param_map);
  data.lattice_array_input = kh_init(yaml_array_param_map);
  data.enumerated_array_input = kh_init(yaml_array_param_map);
  data.setting_names = kh_init(yaml_name_set);
  data.param_names = kh_init(yaml_name_set);

  yaml_parser_t parser;
  yaml_parser_initialize(&parser);
  yaml_parser_set_input_file(&parser, file);

  parser_state_t state = {.settings_block = settings_block};
  yaml_event_type_t event_type;
  do {
    yaml_event_t event;

    // Parse the next YAML "event" and handle any errors encountered.
    yaml_parser_parse(&parser, &event);
    if (parser.error != YAML_NO_ERROR) {
      data.error_code = SW_INVALID_YAML;
      data.error_message = dup_string(parser.problem);
      yaml_event_delete(&event);
      yaml_parser_delete(&parser);
      goto return_data;
    }

    // Process the event, using it to populate our YAML data, and handle
    // any errors resulting from properly-formed YAML that doesn't conform
    // to Skywalker's spec.
    handle_yaml_event(&event, &state, &data);
    if (data.error_code != SW_SUCCESS) {
      yaml_event_delete(&event);
      yaml_parser_delete(&parser);
      goto return_data;
    }
    event_type = event.type;
    yaml_event_delete(&event);
  } while (event_type != YAML_STREAM_END_EVENT);
  yaml_parser_delete(&parser);

  // Did we find a settings block?
  if (settings_block && (settings_block[0] != '\0') && !data.settings) {
    data.error_code = SW_SETTINGS_NOT_FOUND;
    data.error_message = new_string("The settings block '%s' was not found.",
                                    settings_block);
    goto return_data;
  }

  // Postprocess input parameters, expanding 3-element lists if needed, and
  // applying log10 operations.
  postprocess_params(&(data.lattice_input), &(data.error_code),
                     &(data.error_message));
  if (!data.error_code) {
    postprocess_params(&(data.enumerated_input), &(data.error_code),
                       &(data.error_message));
  }

  // Expand 3-element lists for array-valued parameters.
  if (!data.error_code) {
    postprocess_array_params(data.lattice_array_input);
    postprocess_array_params(data.enumerated_array_input);
  }

  // Make sure enumerated parameters are all of the same length.
  validate_enumerated_params(data.enumerated_input, data.enumerated_array_input,
                             &(data.num_enumerated_inputs), &(data.error_code),
                             &(data.error_message));

return_data:
  return data;
}

//------------------------------------------------------------------------
//                          Ensemble construction
//------------------------------------------------------------------------

static void assign_fixed_params(yaml_data_t yaml_data, sw_input_t *input) {
  const char *name;
  real_vec_t values;
  kh_foreach(yaml_data.fixed_input, name, values,
    if (kv_size(values) == 1) {
      sw_input_set(input, name, kv_A(values, 0));
    }
  );
}

static void assign_fixed_array_params(yaml_data_t yaml_data, sw_input_t *input) {
  const char *name;
  real_vec_vec_t array_values;
  kh_foreach(yaml_data.fixed_array_input, name, array_values,
    if (kv_size(array_values) == 1) {
      sw_input_set_array(input, name, kv_A(array_values, 0));
    }
  );
}

static void assign_lattice_params(yaml_data_t yaml_data, size_t l, const size_t m,
                                  sw_input_t *input) {
  assert(m < 8);
  size_t N = 0;
  real_vec_t values[7];
  const char *name[7]={NULL,NULL,NULL,NULL,NULL,NULL,NULL};
  for (khiter_t iter = kh_begin(yaml_data.lattice_input);
      iter != kh_end(yaml_data.lattice_input) && N<m; ++iter) {
    if (!kh_exist(yaml_data.lattice_input, iter)) continue;
    real_vec_t value = kh_value(yaml_data.lattice_input, iter);
    if (kv_size(value) > 1) {
      name[N] = kh_key(yaml_data.lattice_input, iter);
      values[N] = value;
      ++N;
    }
  }
  real_vec_vec_t array_values[7];
  const char *array_name[7]={NULL,NULL,NULL,NULL,NULL,NULL,NULL};
  for (khiter_t iter = kh_begin(yaml_data.array_input);
      iter != kh_end(yaml_data.lattice_array_input) && N<m; ++iter) {
    if (!kh_exist(yaml_data.lattice_array_input, iter)) continue;
    real_vec_vec_t value = kh_value(yaml_data.lattice_array_input, iter);
    if (kv_size(value) > 1) {
      array_name[N] = kh_key(yaml_data.lattice_array_input, iter);
      array_values[N] = value;
      ++N;
    }
  }
  size_t n[7];
  for (size_t i = 0; i < m; ++i) {
    n[i] = (name[i]) ? kv_size(values[i]) : kv_size(array_values[i]);
  };
  size_t j[7];
  {
    size_t L = l;
    for (int i = (int)m-1; i > 0; --i) {
      j[i] = L % n[i];
      L /= n[i];
    }
    j[0] = L;
  }
  for  (int i=0; i<m; ++i) {
    if (name[i])
      sw_input_set(input, name[i], kv_A(values[i], j[i]));
    else
      sw_input_set_array(input, array_name[i], kv_A(array_values[i], j[i]));
  }
}

static void assign_enumerated_params(yaml_data_t yaml_data, size_t enum_index,
                                     sw_input_t *input) {
  const char *name;
  real_vec_t values;
  kh_foreach(yaml_data.enumerated_input, name, values,
    sw_input_set(input, name, kv_A(values, enum_index));
  );

  real_vec_vec_t array_values;
  kh_foreach(yaml_data.enumerated_array_input, name, array_values,
    sw_input_set_array(input, name, kv_A(array_values, enum_index));
  );
}

// This type contains results from building an ensemble.
typedef struct sw_build_result_t {
  const char *yaml_file;
  size_t num_inputs;
  sw_input_t *inputs;
  int error_code;
  const char *error_message;
} sw_build_result_t;

// Generates an array of inputs for an ensemble.
static sw_build_result_t build_ensemble(yaml_data_t yaml_data) {
  sw_build_result_t result = {.num_inputs = 1, .error_code = SW_SUCCESS};

  // Count up the number of inputs and parameters.

  // Fixed parameters
  size_t num_fixed_params = kh_size(yaml_data.fixed_input) +
                            kh_size(yaml_data.fixed_array_input);

  // Lattice parameters
  size_t num_lattice_params = 0;
  {
    real_vec_t values;
    kh_foreach_value(yaml_data.lattice_input, values,
      result.num_inputs *= kv_size(values);
      ++num_lattice_params;
    );
  }
  {
    real_vec_vec_t array_values;
    kh_foreach_value(yaml_data.lattice_array_input, array_values,
      result.num_inputs *= kv_size(array_values);
      ++num_lattice_params;
    );
  }

  // Enumerated parameters
  size_t num_enumerated_params = kh_size(yaml_data.enumerated_input) +
                                 kh_size(yaml_data.enumerated_array_input);
  if (num_enumerated_params > 0) {
    result.num_inputs *= yaml_data.num_enumerated_inputs;
  }

  size_t num_params = num_fixed_params + num_lattice_params +
                      num_enumerated_params;
  if (num_params == 0) {
    result.error_code = SW_EMPTY_ENSEMBLE;
    result.error_message = new_string("Ensemble has no members!");
    return result;
  } else if (num_lattice_params > 7) {
    result.error_code = SW_TOO_MANY_LATTICE_PARAMS;
    result.error_message =
      new_string("The given lattice ensemble has %d traversed parameters "
                 "(must be <= 7).", num_lattice_params);
    return result;
  }

  // Try to allocate storage for inputs.
  result.inputs = malloc(sizeof(sw_input_t) * result.num_inputs);
  if (!result.inputs) {
    result.error_code = SW_ENSEMBLE_TOO_LARGE;
    result.error_message =
      new_string("The given lattice ensemble (%zd members) is too large to fit "
                 "into memory.\n", result.num_inputs);
    return result;
  }

  // Build a list of inputs corresponding to all the traversed parameters.
  for (size_t l = 0; l < result.num_inputs; ++l) {
    result.inputs[l].params = kh_init(param_map);
    result.inputs[l].array_params = kh_init(array_param_map);
    assign_fixed_params(yaml_data, &result.inputs[l]);
    assign_fixed_array_params(yaml_data, &result.inputs[l]);
    if (num_lattice_params > 0) {
      size_t lattice_index = l;
      if (yaml_data.num_enumerated_inputs > 0) {
        lattice_index = l / yaml_data.num_enumerated_inputs;
      }
      assign_lattice_params(yaml_data, lattice_index, num_lattice_params,
                            &result.inputs[l]);
    }
    if (num_enumerated_params > 0) {
      size_t enum_index = l % yaml_data.num_enumerated_inputs;
      assign_enumerated_params(yaml_data, enum_index, &result.inputs[l]);
    }
  }

  return result;
}

//------------------------------------------------------------------------
//                      Ensemble loading and writing
//------------------------------------------------------------------------

sw_ensemble_result_t sw_load_ensemble(const char* yaml_file,
                                      const char* settings_block) {
  sw_ensemble_result_t result = {.error_code = 0};

  // Validate inputs.
  if (settings_block && (!strcmp(settings_block, "input"))) {
    result.error_code = SW_INVALID_SETTINGS_BLOCK;
    result.error_message = new_string("Invalid settings block name: '%s'"
                                      " (cannot be 'input')", settings_block);
    return result;
  }

  FILE *file = fopen(yaml_file, "r");
  if (!file) {
    result.error_code = SW_YAML_FILE_NOT_FOUND;
    result.error_message = new_string("The file '%s' could not be opened.",
                                      yaml_file);
    return result;
  }

  // Parse the YAML file, populating a data container.
  yaml_data_t data = parse_yaml(file, settings_block);
  fclose(file);

  if (data.error_code == SW_SUCCESS) {
    sw_build_result_t build_result = build_ensemble(data);
    if (build_result.error_code != SW_SUCCESS) {
      result.error_code = build_result.error_code;
      result.error_message = build_result.error_message;
    } else {
      sw_ensemble_t *ensemble = malloc(sizeof(sw_ensemble_t));
      ensemble->size = build_result.num_inputs;
      ensemble->position = 0;
      ensemble->inputs = build_result.inputs;
      ensemble->outputs = malloc(ensemble->size*sizeof(sw_output_t));
      for (size_t i = 0; i < ensemble->size; ++i) {
        ensemble->outputs[i].metrics = kh_init(param_map);
        ensemble->outputs[i].array_metrics = kh_init(array_param_map);
      }
      result.settings = data.settings;
      data.settings = NULL;
      ensemble->settings = result.settings;
      result.ensemble = ensemble;
    }
  } else {
    result.error_code = data.error_code;
    result.error_message = data.error_message;
  }

  // Clean up.
  free_yaml_data(data);
  free_yaml_strings(); // delete YAML string pool

  return result;
}

size_t sw_ensemble_size(sw_ensemble_t* ensemble) {
  return ensemble->size;
}

bool sw_ensemble_next(sw_ensemble_t *ensemble,
                      sw_input_t **input,
                      sw_output_t **output) {
  if (ensemble->position >= (int)ensemble->size) {
    ensemble->position = 0; // reset for next traversal
    *input = NULL;
    *output = NULL;
    return false;
  }

  *input = &ensemble->inputs[ensemble->position];
  *output = &ensemble->outputs[ensemble->position];
  ++ensemble->position;
  return true;
}

// We use this to sort input and output quantity names.
static int string_cmp(const void *s1, const void *s2) {
  return strcmp(*(const char**)s1, *(const char**)s2);
}

sw_write_result_t sw_ensemble_write(sw_ensemble_t *ensemble,
                                    const char *module_filename) {
  const char *float_format = 4<sizeof(sw_real_t) ? "%.10g, " : "%.6g, ";
  sw_write_result_t result = {.error_code = SW_SUCCESS};
  if (ensemble->size == 0) {
    result.error_code = SW_EMPTY_ENSEMBLE;
    result.error_message = new_string("The given ensemble is empty!");
    return result;
  }
  FILE* file = fopen(module_filename, "w");
  if (!file) {
    result.error_code = SW_WRITE_FAILURE;
    result.error_message = new_string("Could not write ensemble data to '%s'.",
                                      module_filename);
    return result;
  }
  fprintf(file, "# This file was automatically generated by skywalker.\n\n");
  fprintf(file, "from math import nan as nan, inf as inf\n\n");
  fprintf(file,
      "# Object is just a dynamic container that stores input/output data.\n");
  fprintf(file, "class Object(object):\n");
  fprintf(file, "    pass\n\n");

  // Write settings (if present), sorted by name.
  if (ensemble->settings) {
    fprintf(file, "# Settings are stored here.\n");
    fprintf(file, "settings = Object()\n");
    khash_t(string_map) *settings = ensemble->settings->params;
    size_t num_settings = kh_size(settings);
    const char **setting_names = malloc(sizeof(const char*) * num_settings);
    size_t i = 0;
    for (khiter_t iter = kh_begin(settings); iter != kh_end(settings); ++iter) {
      if (!kh_exist(settings, iter)) continue;
      setting_names[i++] = kh_key(settings, iter);
    }
    qsort(setting_names, num_settings, sizeof(const char*), string_cmp);

    for (i = 0; i < num_settings; ++i) {
      const char *name = setting_names[i];
      khiter_t iter = kh_get(string_map, settings, name);
      const char* value = kh_val(settings, iter);
      fprintf(file, "settings.%s = '%s'\n", name, value);
    }
    free(setting_names);
  }

  // Write input data, sorted by quantity name.
  {
    fprintf(file, "# Input is stored here.\n");
    fprintf(file, "input = Object()\n");
    khash_t(param_map) *params_0 = ensemble->inputs[0].params;
    size_t num_inputs = kh_size(params_0);
    const char **input_names = malloc(sizeof(const char*) * num_inputs);
    size_t i = 0;
    for (khiter_t iter = kh_begin(params_0); iter != kh_end(params_0); ++iter) {
      if (!kh_exist(params_0, iter)) continue;
      input_names[i++] = kh_key(params_0, iter);
    }
    qsort(input_names, num_inputs, sizeof(const char*), string_cmp);

    for (i = 0; i < num_inputs; ++i) {
      const char *name = input_names[i];
      fprintf(file, "input.%s = [", name);
      for (size_t i = 0; i < ensemble->size; ++i) {
        khash_t(param_map) *params_i = ensemble->inputs[i].params;
        khiter_t iter = kh_get(param_map, params_i, name);
        sw_real_t value = kh_val(params_i, iter);
        fprintf(file, float_format, value);
      }
      fprintf(file, "]\n");
    }
    free(input_names);

    khash_t(array_param_map) *array_params_0 = ensemble->inputs[0].array_params;
    size_t num_array_inputs = kh_size(array_params_0);
    const char **array_input_names = malloc(sizeof(const char*) * num_array_inputs);
    i = 0;
    for (khiter_t iter = kh_begin(array_params_0); iter != kh_end(array_params_0); ++iter) {
      if (!kh_exist(array_params_0, iter)) continue;
      array_input_names[i++] = kh_key(array_params_0, iter);
    }
    qsort(array_input_names, num_array_inputs, sizeof(const char*), string_cmp);

    for (i = 0; i < num_array_inputs; ++i) {
      const char *name = array_input_names[i];
      fprintf(file, "input.%s = [", name);
      for (size_t i = 0; i < ensemble->size; ++i) {
        khash_t(array_param_map) *array_params_i = ensemble->inputs[i].array_params;
        khiter_t iter = kh_get(array_param_map, array_params_i, name);
        real_vec_t arrays = kh_val(array_params_i, iter);
        size_t size = kv_size(arrays);
        fprintf(file, "[");
        for (size_t i=0; i<size; ++i)
          fprintf(file, float_format, kv_A(arrays, i));
        fprintf(file, "],");
      }
      fprintf(file, "]\n");
    }
    free(array_input_names);
  }

  // Write output data, sorted by quantity name.
  fprintf(file, "\n# Output data is stored here.\n");
  fprintf(file, "output = Object()\n");
  if (ensemble->size > 0) {
    khash_t(param_map) *params_0 = ensemble->outputs[0].metrics;
    size_t num_outputs = kh_size(params_0);
    const char **output_names = malloc(sizeof(const char*) * num_outputs);
    size_t i = 0;
    for (khiter_t iter = kh_begin(params_0); iter != kh_end(params_0); ++iter) {
      if (!kh_exist(params_0, iter)) continue;
      output_names[i++] = kh_key(params_0, iter);
    }
    qsort(output_names, num_outputs, sizeof(const char*), string_cmp);

    for (i = 0; i < num_outputs; ++i) {
      const char *name = output_names[i];
      fprintf(file, "output.%s = [", name);
      for (size_t i = 0; i < ensemble->size; ++i) {
        khash_t(param_map) *params_i = ensemble->outputs[i].metrics;
        khiter_t iter = kh_get(param_map, params_i, name);
        sw_real_t value = kh_val(params_i, iter);
        if (isnan(value)) {
          fprintf(file, "nan, ");
        } else {
          fprintf(file, float_format, value);
        }
      }
      fprintf(file, "]\n");
    }
    free(output_names);

    khash_t(array_param_map) *array_params_0 = ensemble->outputs[0].array_metrics;
    size_t num_array_outputs = kh_size(array_params_0);
    const char **array_output_names = malloc(sizeof(const char*) * num_array_outputs);
    i = 0;
    for (khiter_t iter = kh_begin(array_params_0); iter != kh_end(array_params_0); ++iter) {
      if (!kh_exist(array_params_0, iter)) continue;
      array_output_names[i++] = kh_key(array_params_0, iter);
    }
    qsort(array_output_names, num_array_outputs, sizeof(const char*), string_cmp);

    for (i = 0; i < num_array_outputs; ++i) {
      const char *name = array_output_names[i];
      fprintf(file, "output.%s = [", name);
      for (size_t i = 0; i < ensemble->size; ++i) {
        khash_t(array_param_map) *array_params_i = ensemble->outputs[i].array_metrics;
        khiter_t iter = kh_get(array_param_map, array_params_i, name);
        real_vec_t arrays = kh_val(array_params_i, iter);
        size_t size = kv_size(arrays);
        fprintf(file, "[");
        for (size_t i=0; i<size; ++i) {
          if (isnan(kv_A(arrays, i))) {
            fprintf(file, "nan, ");
          } else {
            fprintf(file, float_format, kv_A(arrays, i));
          }
        }
        fprintf(file, "],");
      }
      fprintf(file, "]\n");
    }
    free(array_output_names);
  }

  fclose(file);
  return result;
}

void sw_ensemble_free(sw_ensemble_t *ensemble) {
  if (ensemble->settings)
    sw_settings_free(ensemble->settings);
  if (ensemble->inputs) {
    for (size_t i = 0; i < ensemble->size; ++i) {
      kh_destroy(param_map, ensemble->inputs[i].params);
      real_vec_t arrays;
      kh_foreach_value(ensemble->inputs[i].array_params, arrays,
        kv_destroy(arrays);
      );
      kh_destroy(array_param_map, ensemble->inputs[i].array_params);
      kh_destroy(param_map, ensemble->outputs[i].metrics);
      kh_foreach_value(ensemble->outputs[i].array_metrics, arrays,
        kv_destroy(arrays);
      );
      kh_destroy(array_param_map, ensemble->outputs[i].array_metrics);
    }
    free(ensemble->inputs);
    free(ensemble->outputs);
  }
  free(ensemble);
}

//----------------------------
// Skywalker Fortran bindings
//----------------------------

void sw_load_ensemble_f90(const char *yaml_file, const char *settings_block,
                          sw_settings_t **settings, sw_ensemble_t **ensemble,
                          int *error_code, const char **error_message) {
  sw_ensemble_result_t result = sw_load_ensemble(yaml_file, settings_block);
  if (result.error_code == SW_SUCCESS) {
    *settings = result.settings;
    *ensemble = result.ensemble;
  }
  *error_code = result.error_code;
  *error_message = result.error_message;
}

void sw_settings_get_f90(sw_settings_t *settings, const char *name,
                         const char **value, int *error_code,
                         const char **error_message) {
  sw_settings_result_t result = sw_settings_get(settings, name);
  if (result.error_code == SW_SUCCESS) {
    *value = result.value;
  }
  *error_code = result.error_code;
  *error_message = result.error_message;
}

void sw_input_get_f90(sw_input_t *input, const char *name, sw_real_t *value,
                      int *error_code, const char **error_message) {
  sw_input_result_t result = sw_input_get(input, name);
  if (result.error_code == SW_SUCCESS) {
    *value = result.value;
  }
  *error_code = result.error_code;
  *error_message = result.error_message;
}

void sw_input_get_array_f90(sw_input_t *input, const char *name,
                            sw_real_t **values, size_t *size,
                            int *error_code, const char **error_message) {
  sw_input_array_result_t result = sw_input_get_array(input, name);
  if (result.error_code == SW_SUCCESS) {
    *values = result.values;
    *size = result.size;
  }
  *error_code = result.error_code;
  *error_message = result.error_message;
}

void sw_output_set_array_f90(sw_output_t *output, const char *name,
                             const sw_real_t *values, size_t *size) {
  sw_output_set_array(output, name, values, *size);
}


void sw_ensemble_write_f90(sw_ensemble_t *ensemble, const char *module_filename,
                          int *error_code, const char **error_message) {
  sw_write_result_t result = sw_ensemble_write(ensemble, module_filename);
  *error_code = result.error_code;
  *error_message = result.error_message;
}

// Returns a newly-allocated C string for the given Fortran string pointer with
// the given length. Strings of this sort are freed at program exit.
const char* sw_new_c_string_f90(char* f_str_ptr, int f_str_len) {
  char* s = malloc(sizeof(char) * (f_str_len+1));
  memcpy(s, f_str_ptr, sizeof(char) * f_str_len);
  s[f_str_len] = '\0';
  append_string((const char*)s);
  return (const char*)s;
}

#ifdef __cplusplus
} // extern "C"
#endif
