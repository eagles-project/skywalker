#include <skywalker/skywalker.h>

#include <assert.h>
#include <stdio.h>
#include <strings.h>

// Destroys an input, freeing all allocated resources.
static void sw_input_free(sw_input_t *input) {
  for (size_t i = 0; i < input->num_params; ++i) {
    free(input->param_names[i]);
  }
  free(input->param_names);
  free(input->param_values);
  free(input);
}

struct sw_output_t {
  // Number of output metrics.
  size_t num_params;

  // Names of output metrics (of length num_metrics)
  const char **metric_names;

  // Values of output metrics (of length num_metrics)
  real_t *metric_values;
};

// Destroys an output, freeing all allocated resources.
static void sw_output_free(sw_output_t *output) {
  for (size_t i = 0; i < output->num_metrics; ++i) {
    free(output->metric_names[i]);
    free(output->metric_values[i]);
  }
  free(output->metric_names);
  free(output->metric_values);
  free(output);
}

// This type identifies whether we're running a "lattice" or "enumeration"
// ensemble.
typedef enum sw_ens_type_t {
  LATTICE = 0,
  ENUMERATION
} sw_ens_type_t;

// ensemble type
struct sw_ensemble_t {
  sw_settings_t settings;
  sw_ens_type_t type;
  size_t size;
  sw_input_t *inputs;
  sw_output_t *outputs;
  const char *error_message;
};

// Destroys an ensemble, freeing all allocated resources.
static void sw_ensemble_free(sw_ensemble_t *ensemble) {
  if (ensemble->inputs) {
    for (size_t i = 0; i < ensemble->size; ++i) {
      sw_input_free(&(ensemble->inputs[i]));
      sw_output_free(&(ensemble->outputs[i]));
    }
    free(ensemble->inputs);
    free(ensemble->outputs);
  }
  if (ensemble->error_message)
    free(ensemble->error_message);
  free(ensemble);
}

// This type stores data parsed from YAML.
typedef struct sw_yaml_data_t {
  sw_ens_type_t ensemble_type;
  sw_settings_t settings;
  int error_code;
  const char* error_message;
} sw_yaml_data_t;

// Parses a YAML file, returning the results.
static sw_yaml_data_t parse_yaml(const char* yaml_file,
                                 const char* settings_block) {
  sw_yaml_data_t data = {.error_code = 0};
  return data;
}

// This type contains results from building an ensemble.
typedef struct sw_build_result_t {
  size_t num_inputs;
  sw_input_t *inputs;
  int error_code;
  const char *error_message;
} sw_build_result_t;

