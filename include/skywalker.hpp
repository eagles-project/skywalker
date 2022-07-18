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

#ifndef SKYWALKER_HPP
#define SKYWALKER_HPP

#include <skywalker.h>

#include <exception>
#include <functional>
#include <string>
#include <vector>

namespace skywalker {

class Ensemble;

// An exception for handling Skywalker-related errors.
class Exception : public std::exception {
 public:
  /// Constructs an exception containing the given descriptive message.
  Exception(const std::string& message) : _message(message) {}

  const char* what() const throw() { return _message.c_str(); }

 private:
  std::string _message;
};

// Prints a banner containing Skywalker's version info to stderr.
inline void print_banner() {
  sw_print_banner();
}

// Precision of real numbers
using Real = sw_real_t;

// A table of string-valued settings, read from a settings block in a YAML
// file.
class Settings final {
 public:
  // Not directly constructible.
  Settings() = delete;

  Settings(const Settings& rhs): settings_(rhs.settings_) {}

  Settings& operator=(const Settings& rhs) {
    if (&rhs != this) {
      settings_ = rhs.settings_;
    }
    return *this;
  }

  ~Settings() = default;

  // Returns true if the setting with the given name exists within the given
  // settings instance, false otherwise.
  bool has(const std::string& name) const {
    return sw_settings_has(settings_, name.c_str());
  }

  // Retrieves a (string-valued) setting with the given name, throwing an
  // exception if it doesn't exist.
  std::string get(const std::string& name) const {
    auto result = sw_settings_get(settings_, name.c_str());
    if (result.error_code == SW_SUCCESS) {
      return std::string(result.value);
    } else {
      throw Exception(result.error_message);
    }
  }

 private:
  explicit Settings(sw_settings_t *s): settings_(s) {}
  sw_settings_t *settings_;

  friend class Ensemble;
};

// A set of named, real-valued input parameters corresponding to a single
// ensemble member.
class Input final {
 public:
  Input() = default;;
  Input(const Input& rhs): input_(rhs.input_) {}
  Input& operator=(const Input& rhs) {
    if (&rhs != this) {
      input_ = rhs.input_;
    }
    return *this;
  }

  ~Input() = default;

  // Returns true if the input parameter with the given name exists within the
  // given input instance, false otherwise.
  bool has(const std::string& name) const {
    return sw_input_has(input_, name.c_str());
  }

  // Retrieves a (real-valued) parameter with the given name, throwing an
  // exception if it doesn't exist.
  Real get(const std::string& name) const {
    auto result = sw_input_get(input_, name.c_str());
    if (result.error_code == SW_SUCCESS) {
      return result.value;
    } else {
      throw Exception(result.error_message);
    }
  }

  // Returns true if an input array parameter with the given name exists within
  // the given input instance, false otherwise.
  bool has_array(const std::string& name) const {
    return sw_input_has_array(input_, name.c_str());
  }

  // Retrieves a (real-valued) array parameter with the given name, throwing an
  // exception if it doesn't exist.
  std::vector<Real> get_array(const std::string& name) const {
    auto result = sw_input_get_array(input_, name.c_str());
    if (result.error_code == SW_SUCCESS) {
      return std::vector<Real>(result.values, result.values + result.size);
    } else {
      throw Exception(result.error_message);
    }
  }

 private:
  explicit Input(sw_input_t *i): input_(i) {}
  sw_input_t *input_;
  friend class Ensemble;
};

// A set of named, real-valued output parameters corresponding to a single
// ensemble member.
class Output final {
 public:
  Output() = default;
  Output(const Output& rhs): output_(rhs.output_) {}
  Output& operator=(const Output& rhs) {
    if (&rhs != this) {
      output_ = rhs.output_;
    }
    return *this;
  }

  ~Output() = default;

  // Sets a (real-valued) parameter with the given name. This operation cannot
  // fail under normal circumstances.
  void set(const std::string& name, Real value) const {
    sw_output_set(output_, name.c_str(), value);
  }

  // Sets (real-valued) parameters in an array with the given name. This
  // operation cannot fail under normal circumstances.
  void set(const std::string& name, const std::vector<Real> &values) const {
    sw_output_set_array(output_, name.c_str(), values.data(), values.size());
  }

 private:
  explicit Output(sw_output_t *o): output_(o) {}
  sw_output_t *output_;
  friend class Ensemble;
};

// An ensemble that has been loaded from a skywalker input file.
class Ensemble final {
 public:
  // Not directly constructible, copy constructible, or assignable.
  Ensemble() = delete;
  Ensemble(const Ensemble&) = delete;
  Ensemble& operator=(const Ensemble&) = delete;

  ~Ensemble() {
    sw_ensemble_free(ensemble_);
  }

  // Returns the settings associated with this ensemble.
  const Settings& settings() const { return settings_; }

  // Iterates over all ensemble members, applying the given function f to
  // each input/output pair.
  void process(std::function<void(const Input&, Output&)> f) {
    Input i;
    Output o;
    while (sw_ensemble_next(ensemble_, &(i.input_), &(o.output_))) {
      f(i, o);
    }
  }

  // Returns the size of the ensemble (number of members).
  size_t size() const { return sw_ensemble_size(ensemble_); }

  // Writes input and output data within the ensemble to a Python module stored
  // in the file with the given name.
  void write(const std::string& module_filename) const {
    auto result = sw_ensemble_write(ensemble_, module_filename.c_str());
    if (result.error_code != SW_SUCCESS) {
      throw Exception(result.error_message);
    }
  }

 private:
  Ensemble(sw_ensemble_t *e, sw_settings_t* s):
    ensemble_(e), settings_(Settings(s)) {}

  sw_ensemble_t *ensemble_;
  Settings settings_;

  friend Ensemble* load_ensemble(const std::string& yaml_file,
                                 const std::string& settings_block);
};

// Loads an ensemble from the given YAML file, using the block with the given
// name to load settings.
inline Ensemble* load_ensemble(const std::string& yaml_file,
                               const std::string& settings_block = "") {
  auto result = sw_load_ensemble(yaml_file.c_str(), settings_block.c_str());
  if (result.error_code == SW_SUCCESS) {
    return new Ensemble(result.ensemble, result.settings);
  } else {
    throw Exception(result.error_message);
  }
}

} // namespace skywalker

#endif
