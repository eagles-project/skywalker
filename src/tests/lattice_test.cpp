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

// This program tests Skywalker's C++ interface with a lattice ensemble.

#include <skywalker.hpp>

#include <cassert>
#include <iostream>
#include <cstring>
#include <cmath>

void usage(const std::string& prog_name) {
  std::cerr << prog_name << ": usage:" << std::endl;
  std::cerr << prog_name << " <input.yaml>" << std::endl;
  exit(0);
}

static bool approx_equal(sw_real_t x, sw_real_t y) {
  return (std::abs(x - y) < 1e-14);
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

  // Settings
  Settings settings = ensemble->settings();
  assert(settings.has("setting1"));
  assert(settings.get("setting1") == "hello");
  assert(settings.has("setting2"));
  assert(settings.get("setting2") == "81");
  assert(settings.has("setting3"));
  assert(settings.get("setting3") == "3.14159265357");

  assert(not settings.has("nonexistent_param"));

  // Ensemble data
  assert(ensemble->size() == 245520);
  ensemble->process([](const Input& input, Output& output) {
    // Fixed parameters
    assert(input.has("p1"));
    assert(approx_equal(input.get("p1"), 1.0));

    assert(input.has("p2"));
    assert(approx_equal(input.get("p2"), 2.0));

    assert(input.has("p3"));
    assert(approx_equal(input.get("p3"), 3.0));

    // Ensemble parameters
    assert(input.has("tick"));
    assert(input.get("tick") >= 0.0);
    assert(input.get("tick") <= 10.0);

    assert(input.has("tock"));
    assert(input.get("tock") >= 1e1);
    assert(input.get("tock") <= 1e11);

    assert(input.has("pair"));
    assert(input.get("pair") >= 1.0);
    assert(input.get("pair") <= 2.0);

    assert(input.has("triple"));
    assert(input.get("triple") >= 1.0);
    assert(input.get("triple") <= 3.0);

    assert(input.has("quartet"));
    assert(input.get("quartet") >= 1.0);
    assert(input.get("quartet") <= 4.0);

    assert(input.has("quintet"));
    assert(input.get("quintet") >= 1.0);
    assert(input.get("quintet") <= 5.0);

    assert(input.has("sextet"));
    assert(input.get("sextet") >= 1.0);
    assert(input.get("sextet") <= 6.0);

    // Look for a parameter that doesn't exist.
    assert(not input.has("invalid_param"));
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
