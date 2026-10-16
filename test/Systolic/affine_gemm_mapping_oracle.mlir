// The affine GEMM mapping against a brute-force oracle (docs/
// AFFINE_GEMM_MAPPING.md, proof obligation PO5): random nests -- operands of
// rank 1..3 with the GEMM roles in either order (transposes), subscripts that
// run forwards or backwards (negative-stride views), constant (extent-one)
// dimensions anywhere, roles of extent one (M, N or K = 1), offsets, strides,
// steps, origins, loops that count down, split coordinates and loops that run
// once adding arbitrary terms, and perturbations of them (other radices,
// coupling, broadcasts, a coordinate read twice, batches, accumulators at
// another element, out-of-bounds accesses) -- decided by enumerating every
// iteration point, without the structure theorem the compiler relies on.
//
// The compiler's output is checked, not compared with a predicted form: the
// view of each operand that reaches the tile (base buffer, offset, signed
// strides, after subview / transpose / expand_shape) must hold every access of
// the nest exactly once, the three views must agree on (I, J, K), those
// triples must be a bijection onto the box, and every view element must lie
// inside its allocation. Nests the oracle refuses must not be lowered.
//
// With the reassociation permission (allow-fp-reassociation=true):
// RUN: %python %S/Inputs/affine_gemm_mapping_oracle.py --seed 1 --count 400 | FileCheck %s
// RUN: %python %S/Inputs/affine_gemm_mapping_oracle.py --seed 2 --count 400 | FileCheck %s
// RUN: %python %S/Inputs/affine_gemm_mapping_oracle.py --seed 3 --count 400 | FileCheck %s
//
// CHECK-NOT: MISMATCH
// CHECK:     oracle: 400 nests, accept {{[1-9][0-9]+}}, reject {{[1-9][0-9]+}}, skipped {{[0-9]+}}
// CHECK:     accepted with: transposed view {{[1-9][0-9]*}}, negative stride {{[1-9][0-9]*}}, M/N/K = 1 {{[1-9][0-9]*}}, an operand of rank != 2 {{[1-9][0-9]*}}, a loop that runs once {{[1-9][0-9]*}}, batched {{[1-9][0-9]*}} (a factor's view moving with the batch {{[1-9][0-9]*}}), A and B both transposed {{[1-9][0-9]*}}
// CHECK:     mismatches: 0
//
// Without it (the default, docs/AFFINE_GEMM_MAPPING.md section 6) nothing is
// lowered: the systolic execution is a different order of IEEE additions.
// RUN: %python %S/Inputs/affine_gemm_mapping_oracle.py --seed 1 --count 400 --fp-default \
// RUN:   | FileCheck %s --check-prefix=STRICT
//
// STRICT-NOT: MISMATCH
// STRICT:     oracle: 400 nests, accept {{[1-9][0-9]+}}, reject {{[1-9][0-9]+}}, skipped {{[0-9]+}} (pass run without the reassociation permission)
// STRICT:     mismatches: 0