// Generates an array of inputs for a lattice ensemble.
static sw_build_result_t build_lattice_ensemble(sw_yaml_data_t yaml_data) {
  // Count up the number of inputs defined by the parameter walk thingy,
  // excluding those parameters specified.
  size_t num_inputs = 1, num_params = 0;
  for (auto param : ensemble) {
    if (excluded_params.find(param.first) ==
        excluded_params.end()) {          // not excluded
      num_inputs *= param.second.size();  // set of parameter values
      num_params++;
    }
  }
  EKAT_REQUIRE_MSG(((num_params >= 1) and (num_params <= 7)),
                   "Invalid number of overridden parameters ("
                       << num_params << ", must be 1-7).");

  // Start from reference data and build a list of inputs corresponding to all
  // the overridden parameters. This involves some ugly index magic based on the
  // number of parameters.
  std::vector<InputData> inputs(num_inputs, ref_input);
  for (size_t l = 0; l < num_inputs; ++l) {
    if (num_params == 1) {
      auto iter = ensemble.begin();
      auto name = iter->first;
      const auto& vals = iter->second;
      inputs[l][name] = vals[l];
    } else if (num_params == 2) {
      auto iter = ensemble.begin();
      auto name1 = iter->first;
      const auto& vals1 = iter->second;
      iter++;
      auto name2 = iter->first;
      const auto& vals2 = iter->second;
      size_t n2 = vals2.size();
      size_t j1 = l / n2;
      size_t j2 = l - n2 * j1;
      inputs[l][name1] = vals1[j1];
      inputs[l][name2] = vals2[j2];
    } else if (num_params == 3) {
      auto iter = ensemble.begin();
      auto name1 = iter->first;
      const auto& vals1 = iter->second;
      iter++;
      auto name2 = iter->first;
      const auto& vals2 = iter->second;
      iter++;
      auto name3 = iter->first;
      const auto& vals3 = iter->second;
      size_t n2 = vals2.size();
      size_t n3 = vals3.size();
      size_t j1 = l / (n2 * n3);
      size_t j2 = (l - n2 * n3 * j1) / n3;
      size_t j3 = l - n2 * n3 * j1 - n3 * j2;
      inputs[l][name1] = vals1[j1];
      inputs[l][name2] = vals2[j2];
      inputs[l][name3] = vals3[j3];
    } else if (num_params == 4) {
      auto iter = ensemble.begin();
      auto name1 = iter->first;
      const auto& vals1 = iter->second;
      iter++;
      auto name2 = iter->first;
      const auto& vals2 = iter->second;
      iter++;
      auto name3 = iter->first;
      const auto& vals3 = iter->second;
      iter++;
      auto name4 = iter->first;
      const auto& vals4 = iter->second;
      size_t n2 = vals2.size();
      size_t n3 = vals3.size();
      size_t n4 = vals4.size();
      size_t j1 = l / (n2 * n3 * n4);
      size_t j2 = (l - n2 * n3 * n4 * j1) / (n3 * n4);
      size_t j3 = (l - n2 * n3 * n4 * j1 - n3 * n4 * j2) / n4;
      size_t j4 = l - n2 * n3 * n4 * j1 - n3 * n4 * j2 - n4 * j3;
      inputs[l][name1] = vals1[j1];
      inputs[l][name2] = vals2[j2];
      inputs[l][name3] = vals3[j3];
      inputs[l][name4] = vals4[j4];
    } else if (num_params == 5) {
      auto iter = ensemble.begin();
      auto name1 = iter->first;
      const auto& vals1 = iter->second;
      iter++;
      auto name2 = iter->first;
      const auto& vals2 = iter->second;
      iter++;
      auto name3 = iter->first;
      const auto& vals3 = iter->second;
      iter++;
      auto name4 = iter->first;
      const auto& vals4 = iter->second;
      iter++;
      auto name5 = iter->first;
      const auto& vals5 = iter->second;
      size_t n2 = vals2.size();
      size_t n3 = vals3.size();
      size_t n4 = vals4.size();
      size_t n5 = vals5.size();
      size_t j1 = l / (n2 * n3 * n4 * n5);
      size_t j2 = (l - n2 * n3 * n4 * n5 * j1) / (n3 * n4 * n5);
      size_t j3 = (l - n2 * n3 * n4 * n5 * j1 - n3 * n4 * n5 * j2) / (n4 * n5);
      size_t j4 =
          (l - n2 * n3 * n4 * n5 * j1 - n3 * n4 * n5 * j2 - n4 * n5 * j3) / n5;
      size_t j5 = l - n2 * n3 * n4 * n5 * j1 - n3 * n4 * n5 * j2 -
                  n4 * n5 * j3 - n5 * j4;
      inputs[l][name1] = vals1[j1];
      inputs[l][name2] = vals2[j2];
      inputs[l][name3] = vals3[j3];
      inputs[l][name4] = vals4[j4];
      inputs[l][name5] = vals5[j5];
    } else if (num_params == 6) {
      auto iter = ensemble.begin();
      auto name1 = iter->first;
      const auto& vals1 = iter->second;
      iter++;
      auto name2 = iter->first;
      const auto& vals2 = iter->second;
      iter++;
      auto name3 = iter->first;
      const auto& vals3 = iter->second;
      iter++;
      auto name4 = iter->first;
      const auto& vals4 = iter->second;
      iter++;
      auto name5 = iter->first;
      const auto& vals5 = iter->second;
      iter++;
      auto name6 = iter->first;
      const auto& vals6 = iter->second;
      size_t n2 = vals2.size();
      size_t n3 = vals3.size();
      size_t n4 = vals4.size();
      size_t n5 = vals5.size();
      size_t n6 = vals6.size();
      size_t j1 = l / (n2 * n3 * n4 * n5 * n6);
      size_t j2 = (l - n2 * n3 * n4 * n5 * n6 * j1) / (n3 * n4 * n5 * n6);
      size_t j3 = (l - n2 * n3 * n4 * n5 * n6 * j1 - n3 * n4 * n5 * n6 * j2) /
                  (n4 * n5 * n6);
      size_t j4 = (l - n2 * n3 * n4 * n5 * n6 * j1 - n3 * n4 * n5 * n6 * j2 -
                   n4 * n5 * n6 * j3) /
                  (n5 * n6);
      size_t j5 = (l - n2 * n3 * n4 * n5 * n6 * j1 - n3 * n4 * n5 * n6 * j2 -
                   n4 * n5 * n6 * j3 - n5 * n6 * j4) /
                  n6;
      size_t j6 = l - n2 * n3 * n4 * n5 * n6 * j1 - n3 * n4 * n5 * n6 * j2 -
                  n4 * n5 * n6 * j3 - n5 * n6 * j4 - n6 * j5;
      inputs[l][name1] = vals1[j1];
      inputs[l][name2] = vals2[j2];
      inputs[l][name3] = vals3[j3];
      inputs[l][name4] = vals4[j4];
      inputs[l][name5] = vals5[j5];
      inputs[l][name6] = vals6[j6];
    } else {  // if (num_params == 7)
      auto iter = ensemble.begin();
      auto name1 = iter->first;
      const auto& vals1 = iter->second;
      iter++;
      auto name2 = iter->first;
      const auto& vals2 = iter->second;
      iter++;
      auto name3 = iter->first;
      const auto& vals3 = iter->second;
      iter++;
      auto name4 = iter->first;
      const auto& vals4 = iter->second;
      iter++;
      auto name5 = iter->first;
      const auto& vals5 = iter->second;
      iter++;
      auto name6 = iter->first;
      const auto& vals6 = iter->second;
      iter++;
      auto name7 = iter->first;
      const auto& vals7 = iter->second;
      size_t n2 = vals2.size();
      size_t n3 = vals3.size();
      size_t n4 = vals4.size();
      size_t n5 = vals5.size();
      size_t n6 = vals6.size();
      size_t n7 = vals7.size();
      size_t j1 = l / (n2 * n3 * n4 * n5 * n6 * n7);
      size_t j2 =
          (l - n2 * n3 * n4 * n5 * n6 * n7 * j1) / (n3 * n4 * n5 * n6 * n7);
      size_t j3 =
          (l - n2 * n3 * n4 * n5 * n6 * n7 * j1 - n3 * n4 * n5 * n6 * n7 * j2) /
          (n4 * n5 * n6 * n7);
      size_t j4 = (l - n2 * n3 * n4 * n5 * n6 * n7 * j1 -
                   n3 * n4 * n5 * n6 * n7 * j2 - n4 * n5 * n6 * n7 * j3) /
                  (n5 * n6 * n7);
      size_t j5 =
          (l - n2 * n3 * n4 * n5 * n6 * n7 * j1 - n3 * n4 * n5 * n6 * n7 * j2 -
           n4 * n5 * n6 * n7 * j3 - n5 * n6 * n7 * j4) /
          (n6 * n7);
      size_t j6 =
          (l - n2 * n3 * n4 * n5 * n6 * n7 * j1 - n3 * n4 * n5 * n6 * n7 * j2 -
           n4 * n5 * n6 * n7 * j3 - n5 * n6 * n7 * j4 - n6 * n7 * j5) /
          n7;
      size_t j7 = l - n2 * n3 * n4 * n5 * n6 * n7 * j1 -
                  n3 * n4 * n5 * n6 * n7 * j2 - n4 * n5 * n6 * n7 * j3 -
                  n5 * n6 * n7 * j4 - n6 * n7 * j5 - n7 * j6;
      inputs[l][name1] = vals1[j1];
      inputs[l][name2] = vals2[j2];
      inputs[l][name3] = vals3[j3];
      inputs[l][name4] = vals4[j4];
      inputs[l][name5] = vals5[j5];
      inputs[l][name6] = vals6[j6];
      inputs[l][name7] = vals7[j7];
    }
  }

  return inputs;
}

