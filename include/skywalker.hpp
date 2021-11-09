#ifndef SKYWALKER_HPP
#define SKYWALKER_HPP

#include <skywalker.h>

#include <exception>
#include <string>

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
void print_banner() {
  sw_print_banner();
}

// Precision of real numbers
using Real = sw_real_t;

// Ensemble type (SW_LATTICE or SW_ENUMERATION).
using EnsembleType = sw_ens_type_t;

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

 private:
  Input(sw_input_t *i): input_(i) {}
  sw_input_t *input_;
  friend class Ensemble;
};

// A set of named, real-valued output parameters corresponding to a single
// ensemble member.
class Output final {
 public:
  // Not directly constructible.
  Output() = default;
  Output(const Output& rhs): output_(rhs.output_) {}
  Output& operator=(const Output& rhs) {
    if (&rhs != this) {
      output_ = rhs.output_;
    }
    return *this;
  }

  ~Output() = default;

  // Sets a (real-valued) parameter with the given name, throwing an
  // exception if not successful.
  void set(const std::string& name, Real value) const {
    auto result = sw_output_set(output_, name.c_str(), value);
    if (result.error_code != SW_SUCCESS) {
      throw Exception(result.error_message);
    }
  }

 private:
  Output(sw_output_t *o): output_(o) {}
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

  // Returns the type of the ensemble (SW_LATTICE or SW_ENUMERATION).
  EnsembleType type() const { return type_; }

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
    sw_ensemble_write(ensemble_, module_filename.c_str());
  }

 private:
  explicit Ensemble(sw_ens_type_t t, sw_ensemble_t *e, sw_settings_t* s):
    type_(t), ensemble_(e), settings_(Settings(s)) {}

  sw_ens_type_t type_;
  sw_ensemble_t *ensemble_;
  Settings settings_;

  friend Ensemble* load_ensemble(const std::string& yaml_file,
                                 const std::string& settings_block);
};

// Loads an ensemble from the given YAML file, using the block with the given
// name to load settings.
inline Ensemble* load_ensemble(const std::string& yaml_file,
                               const std::string& settings_block) {
  auto result = sw_load_ensemble(yaml_file.c_str(), settings_block.c_str());
  if (result.error_code == SW_SUCCESS) {
    return new Ensemble(result.type, result.ensemble, result.settings);
  } else {
    throw Exception(result.error_message);
  }
}

} // namespace skywalker

#endif
