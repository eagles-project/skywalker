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

#include <skywalker.hpp>

#include <iostream>

void usage() {
  fprintf(stderr, "isotherms_cpp: calculates the pressure of a Van der Waals "
                  "gas given its volume and temperature.\n");
  fprintf(stderr, "isotherms_cpp: usage:\n");
  fprintf(stderr, "isotherms_cpp <input.yaml>\n");
  exit(-1);
}

// Determines the output_file name corresponding to the given name of the
// input file.
std::string output_file_name(const std::string &input_file) {
  size_t dot_index = input_file.find(".");
  if (dot_index < std::string::npos) { // found "."
    return input_file.substr(0, dot_index) + "_cpp.py";
  } else {
    return input_file + "_cpp.py";
  }
}

int main(int argc, char **argv) {

  if (argc == 1) {
    usage();
  }
  std::string input_file = argv[1];

  // Load the ensemble. Any error encountered is fatal.
  std::cout << "isotherms_cpp: Loading ensemble from " << input_file << "...\n";
  skywalker::Ensemble *ensemble = nullptr;
  try {
    ensemble = skywalker::load_ensemble(input_file);
  } catch (skywalker::Exception &e) {
    std::cerr << "isotherms_cpp: " << e.what() << std::endl;
    exit(-1);
  }
  std::cout << "isotherms_cpp: found " << ensemble->size() << " ensemble members.\n";

  // Iterate over all members of the ensemble.
  try {
    ensemble->process([](const skywalker::Input &input,
                         skywalker::Output &output) {
      // Fetch inputs.
      skywalker::Real V = input.get("V"); // gas (molar) volume [m3]
      skywalker::Real T = input.get("T"); // gas temperature [K]

      // Fetch Van der Waals parameters if they're present.
      skywalker::Real a = 0.0, b = 0.0;
      if (input.has("a")) {
        a = input.get("a");
      }
      if (input.has("b")) {
        b = input.get("b");
      }

      // Compute p(V, T).
      static const skywalker::Real R = 8.31446261815324;
      skywalker::Real p = R * T / (V - b) - a/(V*V);

      // Stash the computed pressure in the member's output.
      output.set("p", p);
    });
  } catch (skywalker::Exception &e) {
    std::cerr << "isotherms_cpp: " << e.what() << "\n";
    exit(-1);
  }

  // Write out a Python module.
  std::string output_file = output_file_name(input_file);
  std::cout << "isotherms_cpp: Writing data to " << output_file << "...\n";
  try {
    ensemble->write(output_file);
  } catch (skywalker::Exception &e) {
    std::cerr << "isotherms_cpp: " << e.what() << "\n";
    exit(-1);
  }

  // Clean up.
  delete ensemble;
}
