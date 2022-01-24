# Input Format (YAML)

A Skywalker program accepts a [YAML](https://yaml.org/spec/1.2.2/) file as
input. YAML is a simple and expressive language for describing data in a
portable format. It offers all of the features of Fortran namelists and more,
and it can be used in programs written in any language.

Skywalker extracts information from specific variables in your YAML input file.
For simplicity, we refer to variables at the top level of a YAML file as
**blocks**, because they resemble block sections in other file formats. Here's
an example of a YAML file containing blocks `a`, `b`, `c`, and `d`:

```

a: This block is just a string

b:
  description: this block is a set of keys and values
  key1: value1        # (like this!)
  key2: 112           # (and this!)
  key3: 3.14159265357 # (and this!)

c: [1, 2, 3, 4, 5] # this block is an array of integers

d: # this block is the ѕame array as c, expressed differently
  - 1
  - 2
  - 3
  - 4
  - 5
```

As you can see above, lists of values are indicated by comma-separated values
surrounded by braces, or a sequence of values prepended by hyphens. Notice also
that YAML ignores anything starting with a `#` character, which allows you to
annotate YAML files with comments.

### Skywalker YAML Input

Skywalker looks for the following information in an input file, each of which
belongs in its own block:

* an **ensemble type** block that defines the method used to construct an
  ensemble
* an optional **settings** block that conveys information from the user to a
  Skywalker program
* an **input** block that defines the members of the ensemble using the method
  selected in the **ensemble type** block.

Here's an example of a Skywalker input file used to test an aerosol
parameterization that appears in two different scientific codes:

```
# Computes the rate of binary nucleation as a function of relative humidity [-]
# and temperature [K]

type: lattice

driver1: # settings for driver1
  nucleation_method: 2
  pbl_method: 0

driver2: # settings for driver2
  name: mer07_veh02_wang08_nuc_1box
  newnuc_method_user_choice: 2
  pbl_nuc_wang2008_user_choice: 0

input:
  relative_humidity: [0.01, 1.00, 0.01]
  temperature: [230.15, 300.15, 1]
  c_h2so4: 5e8 # [#/cc]
  planetary_boundary_layer_height: 1100
  height: 500
  xi_nh3: 0
  current_gas_mix_ratios: [[0.023, 0.033]]
  wet_geo_mean_diameter: [[0.001, 0.002, 0.003],[0.004, 0.005, 0.006], [0.001, 0.001, 0.001]]
```

Below, we describe the structure of each of these blocks, and give examples.
Skywalker parses the input YAML file for you, so there's no need to worry about
types, comments, or any of the details of the YAML format.

These blocks can appear in any order within a YAML file.

## Ensemble Type

Skywalker looks for a block named `type` to decide how to build an ensemble. Two
ensemble "types" are supported:

1. `lattice`: A **lattice ensemble** is constructed by combining every parameter
   values with all other parameter values. For example, a lattice of 3
   parameters each assuming 10 values creates an ensemble of $10^3 = 1000$
   members. The order in which parameter values are specified is not important
   for lattice ensembles.
2. `enumeration`: An **enumeration ensemble** is constructed by traversing all
   ensemble parameter values in lockstep. The first ensemble member assumes the
   first specified value of each parameter, the second member assumes the second
   value of each, and so on. In an enumeration ensemble, each parameter must
   have the same number of values (or assume only a single value). For example,
   in an enumeration ensemble consisting of 1000 members, every parameter must
   either assume 1000 values or a single value.

In either case, any parameter that assumes only one value will have that value
in every member of the ensemble.

## Settings

A Skywalker program may need information about how to configure itself, in
addition to the information about the ensembles it constructs. If it does, it
can specify the name of a block to search for "settings". For example, the
Skywalker input file in the first section of this page has two settings blocks:

* `driver1`, which is needed by one Skywalker program
* `driver2`, which is needed by a different Skywalker program

The ability to tell Skywalker where to look for settings allows you to run
several different programs with the same input, and compare their results. This
is an invaluable capability for cross validating scientific codes.

Notice the structure of the `driver1` and `driver2` settings blocks. They
consist of simple named variables, each with a single value. Skywalker settings
are always interpreted as strings, so if your program needs an integer or
floating point value to configure itself, it must convert the relevant setting
to the correct type.

Settings blocks are entirely optional and can be omitted if you don't need them.

## Input

Skywalker looks for parameter values in an `input` block. Each parameter has a
name and either a single value or a list of values.

Look at the `input` block in the above example. There are 6 parameters:
`relative_humidity`, `temperature`, `c_h2so4`,
`planetary_boundary_layer_height`, `height`, and `xi_nh3`. The first two
parameters assume more than one value, and the rest assume only a single value.

Note the parameter `current_gas_mix_ratios`.  This is an example of a parameter that
constists of an array of an array.  For some parameters there are multiple values
for a single ensemble.  This array of arrays syntax covers this case.  For the
`current_gas_mix_ratios`, an array of two values will be available for each ensemble
created.  For lattice and enumeration purposes it is treated as a one (abet array
valued) value the same as `planetary_boundary_layer_height`.

### Lists of Parameter Values and Uniform Spacing

There are a few ways to specify multiple values for a parameter. The simplest
way is to list the values exhaustively. For example, if we want to study the
behavior of our binary nucleation parameterization between temperatures of
230.15 K and 300.15 K, in increments of 1 K, we can write

```
input:
  ...
  temperature: [230.15, 231.15, 232.15, 233.15, ..., 298.15, 299.15, 300.15]
  ...
```

where we have replaced several terms with ellipsis for brevity. This is simple
but cumbersome, especially since we are using a uniform step size. A much easier
way to do the same thing is to use the form of the `temperature` parameter in
the above example, which uses a 3-value list to specify the same set of values:

```
input:
  ...
  temperature: [230.15, 300.15, 1]
  ...
```

This reads: "the `temperature` parameter assumes values from 230.15 to 300.15,
including the endpoints, in uniform steps of 1."

How does Skywalker know this isn't a list containing the temperatures 230.15 K,
300.15 K, and 1 K? It uses a set of simple rules to determine how to interpret
these values.

Lists that satisfy all of these properties are expanded into uniformly spaced
sets of values:

* The list contains 3 values
* The first value is less than the second value
* The third value is less than the second value

All other lists are interpreted as lists containing 3 values.

These rules may seem arbitrary, but in practice, they will never prevent you
from doing what you need to do.

  
### Logarithmic Spacing

Sometimes a parameter varies over several orders of magnitude, making it
inconvenient to specify uniformly-spaced values. For example, chemical reactions
can exhibit different behaviors over large ranges of number concentrations in
their reactants. For these situations, Skywalker offers an elegant solution that
allows you to define the values for a such a parameter on a logarithmic scale.
Simply "take the log of" the entry for your parameter:

```
input:
  ...
  log10(c_h2so4): [10, 12, 0.05]
  ...
```

The above list assigns values to the parameter `c_h2so4` between $10^{10}$ and
$10^{12}$, using $(12 - 10)/0.05 = 40$ uniformly-spaced points on a logarithmic
scale.

You can use this logarithmic option for parameters with explicitly listed values
as well, but it's most uѕeful when combined with the uniform spacing above.

### Single-Valued Parameters

To specify a parameter with a single value, you can use a scalar value for that
parameter, as is done above with the `c_h2so4`,
`planetary_boundary_layer_height`, `height`, and `xi_nh3` parameters. You can
also use a list that contains only 1 parameter:

```
input:
  ...
  c_h2so4: [5e8] # this does the same thing as the original example
  ...
```

### Array-Valued Parameters

Occasionally, it's useful to use a single parameter name for a collection of
values. For example, the concentrations in a miscible fluid can be considered
the components of a single variable, as long as their ordering is well defined.

Here, Skywalker takes advantage of the flexibility in the YAML format. Suppose
you have a variable `n` representing the various concentrations in a fluid, and
that there are 3 components. Here's how you would specify 4 different
configurations of the fluid's concentrations:

```
input:
  ...
  n: [[0.1, 0.3, 0.6], [0, 0.4, 0.4], [0.2, 0.8, 0], [0.5, 0.25, 0.25]]
  ...
```

We have used two sets of braces (a "list of lists") to indicate that the
parameter `n` assumes 4 values, each of which is a list of 3 numbers.

This is a very powerful syntax, but it comes with ѕome caveats:

* You can't use logarithmic spacing options with array parameters
* Skywalker makes no attempt to verify that all values in a list of array
  parameters have the same length
* Single values for array parameters must be surrounded by two sets of braces
  (e.g. `n: [[0.1, 0.3, 0.5]]`). Otherwise, Skywalker interprets the parameter
  as a list of multiple scalar parameters.

```
input:
  ...
  wet_geo_mean_diameter: [[0.001, 0.002, 0.003],[0.004, 0.005, 0.009], [0.001, 0.001, 0.002]]
  ...
```
When using lattice input, This reads that the `wet_geo_mean_diameter` parameter assumes 
an array value with uniform spacing for each ensemble.  
The array value start at [0.001, 0.002, 0.003] and goes to
[0.004, 0.005, 0.009] incrementing each individual array entry by [0.001, 0.001, 0.220]
 for each ensemble.  The interpretation of the values as an expanded list is the same as 
above for a scalar list:

* The list contains 3 sub-lists
* The each value in the first list is less than the corresponding value in the second list
* The each value in the third list is less than the corresponding value in the second list

All other lists are interpreted as lists containing 3 lists.

Use array-valued parameters with caution.