static sw_build_result_t build_enumeration_ensemble(sw_yaml_data_t yaml_data) {
  std::string first_name;
  size_t num_inputs = 0;
  for (auto param : ensemble) {
    if (num_inputs == 0) {
      num_inputs = param.second.size();  // set of parameter values
      first_name = param.first;
    } else if (num_inputs != param.second.size()) {
      throw YamlException(
          std::string("Invalid enumeration: Parameter ") + param.first +
          std::string(" has a different number of values than ") + first_name +
          std::string(" (must match)"));
    }
  }

  if (num_inputs == 0) {
    throw YamlException("No ensemble members!");
  }

  // Trudge through all the ensemble parameters as defined.
  std::vector<InputData> inputs(num_inputs, ref_input);
  for (size_t l = 0; l < num_inputs; ++l) {
    for (auto iter = ensemble.begin(); iter != ensemble.end(); ++iter) {
      auto name = iter->first;
      const auto& vals = iter->second;
      inputs[l][name] = vals[l];
    }
  }

  return inputs;
}

sw_load_result_t sw_load_ensemble(const char* yaml_file,
                                  const char* settings_block) {
  sw_yaml_data_t data = parse_yaml(yaml_file, settings_block);
  sw_load_result_t result = {.error_code = data.error_code,
                             .error_message = data.error_message};
  if (data.error_code == SKYWALKER_SUCCESS) {
  }
  return result;
}

