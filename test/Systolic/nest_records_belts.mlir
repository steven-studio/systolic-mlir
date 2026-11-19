// Stage 2 of docs/GENERALIZED_GEMM_SCOPE.md: the gate's checks on the shape of the nest, each
// reached by a record that passes every check on the record itself. The production lowering
// takes all seven nests below without records (batches, an assignment, a GEMM with K = 1 over
// two loops, a loop of step 2, a coordinate written I = i + 1 or I = 2 i, a run-time offset);
// a record of one box over three loops counted as written describes none of them.
//
// What the research tool says of them (Inputs/nest_records_belts.json, written by
// research/generalized-gemm/test/fuzz/nest_gate_records.py): five are not VERIFIED, and the
// two it verifies have a coordinate that is no loop's counter, which the gate refuses: the
// record's views are over the loops, the lowering's over the coordinates.
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_belts.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=HONEST
// HONEST: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of batched: the record's status is VERIFIED_NOT_MAPPED: the family has a batch coordinate (axis P), and batches are not mapped
// HONEST: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of assigned: the record's status is NO_GEMM
// HONEST: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of twoloops: the record's status is VERIFIED_NOT_MAPPED
// HONEST: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of stepped: the record's status is VERIFIED_NOT_MAPPED
// HONEST: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of offset: the GEMM's coordinate I is not the counter of one of its loops
// HONEST: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of doubled: the GEMM's coordinate I is not the counter of one of its loops
// HONEST: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of symbolic: the record's status is NOT_VERIFIED
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_belts.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=NONE
// NONE-NOT: lowered as
//
// Each nest's own record with @single's verified content in it (Inputs/edit_nest_records.py,
// transplant-NAME): the nest's binding and its function's roots, so every check on the record
// passes; the check on the nest's shape refuses, in its own words.
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_belts.json transplant-batched > %t.batched.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.batched.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=BATCHED
// BATCHED: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of batched: the GEMM has batch loops, and a record of one box describes none
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_belts.json transplant-assigned > %t.assigned.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.assigned.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=ASSIGNED
// ASSIGNED: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of assigned: the GEMM is an assignment C = A * B, and a record describes an accumulation
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_belts.json transplant-twoloops > %t.twoloops.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.twoloops.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=TWOLOOPS
// TWOLOOPS: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of twoloops: the GEMM's nest has 2 loops, and a record of one box has 3
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_belts.json transplant-stepped > %t.stepped.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.stepped.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=STEPPED
// STEPPED: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of stepped: a loop of the GEMM has no constant bounds or a step other than 1
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_belts.json transplant-offset > %t.offset.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.offset.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=OFFSET
// OFFSET: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of offset: the GEMM's coordinate I is not the counter of one of its loops
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_belts.json transplant-doubled > %t.doubled.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.doubled.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=DOUBLED
// DOUBLED: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of doubled: the GEMM's coordinate I is not the counter of one of its loops
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_belts.json transplant-symbolic > %t.symbolic.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.symbolic.json" -o /dev/null 2>&1 | FileCheck %s --check-prefix=SYMBOLIC
// SYMBOLIC: remark: affine GEMM not lowered to systolic.matmul_tile: nest record: nest 0 of symbolic: the lowering's view of A is not one role per dimension of a rank-2 array with no run-time offset, which a record compares with
// Two batches of C += A B.
func.func @batched(%a: memref<2x16x12xf32>, %b: memref<2x12x8xf32>, %c: memref<2x16x8xf32> {llvm.noalias}) {
  affine.for %p = 0 to 2 {
    affine.for %i = 0 to 16 {
      affine.for %j = 0 to 8 {
        affine.for %k = 0 to 12 {
          %z = affine.load %c[%p, %i, %j] : memref<2x16x8xf32>
          %x = affine.load %a[%p, %i, %k] : memref<2x16x12xf32>
          %y = affine.load %b[%p, %k, %j] : memref<2x12x8xf32>
          %m = arith.mulf %x, %y : f32
          %s = arith.addf %z, %m fastmath<reassoc> : f32
          affine.store %s, %c[%p, %i, %j] : memref<2x16x8xf32>
        }
      }
    }
  }
  return
}
// C = A B over two loops (K = 1), each element of C written once.
func.func @assigned(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      %x = affine.load %a[%i, 0] : memref<16x12xf32>
      %y = affine.load %b[0, %j] : memref<12x8xf32>
      %m = arith.mulf %x, %y fastmath<reassoc> : f32
      affine.store %m, %c[%i, %j] : memref<16x8xf32>
    }
  }
  return
}
// C += A B with K = 1, over two loops.
func.func @twoloops(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      %z = affine.load %c[%i, %j] : memref<16x8xf32>
      %x = affine.load %a[%i, 0] : memref<16x12xf32>
      %y = affine.load %b[0, %j] : memref<12x8xf32>
      %m = arith.mulf %x, %y : f32
      %s = arith.addf %z, %m fastmath<reassoc> : f32
      affine.store %s, %c[%i, %j] : memref<16x8xf32>
    }
  }
  return
}
// The rows of C every second one: a loop of step 2.
func.func @stepped(%a: memref<32x12xf32>, %b: memref<12x8xf32>, %c: memref<32x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 32 step 2 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %z = affine.load %c[%i, %j] : memref<32x8xf32>
        %x = affine.load %a[%i, %k] : memref<32x12xf32>
        %y = affine.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        affine.store %s, %c[%i, %j] : memref<32x8xf32>
      }
    }
  }
  return
}
// C[i + 1][j] += A[i + 1][k] B[k][j]: the lowering's coordinate is I = i + 1.
func.func @offset(%a: memref<17x12xf32>, %b: memref<12x8xf32>, %c: memref<17x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %z = affine.load %c[%i + 1, %j] : memref<17x8xf32>
        %x = affine.load %a[%i + 1, %k] : memref<17x12xf32>
        %y = affine.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        affine.store %s, %c[%i + 1, %j] : memref<17x8xf32>
      }
    }
  }
  return
}
// C[2 i][j] += A[2 i][k] B[k][j]: the lowering's coordinate is I = 2 i.
func.func @doubled(%a: memref<32x12xf32>, %b: memref<12x8xf32>, %c: memref<32x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %z = affine.load %c[%i * 2, %j] : memref<32x8xf32>
        %x = affine.load %a[%i * 2, %k] : memref<32x12xf32>
        %y = affine.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        affine.store %s, %c[%i * 2, %j] : memref<32x8xf32>
      }
    }
  }
  return
}
// A read at a run-time offset: A[i + s][k].
func.func @symbolic(%a: memref<32x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}, %s0: index) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %z = affine.load %c[%i, %j] : memref<16x8xf32>
        %x = affine.load %a[%i + symbol(%s0), %k] : memref<32x12xf32>
        %y = affine.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        affine.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
