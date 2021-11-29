# Skywalker

Skywalker is a software library that allows you to construct and execute a
program on an **ensemble**: a collection of sets of input parameters, each of
which completely defines the behavior of the program. Each complete set of
input parameters is a **member** of the ensemble, and your program can associate
a set of outputs with each ensemble member.

A Skywalker program accepts as input a YAML file that defines an ensemble, and
executes logic on each member of that ensemble, generating output. It collects
this output, associating it with the input that produced it, and then writes
output for the entire ensemble into a Python module in a way that allows
postprocessing to be easily performed for the ensemble.

You can write a Skywalker program in C, C++, or Fortran. The [API](api.md) is
very similar in each of these languages.

