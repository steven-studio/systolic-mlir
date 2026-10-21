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
// (the two bounds shifted by different forms: decided by enumerating the
// points as emitted -- a parallelotope with the bounds' shift rows is the
// GEMM of its box, which the compiler's exact path must lower and the oracle
// checks address by address; any other triangle is outside the normalized
// family and never lowered), BOXES IN DISGUISE (section 19.13, since the same
// day: a box or sheared nest emitted with an extra max / min bound result of
// another linear part that never wins on the domain, or an outer loop's
// range extended by values for which an inner loop is empty -- the same
// points, no parallelotope as written; asserted point for point), ONE MEMREF AS BOTH FACTORS through two different views
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
// Since 2026-10-21 (docs section 23, reduction-batch partitioning on by
// default) the oracle models the reduction batch loops too: loops C does not
// read that are read by one factor only become RB loops (T5's virtual K = 1),
// and when the loops both factors read are not one dense K, each in turn is K
// (the longest first, ties the innermost) and the others RB; the first
// decomposition that is a GEMM by enumeration is the verdict, else the
// recognition of sections 19-21. A lowered RB nest is checked address by
// address per RB value (every iteration once, each value's factor views, the
// one slab of C they accumulate into), and C is read and written back inside
// every batch loop (the serial accumulation). Strict FP lowers none of them.
// Reduction batches are made only on the counters as written: a nest whose
// domain normalization rewrites its counters (x = U x', U != I: a sheared
// nest, a disguise with a nonzero shift row) keeps the decision of sections
// 19-21. The nests the default newly lowers on seeds 1-3 (9, 10 and 6) are
// ones the section 21 oracle had refused as broadcasts (a loop read by one
// factor only) or partial-C reductions (K read through two combinations of
// its loops, or not one dense coordinate).
//
// Since 2026-10-21 (docs section 25, ordered output batches) the oracle
// models the accumulations whose C is not injective on a tile's (I, J) box:
// on a box with the counters as written, an output role -- the product's
// first factor's first -- becomes batch loops when those loops and the other
// batch loops are an outermost prefix of the nest's loops of two or more
// iterations, and a demoted loop may then share a dimension of C with a row
// or column coordinate (not when it moves that role's coordinate in the
// role's factor through another linear form, SQ-1). Every lowered nest is
// also checked contribution by contribution from the compiler's views: no
// tile writes an element of C twice, every element of C receives exactly the
// program's products, and the products of different output batch values
// arrive in the program's order. The nests the section 21-23 oracle had
// refused that the compiler now lowers on seeds 1-3 (2, 2 and 1) are merged-C
// accumulations (C's two roles in one dimension, not injective) whose role I
// or J is the outermost loop(s).
//
// The oracle runs the systolic-opt of this build tree (lit substitutes the
// tool's path), not whichever is first on PATH.
//
// With the reassociation permission (allow-fp-reassociation=true):
// RUN: %python %S/Inputs/affine_gemm_mapping_oracle.py --opt systolic-opt --seed 1 --count 400 | FileCheck %s
// RUN: %python %S/Inputs/affine_gemm_mapping_oracle.py --opt systolic-opt --seed 2 --count 400 > %t.2
// RUN: FileCheck %s --input-file=%t.2
// RUN: FileCheck %s --check-prefix=PI --input-file=%t.2
// RUN: FileCheck %s --check-prefix=RBK --input-file=%t.2
// RUN: %python %S/Inputs/affine_gemm_mapping_oracle.py --opt systolic-opt --seed 3 --count 400 > %t.3
// RUN: FileCheck %s --input-file=%t.3
// RUN: FileCheck %s --check-prefix=PI --input-file=%t.3
//
// Seeds 2 and 3 hold PROVEN_INVALID triangles (seed 1 none): the disproof must fire there,
// and each of its witnesses is checked (review of 2026-10-19).
// PI:        refused triangles by class: PROVEN_INVALID {{[1-9][0-9]*}} (
//
// Seed 2 holds nests whose K is chosen among the loops both factors read (seed 3 none).
// RBK:       reduction-batch partitioning: lowered {{[1-9][0-9]*}} (with a loop read by one factor only {{[0-9]+}}, K chosen among the loops both factors read {{[1-9][0-9]*}},
//
// CHECK-NOT: MISMATCH
// CHECK:     oracle: 400 nests, accept {{[1-9][0-9]+}}, reject {{[1-9][0-9]+}}, skipped {{[0-9]+}}, refused by rule {{[1-9][0-9]*}} (triangles {{[1-9][0-9]*}}, C aliasing a factor {{[1-9][0-9]*}})
// CHECK:     accepted with: transposed view {{[1-9][0-9]*}}, negative stride {{[1-9][0-9]*}}, M/N/K = 1 {{[1-9][0-9]*}}, an operand of rank != 2 {{[1-9][0-9]*}}, a loop that runs once {{[1-9][0-9]*}}, batched {{[1-9][0-9]*}} (a factor's view moving with the batch {{[1-9][0-9]*}}), A and B both transposed {{[1-9][0-9]*}}, a max/min loop bound {{[1-9][0-9]*}} (of a batch loop {{[1-9][0-9]*}}), mutations role_in_k {{[1-9][0-9]*}} and batch_once {{[1-9][0-9]*}}, a sheared domain {{[1-9][0-9]*}} (chained {{[1-9][0-9]*}}, with a step > 1 {{[1-9][0-9]*}}, with a max/min bound {{[1-9][0-9]*}}), one array as both factors {{[1-9][0-9]*}} (through two different views {{[1-9][0-9]*}}, through one view {{[1-9][0-9]*}}, served by one tensor {{[1-9][0-9]*}}), K = 1 {{[1-9][0-9]*}} (without a reduction loop {{[1-9][0-9]*}}), a box in disguise {{[1-9][0-9]*}}, a degenerate triangle {{[1-9][0-9]*}}
// CHECK:     generalized mapping: batched with C reading a batch loop in a role dimension {{[1-9][0-9]*}}, batched with two unit extents {{[1-9][0-9]*}}, skewed or merged views (lowered) {{[1-9][0-9]*}}, C not injective (refused) {{[0-9]+}}, domain outside the normalized family (refused) {{[1-9][0-9]*}}, C the base of a factor (refused) {{[1-9][0-9]*}}, out of bounds after a shear (rejected) {{[1-9][0-9]*}}, C not injective after a shear (refused) {{[0-9]+}}
// CHECK:     reduction-batch partitioning: lowered {{[1-9][0-9]*}} (with a loop read by one factor only {{[1-9][0-9]*}}, K chosen among the loops both factors read {{[0-9]+}}, virtual K = 1 {{[1-9][0-9]*}}, with an output batch too {{[0-9]+}}, on a sheared or disguised domain {{[0-9]+}})
// CHECK:     ordered output batches: lowered {{[1-9][0-9]*}} (an output role made batch loops {{[1-9][0-9]*}}, a demoted loop sharing a dimension of C with a row or column coordinate {{[0-9]+}}, with reduction batch loops too {{[0-9]+}}, on a triangle or disguised domain {{[0-9]+}}); lowered with batch iterations writing overlapping elements of C {{[1-9][0-9]*}} (an outermost prefix {{[1-9][0-9]*}}), each checked contribution by contribution
// CHECK:     refused triangles by class: PROVEN_INVALID {{[0-9]+}} (each checked: not centrally symmetric with the stated witness, the stacked access map of full rank, three memrefs), UNKNOWN {{[1-9][0-9]*}}, VALID_BUT_LOWERING_UNSUPPORTED {{[0-9]+}}, PROVEN_VALID {{[0-9]+}}, not recognized {{[0-9]+}}
// CHECK:     mismatches: 0
//
// Without it (the default, docs/AFFINE_GEMM_MAPPING.md section 6) nothing is
// lowered: the systolic execution is a different order of IEEE additions (the
// ordered output batches of section 25 reassociate inside each tile). The
// triangles and the aliasing nests are not lowered (strict mode) here too.
// RUN: %python %S/Inputs/affine_gemm_mapping_oracle.py --opt systolic-opt --seed 1 --count 400 --fp-default \
// RUN:   | FileCheck %s --check-prefix=STRICT
//
// STRICT-NOT: MISMATCH
// STRICT:     oracle: 400 nests, accept {{[1-9][0-9]+}}, reject {{[1-9][0-9]+}}, skipped {{[0-9]+}}, refused by rule {{[1-9][0-9]*}} (triangles {{[1-9][0-9]*}}, C aliasing a factor {{[1-9][0-9]*}}) (pass run without the reassociation permission)
// STRICT:     not lowered (strict mode): {{[1-9][0-9]*}} outside the normalized family, {{[1-9][0-9]*}} with C the base of a factor, {{[1-9][0-9]*}} with reduction batch loops, {{[1-9][0-9]*}} with ordered output batches
// STRICT:     refused triangles by class: PROVEN_INVALID {{[0-9]+}} (each checked: not centrally symmetric with the stated witness, the stacked access map of full rank, three memrefs), UNKNOWN {{[1-9][0-9]*}}, VALID_BUT_LOWERING_UNSUPPORTED {{[0-9]+}}, PROVEN_VALID {{[0-9]+}}, not recognized {{[0-9]+}}
// STRICT:     mismatches: 0
//
// With reduction-batches=false (the recognition of sections 19-21 alone) no
// reduction batch loop is made, and the oracle decides without them; the
// ordered output batches of section 25 do not depend on that option.
// RUN: %python %S/Inputs/affine_gemm_mapping_oracle.py --opt systolic-opt --seed 1 --count 400 --no-reduction-batches \
// RUN:   | FileCheck %s --check-prefix=NORB
//
// NORB-NOT: MISMATCH
// NORB:     oracle: 400 nests, accept {{[1-9][0-9]+}}, reject {{[1-9][0-9]+}}, skipped {{[0-9]+}}, refused by rule {{[1-9][0-9]*}} (triangles {{[1-9][0-9]*}}, C aliasing a factor {{[1-9][0-9]*}})
// NORB:     reduction-batch partitioning (off): lowered 0 (
// NORB:     ordered output batches: lowered {{[1-9][0-9]*}} (an output role made batch loops {{[1-9][0-9]*}},
// NORB:     mismatches: 0
