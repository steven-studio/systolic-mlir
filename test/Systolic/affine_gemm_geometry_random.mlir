// Randomized cross-check of the geometric screening (screenAffineMatmulGeometry,
// docs/AFFINE_GEMM_MAPPING.md section 19, layer 1) against an independent Python
// implementation (Inputs/affine_geometry_check.py): on the GEMM mapping oracle's random
// nests (ranks 1..3, transposes, negative coefficients, split coordinates, loops that
// run once, batches, redundant max/min bounds, perturbations) every printed reader,
// class, ordinal row, rank, kernel vector (membership, Hermite normal form, completeness
// on a small box), joining direction and six-vector condition is recomputed.
//
// RUN: %python %S/Inputs/affine_geometry_check.py --opt systolic-opt --seed 1 --count 300 | FileCheck %s
// RUN: %python %S/Inputs/affine_geometry_check.py --opt systolic-opt --seed 2 --count 300 | FileCheck %s
//
// CHECK-NOT: FAIL
// CHECK:     geometry check: 300 nests, {{[0-9]+}} screened, failures 0
