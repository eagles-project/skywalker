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

// This program tests Skywalker's C++ interface and its support for array
// parameters.

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

using namespace skywalker;

int main(int argc, char **argv) {

  if (argc == 1) {
    usage((const char*)argv[0]);
  }
  std::string input_file = argv[1];

  // Print a banner with Skywalker's version info.
  print_banner();

  // Load the ensemble. Any error encountered is fatal.
  std::cerr << "array_param_test: Loading ensemble from " << input_file << std::endl;
  Ensemble* ensemble = load_ensemble(input_file, "settings");

  // Make sure everything is as it should be.

  // Ensemble data: 4 arrays each containing 4 values, and 4 scalars.
  // We produce identically structured output containing the squares of each input value.
  assert(ensemble->size() == 4);
  ensemble->process([](const Input& input, Output& output) {
    assert(input.has("scalars"));
    Real scalar = input.get("scalars");
    output.set("scalars_squared", scalar * scalar);

    assert(input.has_array("arrays"));
    auto array = input.get_array("arrays");
    assert(array.size() == 4);
    std::vector<Real> array_squared = {array[0]*array[0], array[1]*array[1],
                                       array[2]*array[2], array[3]*array[3]};
    output.set("arrays_squared", array_squared);
  });

  // Now traverse the ensemble again and check its inputs and outputs.
  ensemble->process([](const Input& input, Output& output) {
    std::string name;
    Real scalar_value;
    int num_scalars = 0;
    while (input.next_scalar(name, scalar_value)) {
      assert(name == "scalars");
      printf("scalars[%d] = %g\n", num_scalars, scalar_value);
      ++num_scalars;
    }
    assert(num_scalars == 1);

    num_scalars = 0;
    while (output.next_scalar(name, scalar_value)) {
      assert(name == "scalars_squared");
      printf("scalars_squared[%d] = %g\n", num_scalars, scalar_value);
      ++num_scalars;
    }
    assert(num_scalars == 1);

    int num_arrays = 0;
    std::vector<Real> array_values;
    while (input.next_array(name, array_values)) {
      assert(name == "arrays");
      printf("arrays[%d] = {%g, %g, %g, %g}\n", num_arrays, array_values[0],
             array_values[1], array_values[2], array_values[3]);
      ++num_arrays;
    }
    assert(num_arrays == 1);

    num_arrays = 0;
    while (output.next_array(name, array_values)) {
      assert(name == "arrays_squared");
      printf("arrays_squared[%d] = {%g, %g, %g, %g}\n", num_arrays, array_values[0],
             array_values[1], array_values[2], array_values[3]);
      ++num_arrays;
    }
    assert(num_arrays == 1);
  });

  // Clean up.
  delete ensemble;
}
