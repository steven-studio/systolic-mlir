// Nests whose subscripts their loops carry that the lowering from records refuses (stage 3 of
// docs/GENERALIZED_GEMM_SCOPE.md; docs/GENERALIZED_GEMM_STATEMENT_BV.md, Section 14.9), each
// with its reason. A refused nest stays as it is. Each case is the research tool's own record
// of a nest (Inputs/nest_records_carried_refusals.json, made with --index-invariants and
// written by research/generalized-gemm/test/fuzz/nest_gate_records.py), or a record with the
// verified content of @offset_iter or @offset_while (Inputs/nest_records_carried.json)
// transplanted onto the nest's own binding by Inputs/edit_nest_records.py, so that every check
// on the record passes and a check on the nest refuses: the lowering reads the recurrence from
// the loop, and no record makes it read one that is not there.
//
// The research tool's own records.
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_carried_refusals.json record-driven=true report=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=HONEST
// HONEST: remark: GEMM not lowered from its record: nest 0 of start_dynamic: the record's status is NOT_VERIFIED
// HONEST: remark: GEMM not lowered from its record: nest 0 of step_dynamic: the record's status is NOT_VERIFIED
// HONEST: remark: GEMM not lowered from its record: nest 0 of last_value_read: a loop of the nest (scf.for) carries values from one iteration to the next (iter_args), and the record's lowering takes of them an index started at a constant and stepped by a constant only: value 0 has its last value read by the code after the loop, and the lowering would remove the loop that makes it
// HONEST: remark: GEMM not lowered from its record: nest 0 of doubled: the record's status is NOT_VERIFIED
// HONEST: remark: GEMM not lowered from its record: nest 0 of unchanged: a loop of the nest (scf.for) carries values from one iteration to the next (iter_args), and the record's lowering takes of them an index started at a constant and stepped by a constant only: value 0 is not stepped by a constant: the loop hands on another value than itself plus or minus a constant
// HONEST: remark: GEMM not lowered from its record: nest 0 of carried_i64: a loop of the nest (scf.for) carries values from one iteration to the next (iter_args), and the record's lowering takes of them an index started at a constant and stepped by a constant only: value 0 has type i64, and the record's lowering takes a carried index only
// HONEST: remark: GEMM not lowered from its record: nest 0 of through_j: the record's status is NOT_VERIFIED
// HONEST: remark: GEMM not lowered from its record: nest 0 of with_reduction: the record's status is NOT_VERIFIED: no claim was made about the nest: the gate is {{UNSUPPORTED}} (a scalar accumulator has no exact output view)
// HONEST: remark: GEMM not lowered from its record: nest 0 of affine_carries: a loop of the nest (affine.for) carries values from one iteration to the next (iter_args), and the record's lowering reads the carried indices of an scf.for or an scf.while only
// HONEST: remark: GEMM not lowered from its record: nest 0 of while_start_dynamic: the record's status is NOT_VERIFIED
// HONEST: remark: GEMM not lowered from its record: nest 0 of while_forwards_other: the record's status is NOT_VERIFIED
// HONEST: remark: GEMM not lowered from its record: nest 0 of while_last_value_read: a loop of the nest (scf.while) carries 2 values from one iteration to the next, and the record's lowering takes a counting loop, which carries its counter and indices stepped by a constant only: value 1 has its last value read by the code after the loop, and the lowering would remove the loop that makes it; value 0 (index) is stepped by a constant; value 1 (index) is stepped by a constant
// HONEST: remark: GEMM not lowered from its record: nest 0 of while_stepped_by_counter: the record's status is NOT_VERIFIED
// HONEST-NOT: remark:
//
// Every nest stays as written: no tile is made.
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_carried_refusals.json record-driven=true" | FileCheck %s --check-prefix=KEPT --implicit-check-not=systolic.matmul_tile
// KEPT-LABEL: func.func @start_dynamic(
// KEPT: {{iter_args|scf\.while}}
// KEPT-LABEL: func.func @step_dynamic(
// KEPT: {{iter_args|scf\.while}}
// KEPT-LABEL: func.func @last_value_read(
// KEPT: {{iter_args|scf\.while}}
// KEPT-LABEL: func.func @doubled(
// KEPT: {{iter_args|scf\.while}}
// KEPT-LABEL: func.func @unchanged(
// KEPT: {{iter_args|scf\.while}}
// KEPT-LABEL: func.func @carried_i64(
// KEPT: {{iter_args|scf\.while}}
// KEPT-LABEL: func.func @through_j(
// KEPT: {{iter_args|scf\.while}}
// KEPT-LABEL: func.func @with_reduction(
// KEPT: {{iter_args|scf\.while}}
// KEPT-LABEL: func.func @affine_carries(
// KEPT: {{iter_args|scf\.while}}
// KEPT-LABEL: func.func @while_start_dynamic(
// KEPT: {{iter_args|scf\.while}}
// KEPT-LABEL: func.func @while_forwards_other(
// KEPT: {{iter_args|scf\.while}}
// KEPT-LABEL: func.func @while_last_value_read(
// KEPT: {{iter_args|scf\.while}}
// KEPT-LABEL: func.func @while_stepped_by_counter(
// KEPT: {{iter_args|scf\.while}}
//
// A verified record on the nest's own binding: the lowering's own reading of the loop refuses.
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_carried_refusals.json transplant-start_dynamic %S/Inputs/nest_records_carried.json offset_iter > %t.startdynamic.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.startdynamic.json record-driven=true" 2>&1 | FileCheck %s --check-prefix=STARTDYNAMIC --implicit-check-not=systolic.matmul_tile
// STARTDYNAMIC: remark: GEMM not lowered from its record: nest 0 of start_dynamic: a loop of the nest (scf.for) carries values from one iteration to the next (iter_args), and the record's lowering takes of them an index started at a constant and stepped by a constant only: value 0 starts at a value that is no constant: known only at run time
// STARTDYNAMIC-NOT: remark:
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_carried_refusals.json transplant-step_dynamic %S/Inputs/nest_records_carried.json offset_iter > %t.stepdynamic.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.stepdynamic.json record-driven=true" 2>&1 | FileCheck %s --check-prefix=STEPDYNAMIC --implicit-check-not=systolic.matmul_tile
// STEPDYNAMIC: remark: GEMM not lowered from its record: nest 0 of step_dynamic: a loop of the nest (scf.for) carries values from one iteration to the next (iter_args), and the record's lowering takes of them an index started at a constant and stepped by a constant only: value 0 is not stepped by a constant: the loop hands on another value than itself plus or minus a constant
// STEPDYNAMIC-NOT: remark:
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_carried_refusals.json transplant-doubled %S/Inputs/nest_records_carried.json offset_iter > %t.doubled.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.doubled.json record-driven=true" 2>&1 | FileCheck %s --check-prefix=DOUBLED --implicit-check-not=systolic.matmul_tile
// DOUBLED: remark: GEMM not lowered from its record: nest 0 of doubled: a loop of the nest (scf.for) carries values from one iteration to the next (iter_args), and the record's lowering takes of them an index started at a constant and stepped by a constant only: value 0 is not stepped by a constant: the loop hands on another value than itself plus or minus a constant
// DOUBLED-NOT: remark:
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_carried_refusals.json transplant-through_j %S/Inputs/nest_records_carried.json offset_iter > %t.throughj.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.throughj.json record-driven=true" 2>&1 | FileCheck %s --check-prefix=THROUGHJ --implicit-check-not=systolic.matmul_tile
// THROUGHJ: remark: GEMM not lowered from its record: nest 0 of through_j: a loop of the nest (scf.for) carries values from one iteration to the next (iter_args), and the record's lowering takes of them an index started at a constant and stepped by a constant only: value 0 is not stepped by a constant: the loop hands on another value than itself plus or minus a constant
// THROUGHJ-NOT: remark:
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_carried_refusals.json transplant-with_reduction %S/Inputs/nest_records_carried.json offset_iter > %t.withreduction.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.withreduction.json record-driven=true" 2>&1 | FileCheck %s --check-prefix=WITHREDUCTION --implicit-check-not=systolic.matmul_tile
// WITHREDUCTION: remark: GEMM not lowered from its record: nest 0 of with_reduction: a loop of the nest (scf.for) carries values from one iteration to the next (iter_args): its reduction is held in a value, not in memory, and the record's lowering takes reductions in memory only
// WITHREDUCTION-NOT: remark:
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_carried_refusals.json transplant-while_start_dynamic %S/Inputs/nest_records_carried.json offset_while > %t.whilestartdynamic.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.whilestartdynamic.json record-driven=true" 2>&1 | FileCheck %s --check-prefix=WHILESTARTDYNAMIC --implicit-check-not=systolic.matmul_tile
// WHILESTARTDYNAMIC: remark: GEMM not lowered from its record: nest 0 of while_start_dynamic: a loop of the nest (scf.while) carries 2 values from one iteration to the next, and the record's lowering takes a counting loop, which carries its counter and indices stepped by a constant only: value 1 starts at a value that is no constant: known only at run time; value 0 (index) is stepped by a constant; value 1 (index) is stepped by a constant
// WHILESTARTDYNAMIC-NOT: remark:
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_carried_refusals.json transplant-while_forwards_other %S/Inputs/nest_records_carried.json offset_while > %t.whileforwardsother.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.whileforwardsother.json record-driven=true" 2>&1 | FileCheck %s --check-prefix=WHILEFORWARDSOTHER --implicit-check-not=systolic.matmul_tile
// WHILEFORWARDSOTHER: remark: GEMM not lowered from its record: nest 0 of while_forwards_other: a loop of the nest (scf.while) carries 2 values from one iteration to the next, and the record's lowering takes a counting loop, which carries its counter and indices stepped by a constant only: value 1 is not handed to the body as it is: scf.condition forwards another value; value 0 (index) is stepped by a constant; value 1 (index) is updated by the loop otherwise
// WHILEFORWARDSOTHER-NOT: remark:
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_carried_refusals.json transplant-while_stepped_by_counter %S/Inputs/nest_records_carried.json offset_while > %t.whilesteppedbycounter.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.whilesteppedbycounter.json record-driven=true" 2>&1 | FileCheck %s --check-prefix=WHILESTEPPEDBYCOUNTER --implicit-check-not=systolic.matmul_tile
// WHILESTEPPEDBYCOUNTER: remark: GEMM not lowered from its record: nest 0 of while_stepped_by_counter: a loop of the nest (scf.while) carries 2 values from one iteration to the next, and the record's lowering takes a counting loop, which carries its counter and indices stepped by a constant only: value 1 is not stepped by a constant: the loop hands on another value than itself plus or minus a constant; value 0 (index) is stepped by a constant; value 1 (index) is updated by the loop otherwise
// WHILESTEPPEDBYCOUNTER-NOT: remark:

