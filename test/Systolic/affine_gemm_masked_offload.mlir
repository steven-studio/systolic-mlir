// Phase 5.4.3d-3b-7: exact non-box domains through dense device jobs and an
// exact output mask (docs/AFFINE_GEMM_MAPPING.md section 28). A domain that
// factors as O x BK -- every output's reduction fiber empty or the whole K
// range -- runs as the dense GEMM of its box into a fresh zero temporary,
// whose selected elements are then added into C under the exact Presburger
// guard of Phase 2b. A partial reduction fiber is refused.
//
// RUN: systolic-opt --systolic-declare-fpga-fleet \
// RUN:   --affine-matmul-to-systolic="general-affine=true certified-offload-first=true allow-fp-reassociation=true report=true" \
// RUN:   %s 2>&1 | FileCheck %s --check-prefix=MASK
//
// In the pass's default order the fast path refuses these domains (not
// centrally symmetric) and the certified lowering, the guarded path's first
// decision, takes them: the same jobs without the test control.
// RUN: systolic-opt --systolic-declare-fpga-fleet \
// RUN:   --affine-matmul-to-systolic="general-affine=true allow-fp-reassociation=true" %s -o %t.default.mlir
// RUN: systolic-opt --systolic-declare-fpga-fleet \
// RUN:   --affine-matmul-to-systolic="general-affine=true certified-offload-first=true allow-fp-reassociation=true" %s -o %t.first.mlir
// RUN: diff %t.default.mlir %t.first.mlir
//
// Strict floating point: nothing is offloaded; the guarded MAC nests follow.
// RUN: systolic-opt --systolic-declare-fpga-fleet \
// RUN:   --affine-matmul-to-systolic="general-affine=true certified-offload-first=true report=true" \
// RUN:   %s 2>&1 | FileCheck %s --check-prefix=STRICT
//
// The asynchronous runtime waits for the GEMM before the mask reads the
// temporary.
// RUN: systolic-opt --systolic-declare-fpga-fleet \
// RUN:   --affine-matmul-to-systolic="general-affine=true certified-offload-first=true allow-fp-reassociation=true" \
// RUN:   --lower-systolic-matmul-tile-to-fpga=async=true %s | FileCheck %s --check-prefix=ASYNC
//
// MASK-DAG: remark: affine GEMM 4x4x4 lowered as 1 exact region(s) by the certified device-job lowering (docs/AFFINE_GEMM_MAPPING.md section 27) with an exact output mask over the non-box domain (section 28)
// MASK-DAG: remark: affine GEMM 6x8x40 lowered as 3 exact region(s) by the certified device-job lowering (docs/AFFINE_GEMM_MAPPING.md section 27) with an exact output mask over the non-box domain (section 28)
// MASK-DAG: remark: affine GEMM 8x8x8 lowered as 1 exact region(s) by the certified device-job lowering (docs/AFFINE_GEMM_MAPPING.md section 27) with an exact output mask over the non-box domain (section 28)
// MASK-DAG: remark: certified device offload not applied: UNKNOWN: the reduction fiber of an output coordinate is a proper part of the K range (a K bound depends on i or j)
// MASK-DAG: remark: affine GEMM replaced by its exact guarded MAC nest
//
// STRICT-DAG: remark: certified device offload not applied: UNKNOWN: strict floating-point semantics
// STRICT-DAG: remark: affine GEMM replaced by its exact guarded MAC nest
// STRICT-NOT: systolic.matmul_tile
// STRICT-LABEL: func.func @k_from_i(
//
// ASYNC-LABEL: func.func @upper(
// ASYNC:       %[[T:.*]] = memref.alloc() : memref<4x4xf32>
// ASYNC:       llvm.call @fpga_gemm_begin
// ASYNC:       llvm.call @fpga_tile_submit
// ASYNC:       llvm.call @fpga_gemm_wait
// ASYNC:       scf.if
// ASYNC:         affine.load %[[T]]
// ASYNC:         affine.load %arg0
// ASYNC:         affine.store {{.*}}, %arg0
module {
  // The upper triangle j >= i of a 4 x 4 output: the dense 4 x 4 x 4 GEMM
  // into a zero temporary, then the masked accumulation into C.
  // MASK-LABEL: func.func @upper(
  // MASK:       bufferization.to_tensor %arg1 restrict : memref<4x4xf32> to tensor<4x4xf32>
  // MASK:       bufferization.to_tensor %arg2 restrict : memref<4x4xf32> to tensor<4x4xf32>
  // MASK:       %[[T:.*]] = memref.alloc() : memref<4x4xf32>
  // MASK:       affine.for
  // MASK:         affine.for
  // MASK:           affine.store %cst, %[[T]]
  // MASK:       bufferization.to_tensor %[[T]] restrict writable : memref<4x4xf32> to tensor<4x4xf32>
  // MASK:       systolic.matmul_tile {{.*}} m = 4 n = 4 k = 4 on @acc_
  // MASK:       bufferization.materialize_in_destination %{{.*}} in restrict writable %[[T]]
  // MASK:       affine.for
  // MASK:         affine.for
  // MASK:           scf.if
  // MASK:             affine.load %[[T]]
  // MASK:             affine.load %arg0
  // MASK:             arith.addf
  // MASK:             affine.store {{.*}}, %arg0
  // MASK:       return
  func.func @upper(%C: memref<4x4xf32> {llvm.noalias}, %A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}) {
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

  // A partial tile (M = 6) and K = 40 folded by k_max: the dense box's plan
  // of three invocations, the mask around it.
  // MASK-LABEL: func.func @deep_partial(
  // MASK:       memref.alloc() : memref<6x8xf32>
  // MASK:       systolic.matmul_tile {{.*}} m = 6 n = 8 k = 16 on @acc_8x8_0 {{.*}}systolic.k_count = 3 : i64, systolic.k_index = 0 : i64
  // MASK:       systolic.matmul_tile {{.*}} m = 6 n = 8 k = 16 on @acc_8x8_0 {{.*}}systolic.k_count = 3 : i64, systolic.k_index = 1 : i64
  // MASK:       systolic.matmul_tile {{.*}} m = 6 n = 8 k = 8 on @acc_8x8_0 {{.*}}systolic.k_count = 3 : i64, systolic.k_index = 2 : i64
  // MASK:       scf.if
  // MASK:       return
  func.func @deep_partial(%C: memref<6x8xf32> {llvm.noalias}, %A: memref<6x40xf32> {llvm.noalias}, %B: memref<40x8xf32> {llvm.noalias}) {
    affine.for %i = 0 to 6 { affine.for %j = affine_map<(d0) -> (d0)>(%i) to 8 { affine.for %k = 0 to 40 {
      %c = affine.load %C[%i, %j] : memref<6x8xf32>
      %a = affine.load %A[%i, %k] : memref<6x40xf32>
      %b = affine.load %B[%k, %j] : memref<40x8xf32>
      %p = arith.mulf %a, %b : f32
      %s = arith.addf %c, %p : f32
      affine.store %s, %C[%i, %j] : memref<6x8xf32>
    } } }
    func.return
  }

  // A hexagon: the box with two corners cut by max / min bounds.
  // MASK-LABEL: func.func @hexagon(
  // MASK:       systolic.matmul_tile {{.*}} m = 8 n = 8 k = 8
  // MASK:       scf.if
  // MASK:       return
  func.func @hexagon(%C: memref<8x8xf32> {llvm.noalias}, %A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}) {
    affine.for %i = 0 to 8 { affine.for %j = max affine_map<(d0) -> (0, 3 - d0)>(%i) to min affine_map<(d0) -> (8, 12 - d0)>(%i) { affine.for %k = 0 to 8 {
      %c = affine.load %C[%i, %j] : memref<8x8xf32>
      %a = affine.load %A[%i, %k] : memref<8x8xf32>
      %b = affine.load %B[%k, %j] : memref<8x8xf32>
      %p = arith.mulf %a, %b : f32
      %s = arith.addf %c, %p : f32
      affine.store %s, %C[%i, %j] : memref<8x8xf32>
    } } }
    func.return
  }

  // Two masked nests in one function: two temporaries, two masks, in order
  // (the strict lower triangle's box is tightened to the rows and columns
  // it touches: 3 x 3).
  // MASK-LABEL: func.func @two_masked(
  // MASK:       memref.alloc() : memref<4x4xf32>
  // MASK:       systolic.matmul_tile
  // MASK:       scf.if
  // MASK:             affine.store {{.*}}, %arg0
  // MASK:       memref.alloc() : memref<3x3xf32>
  // MASK:       systolic.matmul_tile {{.*}} m = 3 n = 3 k = 4
  // MASK:       scf.if
  // MASK:             affine.store {{.*}}, %arg3
  // MASK:       return
  func.func @two_masked(%C: memref<4x4xf32> {llvm.noalias}, %A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %D: memref<4x4xf32> {llvm.noalias}) {
    affine.for %i = 0 to 4 { affine.for %j = affine_map<(d0) -> (d0)>(%i) to 4 { affine.for %k = 0 to 4 {
      %c = affine.load %C[%i, %j] : memref<4x4xf32>
      %a = affine.load %A[%i, %k] : memref<4x4xf32>
      %b = affine.load %B[%k, %j] : memref<4x4xf32>
      %p = arith.mulf %a, %b : f32
      %s = arith.addf %c, %p : f32
      affine.store %s, %C[%i, %j] : memref<4x4xf32>
    } } }
    affine.for %i = 0 to 4 { affine.for %j = 0 to affine_map<(d0) -> (d0)>(%i) { affine.for %k = 0 to 4 {
      %c = affine.load %D[%i, %j] : memref<4x4xf32>
      %a = affine.load %A[%i, %k] : memref<4x4xf32>
      %b = affine.load %B[%k, %j] : memref<4x4xf32>
      %p = arith.mulf %a, %b : f32
      %s = arith.addf %c, %p : f32
      affine.store %s, %D[%i, %j] : memref<4x4xf32>
    } } }
    func.return
  }

  // A reduction bound that depends on i: the fiber of (i, j) is [0, i], a
  // proper part of the K range; the mask would skip contributions, so the
  // domain is refused and the guarded MAC nest of Phase 3b-5 takes it.
  // MASK-LABEL: func.func @k_from_i(
  // MASK-NOT:   systolic.matmul_tile
  // MASK:       scf.if
  // MASK:       return
  func.func @k_from_i(%C: memref<4x4xf32> {llvm.noalias}, %A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}) {
    affine.for %i = 0 to 4 { affine.for %j = 0 to 4 { affine.for %k = 0 to affine_map<(d0) -> (d0 + 1)>(%i) {
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
