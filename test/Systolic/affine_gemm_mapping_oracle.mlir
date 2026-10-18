// The affine GEMM mapping against a brute-force oracle (docs/
// AFFINE_GEMM_MAPPING.md, proof obligation PO5): random nests -- operands of
// rank 1..3 with the GEMM roles in either order (transposes), subscripts that
// run forwards or backwards (negative-stride views), constant (extent-one)
// dimensions anywhere, roles of extent one (M, N or K = 1), offsets, strides,
// steps, origins, loops that count down, split coordinates and loops that run
// once adding arbitrary terms, loops with redundant max/min bounds (which the
// compiler must fold: a bound map's first result runs other iterations), and
// perturbations of them (a dimension skewed by another of the operand's roles, C's two
// role dimensions merged into one -- injective or not --, a loop of I or J added to both factors' K
// subscripts -- a batch loop the positional reading would have drive two
// coordinates --, a batch loop that runs once -- a fixed index --, other radices,
// coupling, broadcasts, a coordinate read twice, batches, accumulators at
// another element, out-of-bounds accesses) -- decided by enumerating every
// iteration point, without the structure theorem the compiler relies on.
//
// Since 2026-10-18 (section 19.12) the nests also cover: SHEARED DOMAINS --
// loops whose two bounds are shifted by one integer form of outer counters
// (k in [i, i + 4), coefficients up to 2, two outer counters, chained shifts,
// steps > 1, max/min-decorated bounds), the subscripts rewritten so that the
// sheared nest is the box nest's computation on a parallelotope; the oracle
// decides the box nest and checks the views on its points --, 'triangles'
// (the two bounds shifted by different forms: outside the normalized family,
// never lowered), ONE MEMREF AS BOTH FACTORS through two different views
// (A[i][k] * A[k][j] and its transposed, skewed, strided, reversed variants:
// two read-only tensors of one buffer, named by position in the tile and
// checked address by address) or through one view (M = N = K, the same
// element map: one tensor), C ALIASING A FACTOR'S MEMREF (refused: 'A may
// overlap C'), and K = 1 counted explicitly (a reduction loop that runs once,
// or none). Out-of-bounds accesses and non-injective C under a shear are
// rejected / refused like any other.
//
// The compiler's output is checked, not compared with a predicted form: the
// view of each operand that reaches the tile (base buffer, offset, signed
// strides, after subview / transpose / expand_shape / reinterpret_cast) must
// hold every access of the nest exactly once, the three views must agree on
// (I, J, K), those triples must be a bijection onto the box, and every view
// element must lie inside its allocation. Nests the oracle refuses must not
// be lowered.
//
// The oracle runs the systolic-opt of this build tree (lit substitutes the
// tool's path), not whichever is first on PATH.
//
// With the reassociation permission (allow-fp-reassociation=true):
// RUN: %python %S/Inputs/affine_gemm_mapping_oracle.py --opt systolic-opt --seed 1 --count 400 | FileCheck %s
// RUN: %python %S/Inputs/affine_gemm_mapping_oracle.py --opt systolic-opt --seed 2 --count 400 | FileCheck %s
// RUN: %python %S/Inputs/affine_gemm_mapping_oracle.py --opt systolic-opt --seed 3 --count 400 | FileCheck %s
//
// CHECK-NOT: MISMATCH
// CHECK:     oracle: 400 nests, accept {{[1-9][0-9]+}}, reject {{[1-9][0-9]+}}, skipped {{[0-9]+}}, refused by rule {{[1-9][0-9]*}} (triangles {{[1-9][0-9]*}}, C aliasing a factor {{[1-9][0-9]*}})
// CHECK:     accepted with: transposed view {{[1-9][0-9]*}}, negative stride {{[1-9][0-9]*}}, M/N/K = 1 {{[1-9][0-9]*}}, an operand of rank != 2 {{[1-9][0-9]*}}, a loop that runs once {{[1-9][0-9]*}}, batched {{[1-9][0-9]*}} (a factor's view moving with the batch {{[1-9][0-9]*}}), A and B both transposed {{[1-9][0-9]*}}, a max/min loop bound {{[1-9][0-9]*}} (of a batch loop {{[1-9][0-9]*}}), mutations role_in_k {{[1-9][0-9]*}} and batch_once {{[1-9][0-9]*}}, a sheared domain {{[1-9][0-9]*}} (chained {{[1-9][0-9]*}}, with a step > 1 {{[1-9][0-9]*}}, with a max/min bound {{[1-9][0-9]*}}), one array as both factors {{[1-9][0-9]*}} (through two different views {{[1-9][0-9]*}}, through one view {{[1-9][0-9]*}}, served by one tensor {{[1-9][0-9]*}}), K = 1 {{[1-9][0-9]*}} (without a reduction loop {{[1-9][0-9]*}})
// CHECK:     generalized mapping: batched with C reading a batch loop in a role dimension {{[1-9][0-9]*}}, batched with two unit extents {{[1-9][0-9]*}}, skewed or merged views (lowered) {{[1-9][0-9]*}}, C not injective (refused) {{[0-9]+}}, domain outside the normalized family (refused) {{[1-9][0-9]*}}, C the base of a factor (refused) {{[1-9][0-9]*}}, out of bounds after a shear (rejected) {{[1-9][0-9]*}}, C not injective after a shear (refused) {{[0-9]+}}
// CHECK:     mismatches: 0
//
// Without it (the default, docs/AFFINE_GEMM_MAPPING.md section 6) nothing is
// lowered: the systolic execution is a different order of IEEE additions. The
// triangles and the aliasing nests are not lowered (strict mode) here too.
// RUN: %python %S/Inputs/affine_gemm_mapping_oracle.py --opt systolic-opt --seed 1 --count 400 --fp-default \
// RUN:   | FileCheck %s --check-prefix=STRICT
//
// STRICT-NOT: MISMATCH
// STRICT:     oracle: 400 nests, accept {{[1-9][0-9]+}}, reject {{[1-9][0-9]+}}, skipped {{[0-9]+}}, refused by rule {{[1-9][0-9]*}} (triangles {{[1-9][0-9]*}}, C aliasing a factor {{[1-9][0-9]*}}) (pass run without the reassociation permission)
// STRICT:     not lowered (strict mode): {{[1-9][0-9]*}} outside the normalized family, {{[1-9][0-9]*}} with C the base of a factor
// STRICT:     mismatches: 0