// The index starts at an argument of the function: known only at run time.
func.func @start_dynamic(%a: memref<16x15xf32>, %b: memref<15x8xf32>, %c: memref<16x8xf32> {llvm.noalias}, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  scf.for %i = %c0 to %c16 step %c1 {
    scf.for %j = %c0 to %c8 step %c1 {
      %xf = scf.for %k = %c0 to %c12 step %c1 iter_args(%x = %n) -> (index) {
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %p = memref.load %a[%i, %x] : memref<16x15xf32>
        %q = memref.load %b[%x, %j] : memref<15x8xf32>
        %m = arith.mulf %p, %q : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
        %x1 = arith.addi %x, %c1 : index
        scf.yield %x1 : index
      }
    }
  }
  return
}
// The index is stepped by an argument of the function.
func.func @step_dynamic(%a: memref<16x15xf32>, %b: memref<15x8xf32>, %c: memref<16x8xf32> {llvm.noalias}, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  scf.for %i = %c0 to %c16 step %c1 {
    scf.for %j = %c0 to %c8 step %c1 {
      %xf = scf.for %k = %c0 to %c12 step %c1 iter_args(%x = %c2) -> (index) {
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %p = memref.load %a[%i, %x] : memref<16x15xf32>
        %q = memref.load %b[%x, %j] : memref<15x8xf32>
        %m = arith.mulf %p, %q : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
        %x1 = arith.addi %x, %n : index
        scf.yield %x1 : index
      }
    }
  }
  return
}
// The index's last value is read after its loop (by an addition nobody reads).
func.func @last_value_read(%a: memref<16x15xf32>, %b: memref<15x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  scf.for %i = %c0 to %c16 step %c1 {
    scf.for %j = %c0 to %c8 step %c1 {
      %xf = scf.for %k = %c0 to %c12 step %c1 iter_args(%x = %c2) -> (index) {
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %p = memref.load %a[%i, %x] : memref<16x15xf32>
        %q = memref.load %b[%x, %j] : memref<15x8xf32>
        %m = arith.mulf %p, %q : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
        %x1 = arith.addi %x, %c1 : index
        scf.yield %x1 : index
      }
      %dead = arith.addi %xf, %c1 : index
    }
  }
  return
}
// The index is doubled each iteration: no step by a constant.
func.func @doubled(%a: memref<16x15xf32>, %b: memref<15x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  scf.for %i = %c0 to %c16 step %c1 {
    scf.for %j = %c0 to %c8 step %c1 {
      %xf = scf.for %k = %c0 to %c12 step %c1 iter_args(%x = %c1) -> (index) {
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %p = memref.load %a[%i, %x] : memref<16x15xf32>
        %q = memref.load %b[%x, %j] : memref<15x8xf32>
        %m = arith.mulf %p, %q : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
        %x1 = arith.muli %x, %c2 : index
        scf.yield %x1 : index
      }
    }
  }
  return
}
// The carried index never changes: every iteration reads column 2.
func.func @unchanged(%a: memref<16x15xf32>, %b: memref<15x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  scf.for %i = %c0 to %c16 step %c1 {
    scf.for %j = %c0 to %c8 step %c1 {
      %xf = scf.for %k = %c0 to %c12 step %c1 iter_args(%x = %c2) -> (index) {
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %p = memref.load %a[%i, %x] : memref<16x15xf32>
        %q = memref.load %b[%x, %j] : memref<15x8xf32>
        %m = arith.mulf %p, %q : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
        scf.yield %x : index
      }
    }
  }
  return
}
// The carried value is an i64, cast to index for the accesses.
func.func @carried_i64(%a: memref<16x15xf32>, %b: memref<15x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  %z1 = arith.constant 1 : i64
  %z2 = arith.constant 2 : i64
  scf.for %i = %c0 to %c16 step %c1 {
    scf.for %j = %c0 to %c8 step %c1 {
      %xf = scf.for %k = %c0 to %c12 step %c1 iter_args(%x = %z2) -> (i64) {
        %xi = arith.index_cast %x : i64 to index
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %p = memref.load %a[%i, %xi] : memref<16x15xf32>
        %q = memref.load %b[%xi, %j] : memref<15x8xf32>
        %m = arith.mulf %p, %q : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
        %x1 = arith.addi %x, %z1 : i64
        scf.yield %x1 : i64
      }
    }
  }
  return
}
// The index is carried through the j loop too: the k loop starts it where its last run left it,
// not at a constant, and the j loop hands on another loop's result.
func.func @through_j(%a: memref<16x99xf32>, %b: memref<99x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  scf.for %i = %c0 to %c16 step %c1 {
    %xo = scf.for %j = %c0 to %c8 step %c1 iter_args(%xj = %c2) -> (index) {
      %xf = scf.for %k = %c0 to %c12 step %c1 iter_args(%x = %xj) -> (index) {
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %p = memref.load %a[%i, %x] : memref<16x99xf32>
        %q = memref.load %b[%x, %j] : memref<99x8xf32>
        %m = arith.mulf %p, %q : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
        %x1 = arith.addi %x, %c1 : index
        scf.yield %x1 : index
      }
      scf.yield %xf : index
    }
  }
  return
}
// A carried index beside a reduction held in a value (f32), stored after the loop.
func.func @with_reduction(%a: memref<16x15xf32>, %b: memref<15x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  scf.for %i = %c0 to %c16 step %c1 {
    scf.for %j = %c0 to %c8 step %c1 {
      %init = memref.load %c[%i, %j] : memref<16x8xf32>
      %xf, %sum = scf.for %k = %c0 to %c12 step %c1 iter_args(%x = %c2, %acc = %init) -> (index, f32) {
        %p = memref.load %a[%i, %x] : memref<16x15xf32>
        %q = memref.load %b[%x, %j] : memref<15x8xf32>
        %m = arith.mulf %p, %q : f32
        %s = arith.addf %acc, %m fastmath<reassoc> : f32
        %x1 = arith.addi %x, %c1 : index
        scf.yield %x1, %s : index, f32
      }
      memref.store %sum, %c[%i, %j] : memref<16x8xf32>
    }
  }
  return
}
// An affine.for carries the index: no carried value of an affine.for is read.
func.func @affine_carries(%a: memref<16x15xf32>, %b: memref<15x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  scf.for %i = %c0 to %c16 step %c1 {
    scf.for %j = %c0 to %c8 step %c1 {
      %xf = affine.for %k = 0 to 12 iter_args(%x = %c2) -> (index) {
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %p = memref.load %a[%i, %x] : memref<16x15xf32>
        %q = memref.load %b[%x, %j] : memref<15x8xf32>
        %m = arith.mulf %p, %q : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
        %x1 = arith.addi %x, %c1 : index
        affine.yield %x1 : index
      }
    }
  }
  return
}
// An scf.while starts its carried index at an argument of the function.
func.func @while_start_dynamic(%a: memref<16x15xf32>, %b: memref<15x8xf32>, %c: memref<16x8xf32> {llvm.noalias}, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  scf.for %i = %c0 to %c16 step %c1 {
    scf.for %j = %c0 to %c8 step %c1 {
      %kf, %xf = scf.while (%k = %c0, %xw = %n) : (index, index) -> (index, index) {
        %go = arith.cmpi slt, %k, %c12 : index
        scf.condition(%go) %k, %xw : index, index
      } do {
      ^bb0(%kb: index, %x: index):
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %p = memref.load %a[%i, %x] : memref<16x15xf32>
        %q = memref.load %b[%x, %j] : memref<15x8xf32>
        %m = arith.mulf %p, %q : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
        %k1 = arith.addi %kb, %c1 : index
        %x1 = arith.addi %x, %c1 : index
        scf.yield %k1, %x1 : index, index
      }
    }
  }
  return
}
// An scf.while hands its body another value than the carried index: the constant 2, every iteration.
func.func @while_forwards_other(%a: memref<16x15xf32>, %b: memref<15x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  scf.for %i = %c0 to %c16 step %c1 {
    scf.for %j = %c0 to %c8 step %c1 {
      %kf, %xf = scf.while (%k = %c0, %xw = %c2) : (index, index) -> (index, index) {
        %go = arith.cmpi slt, %k, %c12 : index
        scf.condition(%go) %k, %c2 : index, index
      } do {
      ^bb0(%kb: index, %x: index):
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %p = memref.load %a[%i, %x] : memref<16x15xf32>
        %q = memref.load %b[%x, %j] : memref<15x8xf32>
        %m = arith.mulf %p, %q : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
        %k1 = arith.addi %kb, %c1 : index
        %x1 = arith.addi %x, %c1 : index
        scf.yield %k1, %x1 : index, index
      }
    }
  }
  return
}
// An scf.while whose carried index's last value is read after it.
func.func @while_last_value_read(%a: memref<16x15xf32>, %b: memref<15x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  scf.for %i = %c0 to %c16 step %c1 {
    scf.for %j = %c0 to %c8 step %c1 {
      %kf, %xf = scf.while (%k = %c0, %xw = %c2) : (index, index) -> (index, index) {
        %go = arith.cmpi slt, %k, %c12 : index
        scf.condition(%go) %k, %xw : index, index
      } do {
      ^bb0(%kb: index, %x: index):
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %p = memref.load %a[%i, %x] : memref<16x15xf32>
        %q = memref.load %b[%x, %j] : memref<15x8xf32>
        %m = arith.mulf %p, %q : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
        %k1 = arith.addi %kb, %c1 : index
        %x1 = arith.addi %x, %c1 : index
        scf.yield %k1, %x1 : index, index
      }
      %dead = arith.addi %xf, %c1 : index
    }
  }
  return
}
// An scf.while steps its carried index by its counter: 2, 2, 3, 5, 8, ...; no step by a constant.
func.func @while_stepped_by_counter(%a: memref<16x99xf32>, %b: memref<99x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  scf.for %i = %c0 to %c16 step %c1 {
    scf.for %j = %c0 to %c8 step %c1 {
      %kf, %xf = scf.while (%k = %c0, %xw = %c2) : (index, index) -> (index, index) {
        %go = arith.cmpi slt, %k, %c12 : index
        scf.condition(%go) %k, %xw : index, index
      } do {
      ^bb0(%kb: index, %x: index):
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %p = memref.load %a[%i, %x] : memref<16x99xf32>
        %q = memref.load %b[%x, %j] : memref<99x8xf32>
        %m = arith.mulf %p, %q : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
        %k1 = arith.addi %kb, %c1 : index
        %x1 = arith.addi %x, %kb : index
        scf.yield %k1, %x1 : index, index
      }
    }
  }
  return
}