bool sw_ensemble_next(sw_ensemble_t *ensemble,
                      int *pos,
                      const sw_input_t **input,
                      sw_output_t **output) {
  if (*pos >= (int)ensemble->size) {
    return false;
  }

  *input = ensemble->inputs[*pos];
  *output = ensemble->outputs[*pos];
  return true;
}

void sw_ensemble_write(sw_ensemble_t *ensemble, const char *module_filename) {
  FILE* file = fopen(module_filename, "w");
  fprintf(file, "# This file was automatically generated by skywalker.\n\n");
  fprintf(file, "from math import nan as nan\n\n");
  fprintf(
      file,
      "# Object is just a dynamic container that stores input/output data.\n");
  fprintf(file, "class Object(object):\n");
  fprintf(file, "    pass\n\n");

  // Write input data.
  fprintf(file, "# Input is stored here.\n");
  fprintf(file, "input = Object()\n");
  if (ensemble->size > 0) {
    for (size_t i = 0; i < inputs[0].num_params; ++i) {
      fprintf(file, "input.%s = [", inputs[0].param_names[i]);
      for (size_t j = 0; j < ensemble->size; ++j) {
        fprintf(file, "%g, ", inputs[j].param_values[i]);
      }
  }
  fprintf(file, "]\n");

  // Write output data.
  fprintf(file, "\n# Output data is stored here.\n");
  fprintf(file, "output = Object()\n");
  if (ensemble->size > 0) {
    for (size_t m = 0; m < outputs[0].num_metrics; ++m) {
      fprintf(file, "output.%s = [", outputs[0].metric_names[m]);
      for (size_t i = 0; i < ensemble->size; ++i) {
        real_t value = outputs[i].metric_values[m];
        if (value == value) {
          fprintf(file, "%g, ", value);
        } else {
          fprintf(file, "nan, ");
        }
      }
      fprintf(file, "]\n");
    }
  }

  fclose(file);
}

//----------------------------
// Skywalker Fortran bindings
//----------------------------
// The Skywalker Fortran interface is tailored to the needs of the MAM box
// model. At any given time, its design is likely to reflect the needs of a
// handful of legacy MAM-related codes for comparison with Haero. In this
// sense, it's not a "faithful" Fortran representation of the Skywalker C++
// library.

// This container holds "live" instances of Skywalker Fortran ensemble data,
// which is managed by this Fortran bridge.
static std::set<EnsembleData*>* fortran_ensembles_ = nullptr;

static void destroy_ensembles() {
  for (auto ensemble : *fortran_ensembles_) {
    delete ensemble;
  }
  delete fortran_ensembles_;
  fortran_ensembles_ = nullptr;
}

/// Parses the given file, assuming the given named aerosol configuration,
/// returning an opaque pointer to the ensemble data.
/// @param [in] aerosol_config The named aerosol configuration. The only valid
///                            configuration at this time is "mam4".
/// @param [in] filename The name of the YAML file containing ensemble data.
/// @param [in] model_impl The name of the model implementation (typically
///                        "haero" or "mam").
void* sw_load_ensemble(const char* aerosol_config, const char* filename,
                       const char* model_impl) {
  // Create a ParameterWalk object from the given config and file.
  try {
    auto param_walk =
        skywalker::load_ensemble(aerosol_config, filename, model_impl);

    // Create an ensemble, allocating storage for output data equal in length
    // to the given input data.
    auto ensemble = new EnsembleData;
    ensemble->program_name = param_walk.program_name;
    ensemble->program_params = param_walk.program_params;
    ensemble->inputs = param_walk.gather_inputs();
    OutputData ref_output(param_walk.aero_config);
    ensemble->outputs =
        std::vector<OutputData>(ensemble->inputs.size(), ref_output);
    // Size up the output data arrays to make our life easier down the line.
    for (size_t i = 0; i < ensemble->inputs.size(); ++i) {
      const auto& input = ensemble->inputs[i];
      auto& output = ensemble->outputs[i];
      output.interstitial_number_mix_ratios.resize(
          input.interstitial_number_mix_ratios.size());
      output.cloud_number_mix_ratios.resize(
          input.cloud_number_mix_ratios.size());
      output.interstitial_aero_mmrs.resize(input.interstitial_aero_mmrs.size());
      output.cloud_aero_mmrs.resize(input.cloud_aero_mmrs.size());
      output.gas_mmrs.resize(input.gas_mmrs.size());
    }

    // Track this ensemble, storing its pointer for future reference.
    if (fortran_ensembles_ == nullptr) {
      fortran_ensembles_ = new std::set<EnsembleData*>();
      atexit(destroy_ensembles);
    }
    fortran_ensembles_->emplace(ensemble);
    auto ensemble_ptr = reinterpret_cast<void*>(ensemble);
    return ensemble_ptr;
  } catch (std::exception& e) {
    fprintf(stderr, "Error loading ensemble from %s: %s\n", filename, e.what());
    return nullptr;
  }
}

/// Returns the name of the process being studied by the ensemble.
const char* sw_ensemble_program_name(void* ensemble) {
  auto data = reinterpret_cast<EnsembleData*>(ensemble);
  return data->program_name.c_str();
}

/// Returns the number of parameters passed to the process being studied by the
/// ensemble.
int sw_ensemble_num_program_params(void* ensemble) {
  auto data = reinterpret_cast<EnsembleData*>(ensemble);
  return static_cast<int>(data->program_params.size());
}

/// Sets the given pointers to the name and value of the parameter with the
/// given index, passed to the process for the ensemble.
void sw_ensemble_get_program_param(void* ensemble, int index, const char** name,
                                   const char** value) {
  auto data = reinterpret_cast<EnsembleData*>(ensemble);
  int i = 0;
  for (const auto& param_kv : data->program_params) {
    if (i ==
        index - 1) {  // if the (1-based) index matches, fill in the blanks.
      *name = param_kv.first.c_str();
      *value = param_kv.second.c_str();
      break;
    } else {  // otherwise, keep going.
      ++i;
    }
  }
}

/// Returns the number of inputs (members) for the given ensemble data.
int sw_ensemble_size(void* ensemble) {
  auto data = reinterpret_cast<EnsembleData*>(ensemble);
  return data->inputs.size();
}

/// Fetches array sizes for members in the given ensemble.
void sw_ensemble_get_array_sizes(void* ensemble, int* num_modes,
                                 int* num_populations, int* num_gases) {
  auto data = reinterpret_cast<EnsembleData*>(ensemble);
  EKAT_REQUIRE(not data->inputs.empty());

  // Get the aerosol configuration from the first input.
  const auto& config = data->inputs[0].aero_config;

  // Read off the data.
  *num_modes = config.num_modes();
  *num_populations = config.num_aerosol_populations;
  *num_gases = config.num_gases();
}

/// Fetches the number of aerosols present in each mode, which can be used
/// to map between population and aerosol indices. The output array is sized
/// to store the number of aerosols in each mode.
void sw_ensemble_get_modal_aerosol_sizes(void* ensemble,
                                         int* aerosols_per_mode) {
  auto data = reinterpret_cast<EnsembleData*>(ensemble);
  EKAT_REQUIRE(not data->inputs.empty());

  // Get the aerosol configuration from the first ensemble member.
  const auto& config = data->inputs[0].aero_config;

  // Fetch the numbers of species per mode.
  for (int m = 0; m < config.num_modes(); ++m) {
    const auto mode_species = config.aerosol_species_for_mode(m);
    aerosols_per_mode[m] = int(mode_species.size());
  }
}

/// Fetches an opaque pointer to the ith set of input data from the given
/// ensemble.
void* sw_ensemble_input(void* ensemble, int i) {
  auto data = reinterpret_cast<EnsembleData*>(ensemble);
  EKAT_REQUIRE(i > 0);
  EKAT_REQUIRE(i <= data->inputs.size());
  InputData* input = &(data->inputs[i - 1]);
  return reinterpret_cast<void*>(input);
}

/// Fetches timestepping data from the given ensemble input data pointer.
void sw_input_get_timestepping(void* input, Real* dt, Real* total_time) {
  auto inp = reinterpret_cast<InputData*>(input);
  *dt = inp->dt;
  *total_time = inp->total_time;
}

/// Fetches atmosphere data from the given ensemble input data pointer.
void sw_input_get_atmosphere(void* input, Real* temperature, Real* pressure,
                             Real* vapor_mixing_ratio, Real* height,
                             Real* hydrostatic_dp,
                             Real* planetary_boundary_layer_height) {
  auto inp = reinterpret_cast<InputData*>(input);
  *temperature = inp->temperature;
  *pressure = inp->pressure;
  *vapor_mixing_ratio = inp->vapor_mixing_ratio;
  *height = inp->height;
  *hydrostatic_dp = inp->hydrostatic_dp;
  *planetary_boundary_layer_height = inp->planetary_boundary_layer_height;
}

/// Fetches aerosol data from the given ensemble input data pointer. All output
/// arguments are arrays that are properly sized to store aerosol data.
void sw_input_get_aerosols(void* input, Real* interstitial_number_mix_ratios,
                           Real* cloud_number_mix_ratios,
                           Real* interstitial_aero_mmrs,
                           Real* cloud_aero_mmrs) {
  auto inp = reinterpret_cast<InputData*>(input);
  std::copy(inp->interstitial_number_mix_ratios.begin(),
            inp->interstitial_number_mix_ratios.end(),
            interstitial_number_mix_ratios);
  std::copy(inp->cloud_number_mix_ratios.begin(),
            inp->cloud_number_mix_ratios.end(), cloud_number_mix_ratios);
  std::copy(inp->interstitial_aero_mmrs.begin(),
            inp->interstitial_aero_mmrs.end(), interstitial_aero_mmrs);
  std::copy(inp->cloud_aero_mmrs.begin(), inp->cloud_aero_mmrs.end(),
            cloud_aero_mmrs);
}

/// Fetches gas data from the given ensemble input data pointer. The output
/// argument is an array properly sized to store gas mass mixing ratios.
void sw_input_get_gases(void* input, Real* gas_mmrs) {
  auto inp = reinterpret_cast<InputData*>(input);
  std::copy(inp->gas_mmrs.begin(), inp->gas_mmrs.end(), gas_mmrs);
}

/// Fetches the value of a named user parameter for the given ensemble input.
void sw_input_get_user_param(void* input, const char* name, Real* value) {
  auto inp = reinterpret_cast<InputData*>(input);
  *value = inp->user_params[name];
}

/// Fetches an opaque pointer to the ith set of output data from the given
/// ensemble.
void* sw_ensemble_output(void* ensemble, int i) {
  auto data = reinterpret_cast<EnsembleData*>(ensemble);
  EKAT_REQUIRE(i > 0);
  EKAT_REQUIRE(i <= data->outputs.size());
  OutputData* output = &(data->outputs[i - 1]);
  return reinterpret_cast<void*>(output);
}

/// Sets aerosol data for the given ensemble output data pointer.
void sw_output_set_aerosols(void* output, Real* interstitial_number_mix_ratios,
                            Real* cloud_number_mix_ratios,
                            Real* interstitial_aero_mmrs,
                            Real* cloud_aero_mmrs) {
  auto outp = reinterpret_cast<OutputData*>(output);
  size_t num_modes = outp->interstitial_number_mix_ratios.size();
  size_t num_pops = outp->interstitial_aero_mmrs.size();
  std::copy(interstitial_number_mix_ratios,
            interstitial_number_mix_ratios + num_modes,
            outp->interstitial_number_mix_ratios.begin());
  std::copy(cloud_number_mix_ratios, cloud_number_mix_ratios + num_modes,
            outp->cloud_number_mix_ratios.begin());
  std::copy(interstitial_aero_mmrs, interstitial_aero_mmrs + num_pops,
            outp->interstitial_aero_mmrs.begin());
  std::copy(cloud_aero_mmrs, cloud_aero_mmrs + num_pops,
            outp->cloud_aero_mmrs.begin());
}

/// Sets gas data for the given ensemble output data pointer.
void sw_output_set_gases(void* output, Real* gas_mmrs) {
  auto outp = reinterpret_cast<OutputData*>(output);
  size_t num_gases = outp->gas_mmrs.size();
  std::copy(gas_mmrs, gas_mmrs + num_gases, outp->gas_mmrs.begin());
}

/// Sets the value of a named metric for the given ensemble output.
void sw_output_set_metric(void* output, const char* name, Real value) {
  auto outp = reinterpret_cast<OutputData*>(output);
  outp->metrics[name] = value;
}

// Writes out a Python module containing input and output data for the
// ǥiven ensemble to the given filename.
void sw_ensemble_write_py_module(void* ensemble, const char* filename) {
  auto data = reinterpret_cast<EnsembleData*>(ensemble);
  skywalker::write_py_module(data->inputs, data->outputs, filename);
}

/// Frees all memory associated with the ensemble, including input and output
/// data.
void sw_ensemble_free(void* ensemble) {
  if (fortran_ensembles_ != nullptr) {
    auto data = reinterpret_cast<EnsembleData*>(ensemble);
    auto iter = fortran_ensembles_->find(data);
    if (iter != fortran_ensembles_->end()) {
      fortran_ensembles_->erase(iter);
      delete data;
    }
  }
}
}
