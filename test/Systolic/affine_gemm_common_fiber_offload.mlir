// Phase 5.4.3d-3b-8 M1 (docs/AFFINE_GEMM_MAPPING.md section 29): reduction
// fibers that vary with the output coordinate. Under the hidden option
// common-fiber-offload (off by default: the Phase 3b-7 refusal stands until
// the dispatch-order change is approved) each output block of the box's tile
// plan runs its common reduction fiber on the device -- the plan's folds
// clipped to that interval -- accumulating directly into C, and the
// remaining contributions run on the host as guarded MAC nests after the
// device write-back. The fast path refuses these domains (not centrally
// symmetric), so the pass's default order reaches the certified lowering.
//
// RUN: systolic-opt --systolic-declare-fpga-fleet \
// RUN:   --affine-matmul-to-systolic="general-affine=true allow-fp-reassociation=true common-fiber-offload=true report=true" \
// RUN:   %s 2>&1 >/dev/null | FileCheck %s --check-prefix=REMARK
// RUN: systolic-opt --systolic-declare-fpga-fleet \
// RUN:   --affine-matmul-to-systolic="general-affine=true allow-fp-reassociation=true common-fiber-offload=true" \
// RUN:   %s | FileCheck %s --check-prefix=HYBRID
//
// Without the option: the established refusal, the guarded MAC nests follow.
// RUN: systolic-opt --systolic-declare-fpga-fleet \
// RUN:   --affine-matmul-to-systolic="general-affine=true allow-fp-reassociation=true report=true" \
// RUN:   %s 2>&1 >/dev/null | FileCheck %s --check-prefix=OFF
// RUN: systolic-opt --systolic-declare-fpga-fleet \
// RUN:   --affine-matmul-to-systolic="general-affine=true allow-fp-reassociation=true" \
// RUN:   %s | FileCheck %s --check-prefix=OFFIR
//
// Strict floating point: nothing is offloaded.
// RUN: systolic-opt --systolic-declare-fpga-fleet \
// RUN:   --affine-matmul-to-systolic="general-affine=true common-fiber-offload=true report=true" \
// RUN:   %s 2>&1 >/dev/null | FileCheck %s --check-prefix=STRICT
//
// The asynchronous runtime waits for the device before the host remainder.
// RUN: systolic-opt --systolic-declare-fpga-fleet \
// RUN:   --affine-matmul-to-systolic="general-affine=true allow-fp-reassociation=true common-fiber-offload=true" \
// RUN:   --lower-systolic-matmul-tile-to-fpga=async=true %s | FileCheck %s --check-prefix=ASYNC
//
// REMARK-DAG: remark: affine GEMM 16x16x16 lowered as {{[0-9]+}} exact region(s) by the certified device-job lowering (docs/AFFINE_GEMM_MAPPING.md section 27) with the common reduction fibers of 10 output block(s) on the device ({{[0-9]+}} of 2176 MACs) and the remainder on the host ({{[0-9]+}} MACs in {{[0-9]+}} guarded nest(s)) (section 29)
// REMARK-DAG: remark: affine GEMM 16x16x16 lowered as {{[0-9]+}} exact region(s) by the certified device-job lowering (docs/AFFINE_GEMM_MAPPING.md section 27) with the common reduction fibers of {{[0-9]+}} output block(s) on the device ({{[0-9]+}} of 3416 MACs) and the remainder on the host
// REMARK-DAG: remark: affine GEMM 16x16x16 lowered as {{[0-9]+}} exact region(s) by the certified device-job lowering (docs/AFFINE_GEMM_MAPPING.md section 27) with the common reduction fibers of {{[0-9]+}} output block(s) on the device ({{[0-9]+}} of 816 MACs) and the remainder on the host
// REMARK-DAG: remark: affine GEMM 3x3x3 lowered as {{[0-9]+}} exact region(s) by the certified device-job lowering (docs/AFFINE_GEMM_MAPPING.md section 27) with the common reduction fibers of {{[0-9]+}} output block(s) on the device ({{[0-9]+}} of 18 MACs) and the remainder on the host
// REMARK-DAG: remark: certified device offload not applied: UNKNOWN: no output block has a common reduction fiber: nothing for the device
// REMARK-DAG: remark: affine GEMM replaced by its exact guarded MAC nest
//
// OFF-DAG: remark: certified device offload not applied: UNKNOWN: the reduction fiber of an output coordinate is a proper part of the K range
// OFF-DAG: remark: affine GEMM replaced by its exact guarded MAC nest
// OFF-NOT: (section 29)
// OFFIR-LABEL: func.func @k_le_i(
// OFFIR-NOT:   systolic.matmul_tile
// OFFIR:       scf.if
// OFFIR-LABEL: func.func @k_le_i_plus_j(
// OFFIR-NOT:   systolic.matmul_tile
// OFFIR:       scf.if
//
// STRICT-DAG: remark: certified device offload not applied: UNKNOWN: strict floating-point semantics
// STRICT-DAG: remark: affine GEMM replaced by its exact guarded MAC nest
// STRICT-NOT: (section 29)
//
// ASYNC-LABEL: func.func @k_le_i(
// ASYNC:       llvm.call @fpga_gemm_begin
// ASYNC:       llvm.call @fpga_tile_submit
// ASYNC:       llvm.call @fpga_gemm_wait
// ASYNC:       scf.if
// ASYNC:         memref.load %arg0
// ASYNC:         memref.store {{.*}}, %arg0
// ASYNC-LABEL: func.func @between(
// ASYNC:       llvm.call @fpga_gemm_begin
// ASYNC:       llvm.call @fpga_tile_submit
// ASYNC:       llvm.call @fpga_gemm_wait
// ASYNC-NOT:   llvm.call @fpga_tile_submit
// ASYNC:       scf.if
// ASYNC:         memref.load %arg0
// ASYNC:         memref.store {{.*}}, %arg0
module {
  // Case A: 0 <= k <= i on 16 x 16 x 16: the plan's ten blocks each run K
  // [0, min i of the block] (one fold, clipped), accumulating into C; the
  // remainder nests follow the write-back.
  // HYBRID-LABEL: func.func @k_le_i(
  // HYBRID:       bufferization.to_tensor %arg1 restrict : memref<16x16xf32>
  // HYBRID:       bufferization.to_tensor %arg2 restrict : memref<16x16xf32>
  // HYBRID:       bufferization.to_tensor %arg0 restrict writable : memref<16x16xf32>
  // HYBRID:       systolic.matmul_tile {{.*}} m = 8 n = 8 k = 1 on @acc_8x8_0
  // HYBRID:       systolic.matmul_tile {{.*}} m = 8 n = 8 k = 1 on @acc_8x8_0
  // HYBRID:       systolic.matmul_tile {{.*}} m = 4 n = 4 k = 9 on @acc_4x4_0
  // HYBRID:       bufferization.materialize_in_destination %{{.*}} in restrict writable %arg0
  // HYBRID:       affine.for
  // HYBRID:         scf.if
  // HYBRID:           memref.load %arg1
  // HYBRID:           memref.load %arg2
  // HYBRID:           memref.load %arg0
  // HYBRID:           arith.addf
  // HYBRID:           memref.store {{.*}}, %arg0
  // HYBRID-NOT:   memref.alloc
  // HYBRID:       return
  func.func @k_le_i(%C: memref<16x16xf32> {llvm.noalias}, %A: memref<16x16xf32> {llvm.noalias}, %B: memref<16x16xf32> {llvm.noalias}) {
    affine.for %i = 0 to 16 { affine.for %j = 0 to 16 { affine.for %k = 0 to affine_map<(d0) -> (d0 + 1)>(%i) {
      %c = affine.load %C[%i, %j] : memref<16x16xf32>
      %a = affine.load %A[%i, %k] : memref<16x16xf32>
      %b = affine.load %B[%k, %j] : memref<16x16xf32>
      %p = arith.mulf %a, %b : f32
      %s = arith.addf %c, %p : f32
      affine.store %s, %C[%i, %j] : memref<16x16xf32>
    } } }
    func.return
  }

  // Case B: 0 <= k <= i + j, k < 16.
  // HYBRID-LABEL: func.func @k_le_i_plus_j(
  // HYBRID:       systolic.matmul_tile
  // HYBRID:       bufferization.materialize_in_destination %{{.*}} in restrict writable %arg0
  // HYBRID:       scf.if
  // HYBRID:       return
  func.func @k_le_i_plus_j(%C: memref<16x16xf32> {llvm.noalias}, %A: memref<16x16xf32> {llvm.noalias}, %B: memref<16x16xf32> {llvm.noalias}) {
    affine.for %i = 0 to 16 { affine.for %j = 0 to 16 { affine.for %k = 0 to min affine_map<(d0, d1) -> (16, d0 + d1 + 1)>(%i, %j) {
      %c = affine.load %C[%i, %j] : memref<16x16xf32>
      %a = affine.load %A[%i, %k] : memref<16x16xf32>
      %b = affine.load %B[%k, %j] : memref<16x16xf32>
      %p = arith.mulf %a, %b : f32
      %s = arith.addf %c, %p : f32
      affine.store %s, %C[%i, %j] : memref<16x16xf32>
    } } }
    func.return
  }

  // Case D: j <= k <= i on 16 x 16 x 16: the blocks on the diagonal and
  // above have a cell with an empty fiber (no common fiber, host); the
  // blocks below run K [max j, min i].
  // HYBRID-LABEL: func.func @between(
  // HYBRID:       systolic.matmul_tile
  // HYBRID:       scf.if
  // HYBRID:       return
  func.func @between(%C: memref<16x16xf32> {llvm.noalias}, %A: memref<16x16xf32> {llvm.noalias}, %B: memref<16x16xf32> {llvm.noalias}) {
    affine.for %i = 0 to 16 { affine.for %j = 0 to 16 { affine.for %k = affine_map<(d0) -> (d0)>(%j) to affine_map<(d0) -> (d0 + 1)>(%i) {
      %c = affine.load %C[%i, %j] : memref<16x16xf32>
      %a = affine.load %A[%i, %k] : memref<16x16xf32>
      %b = affine.load %B[%k, %j] : memref<16x16xf32>
      %p = arith.mulf %a, %b : f32
      %s = arith.addf %c, %p : f32
      affine.store %s, %C[%i, %j] : memref<16x16xf32>
    } } }
    func.return
  }

  // Nonzero lower bounds and shifted coordinates: i in [1, 4), j in [0, 3),
  // k in [1, i], C[i + 1][j + 2]: 18 MACs, the box 3 x 3 x 3.
  // HYBRID-LABEL: func.func @shifted(
  // HYBRID:       memref.subview %arg1[2, 1] [3, 3] [1, 1]
  // HYBRID:       memref.subview %arg2[1, 2] [3, 3] [1, 1]
  // HYBRID:       memref.subview %arg0[2, 2] [3, 3] [1, 1]
  // HYBRID:       systolic.matmul_tile {{.*}} m = 3 n = 3 k = 1
  // HYBRID:       scf.if
  // HYBRID:       return
  func.func @shifted(%C: memref<8x8xf32> {llvm.noalias}, %A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}) {
    affine.for %i = 1 to 4 { affine.for %j = 0 to 3 { affine.for %k = 1 to affine_map<(d0) -> (d0 + 1)>(%i) {
      %c = affine.load %C[%i + 1, %j + 2] : memref<8x8xf32>
      %a = affine.load %A[%i + 1, %k] : memref<8x8xf32>
      %b = affine.load %B[%k, %j + 2] : memref<8x8xf32>
      %p = arith.mulf %a, %b : f32
      %s = arith.addf %c, %p : f32
      affine.store %s, %C[%i + 1, %j + 2] : memref<8x8xf32>
    } } }
    func.return
  }

  // Case D on a 4 x 4 output: the plan's single block holds the cell (0, 3)
  // with an empty fiber, so there is no common fiber at all: UNKNOWN, and
  // the guarded MAC nest of Phase 3b-5 takes the nest.
  // HYBRID-LABEL: func.func @empty_common(
  // HYBRID-NOT:   systolic.matmul_tile
  // HYBRID:       scf.if
  // HYBRID:       return
  func.func @empty_common(%C: memref<4x4xf32> {llvm.noalias}, %A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}) {
    affine.for %i = 0 to 4 { affine.for %j = 0 to 4 { affine.for %k = affine_map<(d0) -> (d0)>(%j) to affine_map<(d0) -> (d0 + 1)>(%i) {
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
