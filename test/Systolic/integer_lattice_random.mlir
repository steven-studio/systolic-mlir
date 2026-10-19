// Randomized property test of the integer lattice tools against an independent
// Python implementation (exact fractions for ranks and determinants, brute-force
// enumeration for kernels, mixed radixes and injectivity):
// Inputs/integer_lattice_check.py. Every printed kernel must be a basis of the
// integer kernel in Hermite normal form, every injectivity verdict must agree
// with enumeration and every reported collision must be one.
//
// RUN: %python %S/Inputs/integer_lattice_check.py --opt systolic-opt --seed 1 --count 300 | FileCheck %s
// RUN: %python %S/Inputs/integer_lattice_check.py --opt systolic-opt --seed 2 --count 300 | FileCheck %s
// RUN: %python %S/Inputs/integer_lattice_check.py --opt systolic-opt --seed 3 --count 300 | FileCheck %s
//
// CHECK-NOT: FAIL
// CHECK:     lattice check: 300 matrices, 300 forms, 300 radixes, 300 injectivity cases, failures 0
