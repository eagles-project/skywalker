# Skywalker

Skywalker is a software library that allows you to construct and execute a
program on an **ensemble**: a collection of sets of input parameters, each of
which completely defines the behavior of the program. Each complete set of
input parameters is a **member** of the ensemble, and your program can associate
a set of outputs with each ensemble member.

A Skywalker program accepts as input a [YAML file](input.md) that defines an
ensemble. It then executes logic on each member of that ensemble, generating
output. It collects this output, associating it with the input that produced it,
and then writes all data for the entire ensemble into a [Python module](output.md)
in a way that allows postprocessing to be easily performed.

You can write a Skywalker program in C, C++, or Fortran. The [API](api.md) is
very similar in each of these languages, and you can study [examples](examples.md)
to see how to solve your own problems with a Skywalker program.

Skywalker was funded by the Office of Science's [Biological and Environmental
Research](https://science.osti.gov/ber) Program as part of the
[EAGLES project](https://climatemodeling.science.energy.gov/projects/enabling-aerosol-cloud-interactions-global-convection-permitting-scales-eagles),
an effort to improve the treatment of aerosols in
[E3SM](https://climatemodeling.science.energy.gov/projects/energy-exascale-earth-system-model),
the Department of Energy's global climate model. The source code is available on
[GitHub](https://github.com/eagles-project/skywalker).
