// Stage 2 of docs/GENERALIZED_GEMM_SCOPE.md: records that must not let a lowering go ahead.
// The input is nest_records.mlir, whose nests all lower with Inputs/nest_records.json.
// Each case below reads that file with one field edited (Inputs/edit_nest_records.py), or a
// record the research tool wrote of another input or with other options
// (research/generalized-gemm/test/fuzz/nest_gate_records.py writes those): every refusal
// names the nest and says what does not hold, and the nest stays as it is.
//
// A record of an older version of @single, before its accumulation was marked reassoc.
// RUN: systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_stale.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=STALE
// STALE: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of single: the record's binding is not this nest's: the record is about another version of the nest, or another nest
// STALE: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of strict: no record of function strict
// The nest stays where it was; nothing else in the function changes.
// RUN: systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_stale.json" | FileCheck %s --check-prefix=KEPT
// KEPT-LABEL: func.func @single
// KEPT-NOT: systolic.matmul_tile
// KEPT: affine.for
// KEPT: arith.addf %{{.*}}, %{{.*}} fastmath<reassoc> : f32
// KEPT-LABEL: func.func @strict
//
// The record of @single made with no pair of roots authorized: A and B may overlap C, so the
// research tool verifies nothing. Without records the lowering takes them apart by C's
// llvm.noalias and lowers the nest; with this record it does not.
// RUN: systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_unauthorized.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=UNAUTHORIZED
// UNAUTHORIZED: nest record: nest 0 of single: the record's status is NOT_VERIFIED: the ordered fold is not verified
// The record of @single made at a 16-bit index, for a program whose index has 64 bits.
// RUN: systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_width16.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=WIDTH
// WIDTH: nest record: nest 0 of single: the record's claims are about index values of 16 bits, and this program's index has 64
//
// The second nest of @shared has no record: it stays, and the first one is lowered.
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records.json missing-nest > %t.missing-nest.json
// RUN: systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.missing-nest.json report=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=NONEST
// NONEST: remark: nest record agrees: nest 0 of shared
// NONEST-NEXT: affine.for
// NONEST: remark: affine GEMM 16x8x12 lowered as 5 exact region(s)
// NONEST: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 1 of shared: the function's records have no record of this nest
//
// One field of @single's record edited.
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records.json extent > %t.extent.json
// RUN: systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.extent.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=EXTENT
// EXTENT: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of single: the record's M is 15 and the lowering's is 16
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records.json level > %t.level.json
// RUN: systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.level.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=LEVEL
// LEVEL: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of single: level 0 of the record is not loop 0 of the GEMM: axis, direction or bounds differ
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records.json axes > %t.axes.json
// RUN: systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.axes.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=AXES
// AXES: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of single: the record's coordinate I is not on loop 0, where the lowering has it
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records.json view > %t.view.json
// RUN: systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.view.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=VIEW
// VIEW: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of single: dimension 0 of the record's view of A is not the lowering's: 1 * loop 0 + 0
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records.json root > %t.root.json
// RUN: systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.root.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=ROOT
// ROOT: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of single: the record's A is root 1 and the lowering's is root 0
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records.json flags > %t.flags.json
// RUN: systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.flags.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=FLAGS
// FLAGS: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of single: the record's flags (addition: none; multiplication: none) are not the GEMM's (reassoc; none)
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records.json flag-number > %t.flag-number.json
// RUN: systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.flag-number.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=FLAGNUMBER
// FLAGNUMBER: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of single: the record is incomplete: a flag is no string
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records.json source-flag > %t.source-flag.json
// RUN: systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.source-flag.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=SOURCEFLAG
// SOURCEFLAG: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of single: the record's `reassociation.source_flag_reassoc` is not the GEMM's flag
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records.json accumulator > %t.accumulator.json
// RUN: systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.accumulator.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=ACCUMULATOR
// ACCUMULATOR: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of single: the record's addition takes the accumulator second and the GEMM's takes it first
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records.json element > %t.element.json
// RUN: systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.element.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=ELEMENT
// ELEMENT: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of single: the record's element type is f64 and the GEMM's is f32
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records.json incomplete > %t.incomplete.json
// RUN: systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.incomplete.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=INCOMPLETE
// INCOMPLETE: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of single: the record is incomplete: `gemm.views` is no object
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records.json authorizes > %t.authorizes.json
// RUN: systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.authorizes.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=AUTHORIZES
// AUTHORIZES: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of single: the record claims to authorize a lowering, and no record this pass reads does
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records.json status > %t.status.json
// RUN: systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.status.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=STATUS
// STATUS: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of single: the record's status is VERIFIED_NOT_MAPPED: the view of A has no proposed closed form
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records.json scope > %t.scope.json
// RUN: systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.scope.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=SCOPE
// SCOPE: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of single: the record's claims hold for a range of arguments only, and the lowering is for every value
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records.json separation > %t.separation.json
// RUN: systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.separation.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=SEPARATION
// SEPARATION: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of single: the record assumes roots 0 and 1 apart, and the lowering checks only A and B against C
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records.json proof > %t.proof.json
// RUN: systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.proof.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=PROOF
// PROOF: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of single: the record proves two roots apart by an allocation, and both are arguments here
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records.json theorem13 > %t.theorem13.json
// RUN: systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.theorem13.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=THEOREM
// THEOREM: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of single: the record's Theorem 13 is VERIFIED at SCOPE_ENUMERATED, logical tier SYMBOLIC, and a record is read only when all three are VERIFIED at SYMBOLIC
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records.json frame > %t.frame.json
// RUN: systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.frame.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=FRAME
// FRAME: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of single: the record says the nest writes memory outside the multiply-accumulate
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records.json roots > %t.roots.json
// RUN: systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.roots.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=ROOTS
// ROOTS: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of single: root 2 of the record's function is not root 2 of single: the record is about another version of the function
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records.json missing-function > %t.missing-function.json
// RUN: systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.missing-function.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=NOFUNCTION
// NOFUNCTION: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of single: no record of function single
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records.json scheme > %t.scheme.json
// RUN: systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.scheme.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=SCHEME
// SCHEME: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of single: the record's binding is of the scheme another-binding/1, and this pass computes generalized-gemm-nest-binding/1
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records.json fold > %t.fold.json
// RUN: systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.fold.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=FOLD
// FOLD: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of single: the record does not verify the ordered fold
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records.json refusals > %t.refusals.json
// RUN: systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.refusals.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=REFUSALS
// REFUSALS: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of single: the record has refusals
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records.json box > %t.box.json
// RUN: systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.box.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=BOX
// BOX: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of single: the record's domain is no box
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records.json extra-level > %t.extra-level.json
// RUN: systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.extra-level.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=EXTRALEVEL
// EXTRALEVEL: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of single: the record has 4 levels, and the GEMM's nest 3 loops
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records.json unproved > %t.unproved.json
// RUN: systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.unproved.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=UNPROVED
// UNPROVED: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of single: the record does not prove the view of A
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records.json extra-dimension > %t.extra-dimension.json
// RUN: systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.extra-dimension.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=EXTRADIMENSION
// EXTRADIMENSION: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of single: the record's view of A has 3 dimensions
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records.json reduction > %t.reduction.json
// RUN: systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.reduction.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=REDUCTION
// REDUCTION: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of single: the record's update is not one site of fadd over fmul
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records.json kind > %t.kind.json
// RUN: systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.kind.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=KIND
// KIND: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of single: the record rests on a separation it does not know (witness)
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records.json extra-root > %t.extra-root.json
// RUN: systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.extra-root.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=EXTRAROOT
// EXTRAROOT: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of single: the record's function has 4 roots and single has 3: the record is about another version of the function
//
// What is no file of records is an error, and so is a path that reads no records.
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records.json schema > %t.schema.json
// RUN: not systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.schema.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=SCHEMA
// SCHEMA: error: affine-matmul-to-systolic: nest-records: {{.*}}: a function object is not of the format generalized-gemm-nest/2 (it is generalized-gemm-nest/1)
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records.json duplicate > %t.duplicate.json
// RUN: not systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.duplicate.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=DUPLICATE
// DUPLICATE: error: affine-matmul-to-systolic: nest-records: {{.*}}: two function objects for single
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records.json nonumber > %t.nonumber.json
// RUN: not systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.nonumber.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=NONUMBER
// NONUMBER: error: affine-matmul-to-systolic: nest-records: {{.*}}: a nest of single has no number
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records.json twice > %t.twice.json
// RUN: not systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.twice.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=TWICE
// TWICE: error: affine-matmul-to-systolic: nest-records: {{.*}}: two records of nest 0 of single
// RUN: not systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.none.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=NOFILE
// NOFILE: error: affine-matmul-to-systolic: nest-records: cannot read {{.*}}none.json
// RUN: echo 'not json' > %t.garbage.json
// RUN: not systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.garbage.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=GARBAGE
// GARBAGE: error: affine-matmul-to-systolic: nest-records: {{.*}}garbage.json is no JSON
// RUN: not systolic-opt %S/nest_records.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records.json general-affine=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=PATHS
// PATHS: error: affine-matmul-to-systolic: nest-records is read on the dense path only; general-affine, kernel-coset-offload, certified-offload-first, common-fiber-offload and fp-distribute take other paths, which read no records
