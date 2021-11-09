// This program tests Skywalker's C++ interface with a lattice ensemble.

#include <skywalker.hpp>

#include <cassert>
#include <iostream>
#include <string.h>
#include <tgmath.h>

void usage(const std::string& prog_name) {
  std::cerr << prog_name << ": usage:" << std::endl;
  std::cerr << prog_name << " <input.yaml>" << std::endl;
  exit(0);
}

static bool approx_equal(sw_real_t x, sw_real_t y) {
  return (fabs(x - y) < 1e-14);
}

using namespace skywalker;

int main(int argc, char **argv) {

  if (argc == 1) {
    usage((const char*)argv[0]);
  }
  std::string input_file = argv[1];

  // Print a banner with Skywalker's version info.
  print_banner();

  // Load the ensemble. Any error encountered is fatal.
  std::cerr << "lattice_test: Loading ensemble from " << input_file << std::endl;
  Ensemble* ensemble = load_ensemble(input_file, "settings");

  // Make sure everything is as it should be.

  // Ensemble type
  assert(ensemble->type() == SW_LATTICE);

  // Settings
  Settings settings = ensemble->settings();
  assert(settings.get("param1") == "hello");
  assert(settings.get("param2") == "81");
  assert(settings.get("param3") == "3.14159265357");

  // Ensemble data
  assert(ensemble->size() == 245520);
  ensemble->process([](const Input& input, Output& output) {
    // Fixed parameters
    assert(approx_equal(input.get("p1"), 1.0));
    assert(approx_equal(input.get("p2"), 2.0));
    assert(approx_equal(input.get("p3"), 3.0));

    // Ensemble parameters
    assert(input.get("tick") >= 0.0);
    assert(input.get("tick") <= 10.0);

    assert(input.get("tock") >= 1e1);
    assert(input.get("tock") <= 1e11);

    assert(input.get("pair") >= 1.0);
    assert(input.get("pair") <= 2.0);

    assert(input.get("triple") >= 1.0);
    assert(input.get("triple") <= 3.0);

    assert(input.get("quartet") >= 1.0);
    assert(input.get("quartet") <= 4.0);

    assert(input.get("quintet") >= 1.0);
    assert(input.get("quintet") <= 5.0);

    assert(input.get("sextet") >= 1.0);
    assert(input.get("sextet") <= 6.0);

    // Look for a parameter that doesn't exist.
    bool caught = false;
    try {
      input.get("invalid_param");
    }
    catch (Exception&) {
      caught = true;
    }
    assert(caught);

    // Add a "qoi" metric set to 4.
    output.set("qoi", 4.0);
  });

  // Write out a Python module.
  ensemble->write("lattice_test_cpp.py");

  // Clean up.
  delete ensemble;
}
