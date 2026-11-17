// Stage 3 of docs/GENERALIZED_GEMM_SCOPE.md: what the lowering from records refuses, and
// says. The nests below are each one change away from a GEMM the lowering takes; each case is
// the research tool's own record of a nest (Inputs/nest_records_driven_refusals.json, written by
// research/generalized-gemm/test/fuzz/nest_gate_records.py), or a record transplanted or edited
// by Inputs/edit_nest_records.py so that every check before the one under test passes. A
// refused nest stays as it is.
//
// The research tool's own records: the lowering's alias gate (no llvm.noalias), the status
// of the records the research does not verify, an explicit view that runs backwards,
// binary64; and a permutation (5 i) mod 16 of A's rows, which the tables lower.
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_driven_refusals.json record-driven=true report=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=HONEST
// HONEST: remark: GEMM not lowered from its record: nest 0 of noalias_missing: A may overlap C; declare the arguments llvm.noalias (C restrict) or pass assume-no-alias
// HONEST: remark: GEMM not lowered from its record: nest 0 of stepped: the record's status is VERIFIED_NOT_MAPPED
// HONEST: remark: GEMM not lowered from its record: nest 0 of scf: the record's status is NOT_VERIFIED
// HONEST: remark: GEMM 16x8x12 of nest 0 of permuted_formulas lowered from its record as 5 exact region(s) (tables:
// HONEST: remark: GEMM not lowered from its record: nest 0 of backwards: the view of A has the element stride -12 along I, and an explicit view takes positive strides
// HONEST: remark: GEMM not lowered from its record: nest 0 of in_f64: the update is not in f32, the device's binary32
//
// A record with @formulas' verified content on the nest's own binding (transplant): every
// check on the record passes, and a check on the nest refuses.
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven_refusals.json transplant-scf %S/Inputs/nest_records_driven.json formulas > %t.scf.0.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.scf.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=SCF
// SCF: remark: GEMM not lowered from its record: nest 0 of scf: the nest's loop scf.for is no affine.for, the only loop the record's lowering takes
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven_refusals.json transplant-stepped %S/Inputs/nest_records_driven.json formulas > %t.step.0.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.step.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=STEP
// STEP: remark: GEMM not lowered from its record: nest 0 of stepped: a loop of the nest has no constant bounds, a step other than 1, or results
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven_refusals.json transplant-not_perfect %S/Inputs/nest_records_driven.json formulas > %t.perfect.0.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.perfect.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=PERFECT
// PERFECT: remark: GEMM not lowered from its record: nest 0 of not_perfect: the nest is no perfect band: a loop's body holds more than the next loop
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven_refusals.json transplant-two_loops %S/Inputs/nest_records_driven.json formulas > %t.twoloops.0.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.twoloops.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=TWOLOOPS
// TWOLOOPS: remark: GEMM not lowered from its record: nest 0 of two_loops: the nest has 2 loops, and the record's GEMM is a box of three
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven_refusals.json transplant-extra_store %S/Inputs/nest_records_driven.json formulas > %t.extrastore.0.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.extrastore.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=EXTRASTORE
// EXTRASTORE: remark: GEMM not lowered from its record: nest 0 of extra_store: the loop body is not one multiply-accumulate: 3 loads, 2 stores, 1 multiplications, 1 additions
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven_refusals.json transplant-negated %S/Inputs/nest_records_driven.json formulas > %t.negated.0.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.negated.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=NEGATED
// NEGATED: remark: GEMM not lowered from its record: nest 0 of negated: the loop body holds arith.negf, and the record's lowering takes one multiply-accumulate and index arithmetic
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven_refusals.json transplant-not_its_cell %S/Inputs/nest_records_driven.json formulas > %t.cell.0.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.cell.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=CELL
// CELL: remark: GEMM not lowered from its record: nest 0 of not_its_cell: the update does not read the cell of C it writes
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven_refusals.json transplant-permuted_formulas %S/Inputs/nest_records_driven.json formulas > %t.noform.0.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.noform.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=NOFORM
// NOFORM: remark: GEMM not lowered from its record: nest 0 of permuted_formulas: the nest reads A at an index the lowering cannot read as an affine form of its loops
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven_refusals.json transplant-oob_formulas %S/Inputs/nest_records_driven.json formulas > %t.bounds.0.json
// RUN: %python %S/Inputs/edit_nest_records.py %t.bounds.0.json view@oob_formulas > %t.bounds.1.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.bounds.1.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=BOUNDS
// BOUNDS: remark: GEMM not lowered from its record: nest 0 of oob_formulas: A is read out of bounds along dimension 0
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven_refusals.json transplant-not_a_role %S/Inputs/nest_records_driven.json formulas > %t.role.0.json
// RUN: %python %S/Inputs/edit_nest_records.py %t.role.0.json a-reads-j@not_a_role > %t.role.1.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.role.1.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=ROLE
// ROLE: remark: GEMM not lowered from its record: nest 0 of not_a_role: dimension 1 of A reads the loop of a coordinate that is none of its roles
// A record with @tables_flat's content: the tables' checks.
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven_refusals.json transplant-memory_index %S/Inputs/nest_records_driven.json tables_flat > %t.memory.0.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.memory.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=MEMORY
// MEMORY: remark: GEMM not lowered from its record: nest 0 of memory_index: the loop body is not one multiply-accumulate: 4 loads, 1 stores, 1 multiplications, 1 additions
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven_refusals.json transplant-argument_index %S/Inputs/nest_records_driven.json tables_flat > %t.argument.0.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.argument.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=ARGUMENT
// ARGUMENT: remark: GEMM not lowered from its record: nest 0 of argument_index: the address of A depends on a value that is no counter of the band's loops (an argument of the function: known only at run time)
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven_refusals.json transplant-a_reads_j %S/Inputs/nest_records_driven.json tables_flat > %t.depends.0.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.depends.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=DEPENDS
// DEPENDS: remark: GEMM not lowered from its record: nest 0 of a_reads_j: the address of A depends on J, the coordinate it does not read (iteration (0, 1, 0))
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven_refusals.json transplant-oob_tables %S/Inputs/nest_records_driven.json tables_flat > %t.outside.0.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.outside.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=OUTSIDE
// OUTSIDE: remark: GEMM not lowered from its record: nest 0 of oob_tables: A is read or written out of bounds along dimension 0 at the iteration (15, 0, 0)
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven_refusals.json transplant-divides_by_zero %S/Inputs/nest_records_driven.json tables_flat > %t.zero.0.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.zero.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=ZERO
// ZERO: remark: GEMM not lowered from its record: nest 0 of divides_by_zero: the address arithmetic fails at the iteration (0, 0, 0): arith.divsi overflows, divides by zero or shifts too far
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven_refusals.json transplant-not_its_cell %S/Inputs/nest_records_driven.json tables_flat > %t.tablecell.0.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.tablecell.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=TABLECELL
// TABLECELL: remark: GEMM not lowered from its record: nest 0 of not_its_cell: the update does not read the cell of C it writes, at the iteration (0, 0, 0)
// The research's records with one of the research's own refusals taken away.
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven_refusals.json unmapped-only@c_conflict > %t.conflict.0.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.conflict.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=CONFLICT
// CONFLICT: remark: GEMM not lowered from its record: nest 0 of c_conflict: two cells (I, J) of C, (0, 0) and (8, 0), share the address 0
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven_refusals.json verify@c_overlap > %t.overlap.0.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.overlap.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=OVERLAP
// OVERLAP: remark: GEMM not lowered from its record: nest 0 of c_overlap: two cells of C share an address: the view of C is not one to one
// @formulas' record with one field edited.
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven.json view@formulas > %t.view.0.json
// RUN: systolic-opt %S/nest_records_driven.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.view.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=VIEW
// VIEW: remark: GEMM not lowered from its record: nest 0 of formulas: dimension 0 of the record's view of A is not the nest's index: 1 * loop 0 + 0
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven.json level@formulas > %t.level.0.json
// RUN: systolic-opt %S/nest_records_driven.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.level.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=LEVEL
// LEVEL: remark: GEMM not lowered from its record: nest 0 of formulas: level 0 of the record is not loop 0 of the nest: axis, direction or bounds differ
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven.json axes@formulas > %t.axes.0.json
// RUN: systolic-opt %S/nest_records_driven.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.axes.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=AXES
// AXES: remark: GEMM not lowered from its record: nest 0 of formulas: the record's axis I is not on level 0
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven.json extent@formulas > %t.extent.0.json
// RUN: systolic-opt %S/nest_records_driven.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.extent.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=EXTENT
// EXTENT: remark: GEMM not lowered from its record: nest 0 of formulas: the record's M is 15 and the loop of I runs 16 times
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven.json batch-axis@formulas > %t.batch.0.json
// RUN: systolic-opt %S/nest_records_driven.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.batch.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=BATCH
// BATCH: remark: GEMM not lowered from its record: nest 0 of formulas: the record has a batch axis, which the record's lowering does not map
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven.json extra-level@formulas > %t.extralevel.0.json
// RUN: systolic-opt %S/nest_records_driven.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.extralevel.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=EXTRALEVEL
// EXTRALEVEL: remark: GEMM not lowered from its record: nest 0 of formulas: the record has 4 levels, and the band 3 loops
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven.json element@formulas > %t.element.0.json
// RUN: systolic-opt %S/nest_records_driven.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.element.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=ELEMENT
// ELEMENT: remark: GEMM not lowered from its record: nest 0 of formulas: the record's element type is f64 and the nest's is f32
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven.json reduction@formulas > %t.reduction.0.json
// RUN: systolic-opt %S/nest_records_driven.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.reduction.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=REDUCTION
// REDUCTION: remark: GEMM not lowered from its record: nest 0 of formulas: the record's update is not one site of fadd over fmul
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven.json accumulator@formulas > %t.accumulator.0.json
// RUN: systolic-opt %S/nest_records_driven.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.accumulator.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=ACCUMULATOR
// ACCUMULATOR: remark: GEMM not lowered from its record: nest 0 of formulas: the record's addition takes the accumulator on the other side
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven.json flags@formulas > %t.flags.0.json
// RUN: systolic-opt %S/nest_records_driven.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.flags.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=FLAGS
// FLAGS: remark: GEMM not lowered from its record: nest 0 of formulas: the record's flags are not the nest's
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven.json flag-number@formulas > %t.flagnumber.0.json
// RUN: systolic-opt %S/nest_records_driven.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.flagnumber.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=FLAGNUMBER
// FLAGNUMBER: remark: GEMM not lowered from its record: nest 0 of formulas: the record is incomplete: a flag is no string
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven.json source-flag@formulas > %t.sourceflag.0.json
// RUN: systolic-opt %S/nest_records_driven.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.sourceflag.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=SOURCEFLAG
// SOURCEFLAG: remark: GEMM not lowered from its record: nest 0 of formulas: the record's `reassociation.source_flag_reassoc` is not the nest's flag
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven.json a-root-c@formulas > %t.aroot.0.json
// RUN: systolic-opt %S/nest_records_driven.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.aroot.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=AROOT
// AROOT: remark: GEMM not lowered from its record: nest 0 of formulas: the record's A and B are not the roots the product reads
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven.json c-root@formulas > %t.croot.0.json
// RUN: systolic-opt %S/nest_records_driven.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.croot.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=CROOT
// CROOT: remark: GEMM not lowered from its record: nest 0 of formulas: the record's C is root 0 and the nest stores to root 2
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven.json separation@formulas > %t.separation.0.json
// RUN: systolic-opt %S/nest_records_driven.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.separation.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=SEPARATION
// SEPARATION: remark: GEMM not lowered from its record: nest 0 of formulas: the record assumes roots 0 and 1 apart, and the lowering checks only A and B against C
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven.json proof@formulas > %t.proof.0.json
// RUN: systolic-opt %S/nest_records_driven.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.proof.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=PROOF
// PROOF: remark: GEMM not lowered from its record: nest 0 of formulas: the record proves two roots apart by an allocation, and both are arguments here
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven.json kind@formulas > %t.kind.0.json
// RUN: systolic-opt %S/nest_records_driven.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.kind.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=KIND
// KIND: remark: GEMM not lowered from its record: nest 0 of formulas: the record rests on a separation it does not know (witness)
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven.json unproved@formulas > %t.unproved.0.json
// RUN: systolic-opt %S/nest_records_driven.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.unproved.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=UNPROVED
// UNPROVED: remark: GEMM not lowered from its record: nest 0 of formulas: the record does not prove the view of A
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven.json parameter@formulas > %t.parameter.0.json
// RUN: systolic-opt %S/nest_records_driven.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.parameter.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=PARAMETER
// PARAMETER: remark: GEMM not lowered from its record: nest 0 of formulas: the record has no affine closed form for dimension 0 of the view of A
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven.json extra-dimension@formulas > %t.extradimension.0.json
// RUN: systolic-opt %S/nest_records_driven.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.extradimension.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=EXTRADIMENSION
// EXTRADIMENSION: remark: GEMM not lowered from its record: nest 0 of formulas: the record's view of A has 3 dimensions, and A has rank 2
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven.json incomplete@formulas > %t.incomplete.0.json
// RUN: systolic-opt %S/nest_records_driven.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.incomplete.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=INCOMPLETE
// INCOMPLETE: remark: GEMM not lowered from its record: nest 0 of formulas: the record is incomplete: `gemm.views` is no object
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven.json scope@formulas > %t.scope.0.json
// RUN: systolic-opt %S/nest_records_driven.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.scope.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=SCOPE
// SCOPE: remark: GEMM not lowered from its record: nest 0 of formulas: the record's claims hold for a range of arguments only, and the lowering is for every value
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven.json authorizes@formulas > %t.authorizes.0.json
// RUN: systolic-opt %S/nest_records_driven.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.authorizes.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=AUTHORIZES
// AUTHORIZES: remark: GEMM not lowered from its record: nest 0 of formulas: the record claims to authorize a lowering, and no record this pass reads does
// @tables_flat's record edited.
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven.json extra-refusal@tables_flat > %t.refusal.0.json
// RUN: systolic-opt %S/nest_records_driven.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.refusal.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=REFUSAL
// REFUSAL: remark: GEMM not lowered from its record: nest 0 of tables_flat: the record's status is VERIFIED_NOT_MAPPED: the view of C has no proposed closed form
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven.json table-form@tables_flat > %t.tableform.0.json
// RUN: systolic-opt %S/nest_records_driven.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.tableform.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=TABLEFORM
// TABLEFORM: remark: GEMM not lowered from its record: nest 0 of tables_flat: the record's closed form of A is not the nest's address at the iteration (0, 0, 0)
// The tables' budget of iterations, below the domain's 1536.
// RUN: systolic-opt %S/nest_records_driven.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_driven.json record-driven=true record-table-budget=1000" -o /dev/null 2>&1 | FileCheck %s --check-prefix=BUDGET
// BUDGET: remark: GEMM not lowered from its record: nest 0 of tables_flat: the domain has more than 1000 iterations, the most the tables are built for at compile time
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven_refusals.json transplant-index_before %S/Inputs/nest_records_driven.json tables_flat > %t.before.0.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.before.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=BEFORE
// BEFORE: remark: GEMM not lowered from its record: nest 0 of index_before: the address of A depends on a value computed before the nest (memref.load): known only at run time
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven_refusals.json transplant-four_loops %S/Inputs/nest_records_driven.json formulas > %t.fourloops.0.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.fourloops.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=FOURLOOPS
// FOURLOOPS: remark: GEMM not lowered from its record: nest 0 of four_loops: the nest has more than three loops, and the record's GEMM is a box of three
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven_refusals.json transplant-stores_a_value %S/Inputs/nest_records_driven.json formulas > %t.storesvalue.0.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.storesvalue.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=STORESVALUE
// STORESVALUE: remark: GEMM not lowered from its record: nest 0 of stores_a_value: the store does not write the sum
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven_refusals.json transplant-adds_a_value %S/Inputs/nest_records_driven.json formulas > %t.addsvalue.0.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.addsvalue.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=ADDSVALUE
// ADDSVALUE: remark: GEMM not lowered from its record: nest 0 of adds_a_value: the addition does not add the product
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven_refusals.json transplant-adds_the_product_twice %S/Inputs/nest_records_driven.json formulas > %t.twice.0.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.twice.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=TWICE
// TWICE: remark: GEMM not lowered from its record: nest 0 of adds_the_product_twice: the addition's other operand is no load of the body
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven_refusals.json transplant-multiplies_the_accumulator %S/Inputs/nest_records_driven.json formulas > %t.accfactor.0.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.accfactor.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=ACCFACTOR
// ACCFACTOR: remark: GEMM not lowered from its record: nest 0 of multiplies_the_accumulator: the product is not of two loads of the body other than the accumulator's
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven_refusals.json transplant-squares %S/Inputs/nest_records_driven.json formulas > %t.squares.0.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.squares.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=SQUARES
// SQUARES: remark: GEMM not lowered from its record: nest 0 of squares: the product squares one load
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven_refusals.json transplant-accumulates_from_d %S/Inputs/nest_records_driven.json formulas > %t.fromd.0.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.fromd.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=FROMD
// FROMD: remark: GEMM not lowered from its record: nest 0 of accumulates_from_d: the accumulator is not read from the memref the sum is stored to
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven_refusals.json transplant-in_f64 %S/Inputs/nest_records_driven.json formulas > %t.f64.0.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.f64.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=F64
// F64: remark: GEMM not lowered from its record: nest 0 of in_f64: the update is not in f32, the device's binary32
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven_refusals.json transplant-dynamic %S/Inputs/nest_records_driven.json formulas > %t.dynamic.0.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.dynamic.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=DYNAMIC
// DYNAMIC: remark: GEMM not lowered from its record: nest 0 of dynamic: A is no memref of static shape and the identity layout over the update's element type
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven_refusals.json transplant-c_permuted %S/Inputs/nest_records_driven.json formulas > %t.cperm.0.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.cperm.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=CPERM
// CPERM: remark: GEMM not lowered from its record: nest 0 of c_permuted: the nest reads C at an index the lowering cannot read as an affine form of its loops
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven_refusals.json transplant-i32_index %S/Inputs/nest_records_driven.json tables_flat > %t.i32.0.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.i32.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=I32
// I32: remark: GEMM not lowered from its record: nest 0 of i32_index: the address of A is computed from i32, and the tables evaluate index and i64 arithmetic only
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_driven_refusals.json transplant-c_load_permuted %S/Inputs/nest_records_driven.json formulas > %t.cloadperm.0.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.cloadperm.0.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=CLOADPERM
// CLOADPERM: remark: GEMM not lowered from its record: nest 0 of c_load_permuted: the nest reads C at an index the lowering cannot read as an affine form of its loops
// C is not declared llvm.noalias: the research verifies the nest with the pairs authorized,
// and the lowering's alias gate still refuses.
func.func @noalias_missing(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32>) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %x = memref.load %a[%i, %k] : memref<16x12xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
// A loop of step 2: the research does not verify the coordinates over the scope.
func.func @stepped(%a: memref<32x12xf32>, %b: memref<12x8xf32>, %c: memref<32x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 32 step 2 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %z = memref.load %c[%i, %j] : memref<32x8xf32>
        %x = memref.load %a[%i, %k] : memref<32x12xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<32x8xf32>
      }
    }
  }
  return
}
// scf.for loops: the research proposes no candidate, and the lowering takes affine.for only.
func.func @scf(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  scf.for %i = %c0 to %c16 step %c1 {
    scf.for %j = %c0 to %c8 step %c1 {
      scf.for %k = %c0 to %c12 step %c1 {
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %x = memref.load %a[%i, %k] : memref<16x12xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
// A's row read from memory: an address that is data.
func.func @memory_index(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}, %p: memref<16xindex>) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %r = memref.load %p[%i] : memref<16xindex>
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %x = memref.load %a[%r, %k] : memref<16x12xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
// A's row offset by an argument: an address known only at run time.
func.func @argument_index(%a: memref<32x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}, %s0: index) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %r = arith.addi %i, %s0 : index
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %x = memref.load %a[%r, %k] : memref<32x12xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
// A read at (k + j) mod 12: its address depends on J.
func.func @a_reads_j(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %t = arith.addi %k, %j : index
        %w = arith.constant 12 : index
        %q = arith.remsi %t, %w : index
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %x = memref.load %a[%i, %q] : memref<16x12xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
// C at (8 i + j) mod 64: two cells (I, J) share an address.
func.func @c_conflict(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<128xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %w = arith.constant 8 : index
        %r = arith.muli %i, %w : index
        %t = arith.addi %r, %j : index
        %v = arith.constant 64 : index
        %q = arith.remsi %t, %v : index
        %z = memref.load %c[%q] : memref<128xf32>
        %x = memref.load %a[%i, %k] : memref<16x12xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%q] : memref<128xf32>
      }
    }
  }
  return
}
// A[i + 1][k] with 16 rows: the last row is read out of bounds.
func.func @oob_formulas(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %o = arith.constant 1 : index
        %r = arith.addi %i, %o : index
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %x = memref.load %a[%r, %k] : memref<16x12xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
// A flat, one row on: 12 i + k + 12 reads past the end; the constants before the nest.
func.func @oob_tables(%a: memref<192xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %w = arith.constant 12 : index
  %o = arith.constant 12 : index
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %r = arith.muli %i, %w : index
        %t = arith.addi %r, %k : index
        %u = arith.addi %t, %o : index
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %x = memref.load %a[%u] : memref<192xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
// A's row i / (k - k): the address arithmetic divides by zero.
func.func @divides_by_zero(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %z0 = arith.subi %k, %k : index
        %r = arith.divsi %i, %z0 : index
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %x = memref.load %a[%r, %k] : memref<16x12xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
// The accumulator is read from C[i][j + 1] and the sum stored to C[i][j].
func.func @not_its_cell(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x9xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %one = arith.constant 1 : index
      %j1 = arith.addi %j, %one : index
      %z = memref.load %c[%i, %j1] : memref<16x9xf32>
        %x = memref.load %a[%i, %k] : memref<16x12xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x9xf32>
      }
    }
  }
  return
}
// The sum stored twice, to C and to D.
func.func @extra_store(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}, %d: memref<16x8xf32>) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %x = memref.load %a[%i, %k] : memref<16x12xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
        memref.store %s, %d[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
// A store between the loops of j and k: no perfect band.
func.func @not_perfect(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}, %d: memref<16x8xf32>) {
  %zero = arith.constant 0.0 : f32
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      memref.store %zero, %d[%i, %j] : memref<16x8xf32>
      affine.for %k = 0 to 12 {
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %x = memref.load %a[%i, %k] : memref<16x12xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
// Two loops (K = 1).
func.func @two_loops(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      %c0 = arith.constant 0 : index
      %z = memref.load %c[%i, %j] : memref<16x8xf32>
      %x = memref.load %a[%i, %c0] : memref<16x12xf32>
      %y = memref.load %b[%c0, %j] : memref<12x8xf32>
      %m = arith.mulf %x, %y : f32
      %s = arith.addf %z, %m fastmath<reassoc> : f32
      memref.store %s, %c[%i, %j] : memref<16x8xf32>
    }
  }
  return
}
// The product negated before it is added: an operation the update does not have.
func.func @negated(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %x = memref.load %a[%i, %k] : memref<16x12xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %n = arith.negf %m : f32
        %s = arith.addf %z, %n fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
// A's rows (5 i) mod 16: no affine form, so no formula reads it.
func.func @permuted_formulas(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %w = arith.constant 5 : index
        %v = arith.constant 16 : index
        %t = arith.muli %i, %w : index
        %r = arith.remsi %t, %v : index
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %x = memref.load %a[%r, %k] : memref<16x12xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
// C flat at 4 i + j, j up to 7: two cells (I, J) share an address.
func.func @c_overlap(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<128xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %w = arith.constant 4 : index
        %r = arith.muli %i, %w : index
        %t = arith.addi %r, %j : index
        %z = memref.load %c[%t] : memref<128xf32>
        %x = memref.load %a[%i, %k] : memref<16x12xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%t] : memref<128xf32>
      }
    }
  }
  return
}
// A[i][k + j]: a dimension of A reads J, which is no role of A.
func.func @not_a_role(%a: memref<16x20xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %t = arith.addi %k, %j : index
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %x = memref.load %a[%i, %t] : memref<16x20xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
// A flat at 12 (15 - i) + k: a view that runs backwards, with a negative element stride.
func.func @backwards(%a: memref<192xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %f = arith.constant 15 : index
        %w = arith.constant 12 : index
        %r = arith.subi %f, %i : index
        %q = arith.muli %r, %w : index
        %u = arith.addi %q, %k : index
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %x = memref.load %a[%u] : memref<192xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
// A's row offset by a value loaded before the nest: an address known only at run time.
func.func @index_before(%a: memref<40x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}, %p: memref<16xindex>) {
  %c0 = arith.constant 0 : index
  %r0 = memref.load %p[%c0] : memref<16xindex>
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %r = arith.addi %i, %r0 : index
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %x = memref.load %a[%r, %k] : memref<40x12xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
// A fourth loop around the three: no box of three.
func.func @four_loops(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  affine.for %r = 0 to 2 {
    affine.for %i = 0 to 16 {
      affine.for %j = 0 to 8 {
        affine.for %k = 0 to 12 {
          %z = memref.load %c[%i, %j] : memref<16x8xf32>
          %x = memref.load %a[%i, %k] : memref<16x12xf32>
          %y = memref.load %b[%k, %j] : memref<12x8xf32>
          %m = arith.mulf %x, %y : f32
          %s = arith.addf %z, %m fastmath<reassoc> : f32
          memref.store %s, %c[%i, %j] : memref<16x8xf32>
        }
      }
    }
  }
  return
}
// The store writes A's value, not the sum.
func.func @stores_a_value(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %x = memref.load %a[%i, %k] : memref<16x12xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %x, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
// The addition adds A's value, not the product.
func.func @adds_a_value(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %x = memref.load %a[%i, %k] : memref<16x12xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %x fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
// The addition adds the product to itself.
func.func @adds_the_product_twice(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %x = memref.load %a[%i, %k] : memref<16x12xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %m, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
// The product reads the accumulator.
func.func @multiplies_the_accumulator(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %x = memref.load %a[%i, %k] : memref<16x12xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %z, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
// The product squares A's value.
func.func @squares(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %x = memref.load %a[%i, %k] : memref<16x12xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %x : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
// The accumulator read from D, the sum stored to C.
func.func @accumulates_from_d(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}, %d: memref<16x8xf32>) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %z = memref.load %d[%i, %j] : memref<16x8xf32>
        %x = memref.load %a[%i, %k] : memref<16x12xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
// Binary64: the device is binary32.
func.func @in_f64(%a: memref<16x12xf64>, %b: memref<12x8xf64>, %c: memref<16x8xf64> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %z = memref.load %c[%i, %j] : memref<16x8xf64>
        %x = memref.load %a[%i, %k] : memref<16x12xf64>
        %y = memref.load %b[%k, %j] : memref<12x8xf64>
        %m = arith.mulf %x, %y : f64
        %s = arith.addf %z, %m fastmath<reassoc> : f64
        memref.store %s, %c[%i, %j] : memref<16x8xf64>
      }
    }
  }
  return
}
// A of a dynamic leading extent.
func.func @dynamic(%a: memref<?x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %x = memref.load %a[%i, %k] : memref<?x12xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
// C's columns (3 j) mod 8: no affine form of C's index.
func.func @c_permuted(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %w = arith.constant 3 : index
        %v = arith.constant 8 : index
        %t = arith.muli %j, %w : index
        %q = arith.remsi %t, %v : index
        %z = memref.load %c[%i, %q] : memref<16x8xf32>
        %x = memref.load %a[%i, %k] : memref<16x12xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %q] : memref<16x8xf32>
      }
    }
  }
  return
}
// A's row computed in i32: the tables evaluate 64-bit arithmetic only.
func.func @i32_index(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %n = arith.index_cast %i : index to i32
        %one = arith.constant 0 : i32
        %m32 = arith.addi %n, %one : i32
        %r = arith.index_cast %m32 : i32 to index
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %x = memref.load %a[%r, %k] : memref<16x12xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
// The accumulator read from C[i][(3 j) mod 8], the sum stored to C[i][j].
func.func @c_load_permuted(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %w3 = arith.constant 3 : index
      %w8 = arith.constant 8 : index
      %t3 = arith.muli %j, %w3 : index
      %q = arith.remsi %t3, %w8 : index
      %z = memref.load %c[%i, %q] : memref<16x8xf32>
        %x = memref.load %a[%i, %k] : memref<16x12xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}

