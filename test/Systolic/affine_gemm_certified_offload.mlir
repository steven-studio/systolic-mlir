// Phase 5.4.3d-3b-6: the certified device-job lowering of recognized GEMMs
// (docs/AFFINE_GEMM_MAPPING.md section 27). With the program's permission to
// reassociate (allow-fp-reassociation here; the reassoc flag in a program)
// a standard contiguous GEMM becomes the same systolic.matmul_tile plan the
// established fast path emits, on views of the source memrefs; the hidden
// test control certified-offload-first makes the certified lowering take
// the nest before the fast path so that the two can be compared.
//
// RUN: systolic-opt --systolic-declare-fpga-fleet \
// RUN:   --affine-matmul-to-systolic="general-affine=true certified-offload-first=true allow-fp-reassociation=true report=true" \
// RUN:   %s 2>&1 | FileCheck %s --check-prefix=CERTIFIED
//
// The two producers of one plan agree byte for byte.
// RUN: systolic-opt --systolic-declare-fpga-fleet \
// RUN:   --affine-matmul-to-systolic="general-affine=true allow-fp-reassociation=true" %s -o %t.fast.mlir
// RUN: systolic-opt --systolic-declare-fpga-fleet \
// RUN:   --affine-matmul-to-systolic="general-affine=true certified-offload-first=true allow-fp-reassociation=true" %s -o %t.certified.mlir
// RUN: diff %t.fast.mlir %t.certified.mlir
//
// Under strict floating point nothing is offloaded (section 6): the device's
// order of additions, its +0 start and its flush-to-zero are not the loop's;
// the guarded MAC nest of Phase 3b-5 takes the nests instead.
// RUN: systolic-opt --systolic-declare-fpga-fleet \
// RUN:   --affine-matmul-to-systolic="general-affine=true certified-offload-first=true report=true" \
// RUN:   %s 2>&1 | FileCheck %s --check-prefix=STRICT
//
// The emitted tiles are real device jobs of the existing runtime ABI.
// RUN: systolic-opt --systolic-declare-fpga-fleet \
// RUN:   --affine-matmul-to-systolic="general-affine=true certified-offload-first=true allow-fp-reassociation=true" \
// RUN:   --lower-systolic-matmul-tile-to-fpga --systolic-runtime-to-llvm %s | FileCheck %s --check-prefix=RUNTIME
//
// The remarks precede the printed module.
// CERTIFIED-DAG: remark: affine GEMM 16x16x16 lowered as 10 exact region(s) by the certified device-job lowering (docs/AFFINE_GEMM_MAPPING.md section 27) on 4 accelerator(s)
// CERTIFIED-DAG: remark: affine GEMM 4x4x4 lowered as 1 exact region(s) by the certified device-job lowering
// CERTIFIED-DAG: remark: affine GEMM 6x8x40 lowered as 3 exact region(s) by the certified device-job lowering
// CERTIFIED-DAG: remark: certified device offload not applied: UNKNOWN: the logical domain is not a box
// CERTIFIED-DAG: remark: certified device offload not applied: UNKNOWN: the coordinate map is not unimodular
// CERTIFIED-DAG: remark: affine GEMM replaced by its exact guarded MAC nest
// CERTIFIED-DAG: remark: guarded MAC replacement not applied: UNKNOWN: A may alias C
//
// STRICT-DAG: remark: certified device offload not applied: UNKNOWN: strict floating-point semantics: the device sums each invocation from +0 in its own order
// STRICT-DAG: remark: affine GEMM not lowered to systolic.matmul_tile: floating-point reassociation is not permitted
// STRICT-DAG: remark: affine GEMM replaced by its exact guarded MAC nest
// STRICT-NOT: systolic.matmul_tile
// STRICT-LABEL: func.func @may_alias(
//
// RUNTIME-LABEL: llvm.func @gemm16(
// RUNTIME-COUNT-10: llvm.call @fpga_matmul_tiled_auto_scheduled
// RUNTIME-LABEL: llvm.func @siblings(
// RUNTIME: llvm.call @fpga_matmul_tiled_auto_scheduled
module {
  // 16 x 16 x 16: ten invocations over the 1x8x8 + 3x4x4 fleet, the plan of
  // planSystolicMatmul, on the whole memrefs.
  // CERTIFIED-LABEL: func.func @gemm16(
  // CERTIFIED-NOT:   affine.for
  // CERTIFIED:       bufferization.to_tensor %arg1 restrict : memref<16x16xf32> to tensor<16x16xf32>
  // CERTIFIED:       bufferization.to_tensor %arg2 restrict : memref<16x16xf32> to tensor<16x16xf32>
  // CERTIFIED:       bufferization.to_tensor %arg0 restrict writable : memref<16x16xf32> to tensor<16x16xf32>
  // CERTIFIED-COUNT-10: systolic.matmul_tile
  // CERTIFIED-NOT:   systolic.matmul_tile
  // CERTIFIED:       bufferization.materialize_in_destination %{{.*}} in restrict writable %arg0
  // CERTIFIED-NEXT:  return
  func.func @gemm16(%C: memref<16x16xf32> {llvm.noalias}, %A: memref<16x16xf32> {llvm.noalias}, %B: memref<16x16xf32> {llvm.noalias}) {
    affine.for %i = 0 to 16 { affine.for %j = 0 to 16 { affine.for %k = 0 to 16 {
      %c = affine.load %C[%i, %j] : memref<16x16xf32>
      %a = affine.load %A[%i, %k] : memref<16x16xf32>
      %b = affine.load %B[%k, %j] : memref<16x16xf32>
      %p = arith.mulf %a, %b : f32
      %s = arith.addf %c, %p : f32
      affine.store %s, %C[%i, %j] : memref<16x16xf32>
    } } }
    func.return
  }

  // Two nests in one function with memory operations before, between and
  // after them: each is replaced in its own position, in order.
  // CERTIFIED-LABEL: func.func @siblings(
  // CERTIFIED:        memref.store %cst, %arg0[%c0, %c0]
  // CERTIFIED:        systolic.matmul_tile
  // CERTIFIED:        bufferization.materialize_in_destination %{{.*}} in restrict writable %arg0
  // CERTIFIED-NEXT:   memref.load %arg0[%c0, %c0]
  // CERTIFIED-NEXT:   memref.store {{.*}}, %arg3[%c0, %c0]
  // CERTIFIED:        systolic.matmul_tile
  // CERTIFIED:        bufferization.materialize_in_destination %{{.*}} in restrict writable %arg3
  // CERTIFIED-NEXT:   memref.store %cst, %arg3[%c0, %c0]
  // CERTIFIED-NEXT:   return
  func.func @siblings(%C: memref<4x4xf32> {llvm.noalias}, %A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %D: memref<4x4xf32> {llvm.noalias}) {
    %c0 = arith.constant 0 : index
    %one = arith.constant 1.0 : f32
    memref.store %one, %C[%c0, %c0] : memref<4x4xf32>
    affine.for %i = 0 to 4 { affine.for %j = 0 to 4 { affine.for %k = 0 to 4 {
      %c = affine.load %C[%i, %j] : memref<4x4xf32>
      %a = affine.load %A[%i, %k] : memref<4x4xf32>
      %b = affine.load %B[%k, %j] : memref<4x4xf32>
      %p = arith.mulf %a, %b : f32
      %s = arith.addf %c, %p : f32
      affine.store %s, %C[%i, %j] : memref<4x4xf32>
    } } }
    %mid = memref.load %C[%c0, %c0] : memref<4x4xf32>
    memref.store %mid, %D[%c0, %c0] : memref<4x4xf32>
    affine.for %i = 0 to 4 { affine.for %j = 0 to 4 { affine.for %k = 0 to 4 {
      %c = affine.load %D[%i, %j] : memref<4x4xf32>
      %a = affine.load %A[%i, %k] : memref<4x4xf32>
      %b = affine.load %B[%k, %j] : memref<4x4xf32>
      %p = arith.mulf %a, %b : f32
      %s = arith.addf %c, %p : f32
      affine.store %s, %D[%i, %j] : memref<4x4xf32>
    } } }
    memref.store %one, %D[%c0, %c0] : memref<4x4xf32>
    func.return
  }

  // A partial tile (M = 6 on the 8x8 array) and K = 40 folded by k_max = 16
  // into three invocations added into C in order, on static subviews of a
  // dynamic leading dimension (the C front end's float A[][40]).
  // CERTIFIED-LABEL: func.func @partial_k40(
  // CERTIFIED:        memref.subview %arg1[0, 0] [6, 40] [1, 1] : memref<?x40xf32> to memref<6x40xf32, strided<[40, 1]>>
  // CERTIFIED:        memref.subview %arg2[0, 0] [40, 8] [1, 1] : memref<?x8xf32> to memref<40x8xf32, strided<[8, 1]>>
  // CERTIFIED:        memref.subview %arg0[0, 0] [6, 8] [1, 1] : memref<?x8xf32> to memref<6x8xf32, strided<[8, 1]>>
  // CERTIFIED:        systolic.matmul_tile {{.*}} m = 6 n = 8 k = 16 on @acc_8x8_0 {{.*}}systolic.k_count = 3 : i64, systolic.k_index = 0 : i64
  // CERTIFIED:        systolic.matmul_tile {{.*}} m = 6 n = 8 k = 16 on @acc_8x8_0 {{.*}}systolic.k_count = 3 : i64, systolic.k_index = 1 : i64
  // CERTIFIED:        systolic.matmul_tile {{.*}} m = 6 n = 8 k = 8 on @acc_8x8_0 {{.*}}systolic.k_count = 3 : i64, systolic.k_index = 2 : i64
  // CERTIFIED-NOT:    systolic.matmul_tile
  // CERTIFIED:        return
  func.func @partial_k40(%C: memref<?x8xf32> {llvm.noalias}, %A: memref<?x40xf32> {llvm.noalias}, %B: memref<?x8xf32> {llvm.noalias}) {
    affine.for %i = 0 to 6 { affine.for %j = 0 to 8 { affine.for %k = 0 to 40 {
      %c = affine.load %C[%i, %j] : memref<?x8xf32>
      %a = affine.load %A[%i, %k] : memref<?x40xf32>
      %b = affine.load %B[%k, %j] : memref<?x8xf32>
      %p = arith.mulf %a, %b : f32
      %s = arith.addf %c, %p : f32
      affine.store %s, %C[%i, %j] : memref<?x8xf32>
    } } }
    func.return
  }

  // A triangular domain: not a box, UNKNOWN for the device jobs (and not
  // centrally symmetric for the fast path); the exact guarded MAC nest of
  // Phase 3b-5 takes it (its statement is strict).
  // CERTIFIED-LABEL: func.func @triangular(
  // CERTIFIED-NOT:    systolic.matmul_tile
  // CERTIFIED:        scf.if
  // CERTIFIED:        return
  func.func @triangular(%C: memref<4x4xf32> {llvm.noalias}, %A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}) {
    affine.for %i = 0 to 4 { affine.for %j = affine_map<(d0) -> (d0)>(%i) to 4 { affine.for %k = 0 to 4 {
      %c = affine.load %C[%i, %j] : memref<4x4xf32>
      %a = affine.load %A[%i, %k] : memref<4x4xf32>
      %b = affine.load %B[%k, %j] : memref<4x4xf32>
      %p = arith.mulf %a, %b : f32
      %s = arith.addf %c, %p : f32
      affine.store %s, %C[%i, %j] : memref<4x4xf32>
    } } }
    func.return
  }

  // A non-unimodular map (C[2i+j][j], det 2): UNKNOWN for the device jobs in
  // this phase; the fast path lowers it through its general view.
  // CERTIFIED-LABEL: func.func @stretch(
  // CERTIFIED-NOT:    scf.if
  // CERTIFIED:        systolic.matmul_tile
  // CERTIFIED:        return
  func.func @stretch(%C: memref<16x4xf32> {llvm.noalias}, %A: memref<16x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}) {
    affine.for %i = 0 to 4 { affine.for %j = 0 to 4 { affine.for %k = 0 to 4 {
      %c = affine.load %C[2 * %i + %j, %j] : memref<16x4xf32>
      %a = affine.load %A[2 * %i + %j, %k] : memref<16x4xf32>
      %b = affine.load %B[%k, %j] : memref<4x4xf32>
      %p = arith.mulf %a, %b : f32
      %s = arith.addf %c, %p : f32
      affine.store %s, %C[2 * %i + %j, %j] : memref<16x4xf32>
    } } }
    func.return
  }

  // Arguments that may alias: the shared preflight refuses before either
  // path; the nest stays as written.
  // CERTIFIED-LABEL: func.func @may_alias(
  // CERTIFIED:        affine.load %arg0
  // CERTIFIED:        affine.store
  func.func @may_alias(%C: memref<4x4xf32>, %A: memref<4x4xf32>, %B: memref<4x4xf32>) {
    affine.for %i = 0 to 4 { affine.for %j = 0 to 4 { affine.for %k = 0 to 4 {
      %c = affine.load %C[%i, %j] : memref<4x4xf32>
      %a = affine.load %A[%i, %k] : memref<4x4xf32>
      %b = affine.load %B[%k, %j] : memref<4x4xf32>
      %p = arith.mulf %a, %b : f32
      %s = arith.addf %c, %p : f32
      affine.store %s, %C[%i, %j] : memref<4x4xf32>
    } } }
    func.return
  }
}
