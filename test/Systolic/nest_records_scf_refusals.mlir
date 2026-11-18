// GEMMs written with scf.for that the lowering from records refuses (stage 3 of
// docs/GENERALIZED_GEMM_SCOPE.md, on scf.for loops), each with its reason, and two it takes. A
// refused nest stays as it is. Each case is the research tool's own record of a nest
// (Inputs/nest_records_scf_refusals.json, written by
// research/generalized-gemm/test/fuzz/nest_gate_records.py), or a record with @scf_gemm's
// verified content (Inputs/nest_records_scf.json) transplanted onto the nest's own binding by
// Inputs/edit_nest_records.py, so that every check on the record passes and a check on the
// nest refuses.
//
// The research tool's own records: the status of each record the research does not verify, the
// alias gate, the floating-point contract; unsigned comparisons of bounds that are not
// negative, and an index k + 1 inside its array, which are lowered.
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_scf_refusals.json record-driven=true report=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=HONEST
// HONEST: remark: GEMM not lowered from its record: nest 0 of iterargs: the record's status is NOT_VERIFIED: no claim was made about the nest: the gate is UNSUPPORTED (a scalar accumulator has no exact output view)
// HONEST: remark: GEMM not lowered from its record: nest 0 of alias: A may overlap C; declare the arguments llvm.noalias (C restrict) or pass assume-no-alias
// HONEST: remark: GEMM not lowered from its record: nest 0 of outside: the record's status is NOT_VERIFIED
// HONEST: remark: GEMM not lowered from its record: nest 0 of second_write: the record's status is NOT_VERIFIED: the nest writes memory outside the multiply-accumulate: 1 write(s) in the frame of the computation claim
// HONEST: remark: GEMM not lowered from its record: nest 0 of calls: the record's status is {{UNSUPPORTED}}: the nest has no faithful copy: the nest calls a function (func.call), and a callee is not outlined
// HONEST: remark: GEMM not lowered from its record: nest 0 of dynamic: the record's status is NOT_VERIFIED
// HONEST: remark: GEMM not lowered from its record: nest 0 of stepped: the record's status is VERIFIED_NOT_MAPPED
// HONEST: remark: GEMM not lowered from its record: nest 0 of strict: floating-point reassociation is not permitted
// HONEST: remark: GEMM not lowered from its record: nest 0 of parallel: the record's status is {{UNSUPPORTED}}: the nest has no faithful copy: the nest holds a parallel loop (scf.parallel)
// HONEST: remark: GEMM 16x8x12 of nest 0 of unsigned_ok lowered from its record as 5 exact region(s) (formulas:
// HONEST: remark: GEMM not lowered from its record: nest 0 of unsigned_negative: the record's status is NOT_VERIFIED
// HONEST: remark: GEMM not lowered from its record: nest 0 of unsigned_negative_upper: the record's status is
// HONEST: remark: GEMM 16x8x12 of nest 0 of beyond_ok lowered from its record as 5 exact region(s) (formulas:
// HONEST: remark: GEMM not lowered from its record: nest 0 of beyond: the record's status is NOT_VERIFIED
// HONEST-NOT: remark:
//
// The refused nests stay as written; the two taken are tiles.
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_scf_refusals.json record-driven=true" | FileCheck %s --check-prefix=KEPT
// KEPT-LABEL: func.func @iterargs
// KEPT-NOT: systolic.matmul_tile
// KEPT: scf.for {{.*}} iter_args
// KEPT-LABEL: func.func @alias
// KEPT-NOT: systolic.matmul_tile
// KEPT: scf.for
// KEPT-LABEL: func.func @outside
// KEPT-NOT: systolic.matmul_tile
// KEPT: scf.for
// KEPT-LABEL: func.func @second_write
// KEPT-NOT: systolic.matmul_tile
// KEPT: scf.for
// KEPT-LABEL: func.func @calls
// KEPT-NOT: systolic.matmul_tile
// KEPT: func.call @observe
// KEPT-LABEL: func.func @dynamic
// KEPT-NOT: systolic.matmul_tile
// KEPT: scf.for
// KEPT-LABEL: func.func @stepped
// KEPT-NOT: systolic.matmul_tile
// KEPT: scf.for
// KEPT-LABEL: func.func @strict
// KEPT-NOT: systolic.matmul_tile
// KEPT: scf.for
// KEPT-LABEL: func.func @parallel
// KEPT-NOT: systolic.matmul_tile
// KEPT: scf.parallel
// KEPT-LABEL: func.func @unsigned_ok
// KEPT-COUNT-5: systolic.matmul_tile
// KEPT-NOT: scf.for
// KEPT-LABEL: func.func @unsigned_negative
// KEPT-NOT: systolic.matmul_tile
// KEPT: scf.for unsigned
// KEPT-LABEL: func.func @unsigned_negative_upper
// KEPT-NOT: systolic.matmul_tile
// KEPT: scf.for unsigned
// KEPT-LABEL: func.func @beyond_ok
// KEPT-COUNT-5: systolic.matmul_tile
// KEPT-NOT: scf.for
// KEPT-LABEL: func.func @beyond
// KEPT-NOT: systolic.matmul_tile
// KEPT: scf.for
//
// Transplants: every check on the record passes, and the nest's own shape refuses.
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_scf_refusals.json transplant-iterargs %S/Inputs/nest_records_scf.json scf_gemm > %t.iterargs.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.iterargs.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=ITERARGS
// ITERARGS: remark: GEMM not lowered from its record: nest 0 of iterargs: a loop of the nest (scf.for) carries values from one iteration to the next (iter_args): its reduction is held in a value, not in memory, and the record's lowering takes reductions in memory only
// ITERARGS-NOT: remark:
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_scf_refusals.json transplant-dynamic %S/Inputs/nest_records_scf.json scf_gemm > %t.dynamic.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.dynamic.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=DYNAMIC
// DYNAMIC: remark: GEMM not lowered from its record: nest 0 of dynamic: a loop of the nest has a bound that is no constant: the scf.for's bounds are known only at run time
// DYNAMIC-NOT: remark:
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_scf_refusals.json transplant-stepped %S/Inputs/nest_records_scf.json scf_gemm > %t.step.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.step.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=STEP
// STEP: remark: GEMM not lowered from its record: nest 0 of stepped: a loop of the nest has a step other than 1
// STEP-NOT: remark:
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_scf_refusals.json transplant-unsigned_negative %S/Inputs/nest_records_scf.json scf_gemm > %t.unsigned.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.unsigned.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=UNSIGNED
// UNSIGNED: remark: GEMM not lowered from its record: nest 0 of unsigned_negative: a loop of the nest compares its bounds unsigned, and a bound is negative
// UNSIGNED-NOT: remark:
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_scf_refusals.json transplant-unsigned_negative_upper %S/Inputs/nest_records_scf.json scf_gemm > %t.unsignedupper.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.unsignedupper.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=UNSIGNEDUPPER
// UNSIGNEDUPPER: remark: GEMM not lowered from its record: nest 0 of unsigned_negative_upper: a loop of the nest compares its bounds unsigned, and a bound is negative
// UNSIGNEDUPPER-NOT: remark:
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_scf_refusals.json transplant-parallel %S/Inputs/nest_records_scf.json scf_gemm > %t.parallel.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.parallel.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=PARALLEL
// PARALLEL: remark: GEMM not lowered from its record: nest 0 of parallel: the nest's loop scf.parallel is neither affine.for nor scf.for, the loops the record's lowering takes
// PARALLEL-NOT: remark:
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_scf_refusals.json transplant-second_write %S/Inputs/nest_records_scf.json scf_gemm > %t.second.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.second.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=SECOND
// SECOND: remark: GEMM not lowered from its record: nest 0 of second_write: the loop body is not one multiply-accumulate: 3 loads, 2 stores, 1 multiplications, 1 additions
// SECOND-NOT: remark:
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_scf_refusals.json transplant-calls %S/Inputs/nest_records_scf.json scf_gemm > %t.calls.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.calls.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=CALLS
// CALLS: remark: GEMM not lowered from its record: nest 0 of calls: the loop body holds func.call, and the record's lowering takes one multiply-accumulate and index arithmetic
// CALLS-NOT: remark:
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_scf_refusals.json transplant-beyond %S/Inputs/nest_records_scf_refusals.json beyond_ok > %t.beyond.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.beyond.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=BEYOND
// BEYOND: remark: GEMM not lowered from its record: nest 0 of beyond: A is read out of bounds along dimension 1
// BEYOND-NOT: remark:

func.func private @observe(f32)
// The reduction over K held in a value (iter_args), stored after the loop: the research
// verifies no claim (a scalar accumulator has no exact output view), and the lowering takes
// reductions in memory only.
func.func @iterargs(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  scf.for %i = %c0 to %c16 step %c1 {
    scf.for %j = %c0 to %c8 step %c1 {
      %init = memref.load %c[%i, %j] : memref<16x8xf32>
      %sum = scf.for %k = %c0 to %c12 step %c1 iter_args(%acc = %init) -> (f32) {
        %x = memref.load %a[%i, %k] : memref<16x12xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %acc, %m fastmath<reassoc> : f32
        scf.yield %s : f32
      }
      memref.store %sum, %c[%i, %j] : memref<16x8xf32>
    }
  }
  return
}
// No llvm.noalias on C: the research verifies the nest under the authorized names, and the
// lowering's alias gate refuses it.
func.func @alias(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32>) {
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
// K runs to 13 over arrays of 12: every run reads outside A, and the research verifies nothing.
func.func @outside(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c13 = arith.constant 13 : index
  %c16 = arith.constant 16 : index
  scf.for %i = %c0 to %c16 step %c1 {
    scf.for %j = %c0 to %c8 step %c1 {
      scf.for %k = %c0 to %c13 step %c1 {
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
// A second write beside the update: a side effect the GEMM does not have.
func.func @second_write(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}, %d: memref<16x8xf32> {llvm.noalias}) {
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
        memref.store %m, %d[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
// A call in the body: the research has no faithful copy of the nest.
func.func @calls(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
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
        func.call @observe(%s) : (f32) -> ()
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
// The bound of I is an argument, known only at run time.
func.func @dynamic(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  scf.for %i = %c0 to %n step %c1 {
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
// A step of 2 for I.
func.func @stepped(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  scf.for %i = %c0 to %c16 step %c2 {
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
// No reassoc on the accumulation: the floating-point contract keeps the loop's order.
func.func @strict(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
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
        %s = arith.addf %z, %m : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
// I and J in a parallel loop, whose iterations have no order of their own.
func.func @parallel(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  scf.parallel (%i, %j) = (%c0, %c0) to (%c16, %c8) step (%c1, %c1) {
    scf.for %k = %c0 to %c12 step %c1 {
      %z = memref.load %c[%i, %j] : memref<16x8xf32>
      %x = memref.load %a[%i, %k] : memref<16x12xf32>
      %y = memref.load %b[%k, %j] : memref<12x8xf32>
      %m = arith.mulf %x, %y : f32
      %s = arith.addf %z, %m fastmath<reassoc> : f32
      memref.store %s, %c[%i, %j] : memref<16x8xf32>
    }
  }
  return
}
// Unsigned comparisons of bounds that are not negative: the same iterations, lowered.
func.func @unsigned_ok(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  scf.for unsigned %i = %c0 to %c16 step %c1 {
    scf.for unsigned %j = %c0 to %c8 step %c1 {
      scf.for unsigned %k = %c0 to %c12 step %c1 {
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
// An unsigned comparison from -4: as unsigned, 2^64 - 4 is past 12, and the loop never runs.
func.func @unsigned_negative(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %cm4 = arith.constant -4 : index
  scf.for unsigned %i = %cm4 to %c12 step %c1 {
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
// An unsigned comparison up to -4: as unsigned, 2^64 - 4, which the loop would run to.
func.func @unsigned_negative_upper(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %cm4 = arith.constant -4 : index
  scf.for unsigned %i = %c0 to %cm4 step %c1 {
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
// A read at column k + 1 of an array of 13 columns: in bounds, lowered (and the template of
// @beyond's transplant).
func.func @beyond_ok(%a: memref<16x13xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  scf.for %i = %c0 to %c16 step %c1 {
    scf.for %j = %c0 to %c8 step %c1 {
      scf.for %k = %c0 to %c12 step %c1 {
        %one = arith.constant 1 : index
        %k1 = arith.addi %k, %one : index
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %x = memref.load %a[%i, %k1] : memref<16x13xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
// The same read of an array of 12 columns: column 12 is outside A.
func.func @beyond(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  scf.for %i = %c0 to %c16 step %c1 {
    scf.for %j = %c0 to %c8 step %c1 {
      scf.for %k = %c0 to %c12 step %c1 {
        %one = arith.constant 1 : index
        %k1 = arith.addi %k, %one : index
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %x = memref.load %a[%i, %k1] : memref<16x12xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
