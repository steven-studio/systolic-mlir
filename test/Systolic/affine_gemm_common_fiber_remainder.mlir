// Phase 5.4.3d-3b-8 M2b-1 (docs/AFFINE_GEMM_MAPPING.md section 29.8): the
// host remainder of each output block is emitted in pieces -- below and above
// the block's common reduction fiber, or the whole block without one -- and
// each piece's guarded nest sweeps the exact bounding box of the piece's
// integer points instead of the block times the whole K range. The guard is
// unchanged (the piece itself); only the loop bounds are tighter. Checked
// here: the bounds of every nest, in emission order, and the nest counts.
//
// RUN: systolic-opt --systolic-declare-fpga-fleet \
// RUN:   --affine-matmul-to-systolic="general-affine=true allow-fp-reassociation=true common-fiber-offload=true report=true" \
// RUN:   %s 2>&1 >/dev/null | FileCheck %s --check-prefix=REMARK
// RUN: systolic-opt --systolic-declare-fpga-fleet \
// RUN:   --affine-matmul-to-systolic="general-affine=true allow-fp-reassociation=true common-fiber-offload=true" \
// RUN:   %s | FileCheck %s --check-prefix=TIGHT
//
// The fleet's plan for a 16 x 16 output: two 8 x 8 blocks (rows 0-7) and
// eight 4 x 4 blocks (rows 8-15). k <= i: rows [r0, r1] share the fiber
// [0, r0]; the one piece above it is i, k in [r0 + 1, r1] with k <= i over
// the block's columns: a 7 x 8 x 7 or a 3 x 4 x 3 box.
// REMARK-DAG: remark: affine GEMM 16x16x16 lowered as 10 exact region(s) by the certified device-job lowering (docs/AFFINE_GEMM_MAPPING.md section 27) with the common reduction fibers of 10 output block(s) on the device (1536 of 2176 MACs) and the remainder on the host (640 MACs in 10 guarded nest(s)) (section 29)
// k >= j: columns [c0, c1] share the fiber [c1, 15]; the piece below it is
// j, k in [c0, c1 - 1] with k >= j over the block's rows.
// REMARK-DAG: remark: affine GEMM 16x16x16 lowered as 10 exact region(s) by the certified device-job lowering (docs/AFFINE_GEMM_MAPPING.md section 27) with the common reduction fibers of 10 output block(s) on the device (1536 of 2176 MACs) and the remainder on the host (640 MACs in 10 guarded nest(s)) (section 29)
// j <= k <= i: the two blocks on the diagonal have no common fiber (whole
// block, one nest each), the three 4 x 4 blocks below the 8 x 8 ones and
// the three below the diagonal among the 4 x 4 ones have both pieces (two
// nests each, 12 in all... see below), blocks above the diagonal nothing.
// REMARK-DAG: remark: affine GEMM 16x16x16 lowered as 5 exact region(s) by the certified device-job lowering (docs/AFFINE_GEMM_MAPPING.md section 27) with the common reduction fibers of 5 output block(s) on the device (416 of 816 MACs) and the remainder on the host (400 MACs in 13 guarded nest(s)) (section 29)
// k <= i + j, k < 16: every block one piece above its fiber [0, r0 + c0].
// REMARK-DAG: remark: affine GEMM 16x16x16 lowered as 10 exact region(s) by the certified device-job lowering (docs/AFFINE_GEMM_MAPPING.md section 27) with the common reduction fibers of 10 output block(s) on the device (2480 of 3416 MACs) and the remainder on the host (936 MACs in 5 guarded nest(s)) (section 29)
// Shifted: i in [1, 3], j in [0, 2], k in [1, i]: the fiber is {1}, the
// piece above it i, k in [2, 3].
// REMARK-DAG: remark: affine GEMM 3x3x3 lowered as 1 exact region(s) by the certified device-job lowering (docs/AFFINE_GEMM_MAPPING.md section 27) with the common reduction fibers of 1 output block(s) on the device (9 of 18 MACs) and the remainder on the host (9 MACs in 1 guarded nest(s)) (section 29)
module {
  // TIGHT-LABEL: func.func @k_le_i(
  // TIGHT:       bufferization.materialize_in_destination
  // Block rows 0-7, columns 0-7: fiber {0}; the piece i, k in [1, 7].
  // TIGHT:       affine.for %{{.*}} = 1 to 8 {
  // TIGHT-NEXT:    affine.for %{{.*}} = 0 to 8 {
  // TIGHT-NEXT:      affine.for %{{.*}} = 1 to 8 {
  // TIGHT:             scf.if
  // Block rows 0-7, columns 8-15.
  // TIGHT:       affine.for %{{.*}} = 1 to 8 {
  // TIGHT-NEXT:    affine.for %{{.*}} = 8 to 16 {
  // TIGHT-NEXT:      affine.for %{{.*}} = 1 to 8 {
  // TIGHT:             scf.if
  // Block rows 8-11, columns 0-3: fiber [0, 8]; the piece i, k in [9, 11].
  // TIGHT:       affine.for %{{.*}} = 9 to 12 {
  // TIGHT-NEXT:    affine.for %{{.*}} = 0 to 4 {
  // TIGHT-NEXT:      affine.for %{{.*}} = 9 to 12 {
  // TIGHT:             scf.if
  // TIGHT:       affine.for %{{.*}} = 9 to 12 {
  // TIGHT-NEXT:    affine.for %{{.*}} = 4 to 8 {
  // TIGHT-NEXT:      affine.for %{{.*}} = 9 to 12 {
  // TIGHT:       affine.for %{{.*}} = 9 to 12 {
  // TIGHT-NEXT:    affine.for %{{.*}} = 8 to 12 {
  // TIGHT-NEXT:      affine.for %{{.*}} = 9 to 12 {
  // TIGHT:       affine.for %{{.*}} = 9 to 12 {
  // TIGHT-NEXT:    affine.for %{{.*}} = 12 to 16 {
  // TIGHT-NEXT:      affine.for %{{.*}} = 9 to 12 {
  // Block rows 12-15: fiber [0, 12]; the piece i, k in [13, 15].
  // TIGHT:       affine.for %{{.*}} = 13 to 16 {
  // TIGHT-NEXT:    affine.for %{{.*}} = 0 to 4 {
  // TIGHT-NEXT:      affine.for %{{.*}} = 13 to 16 {
  // TIGHT:       affine.for %{{.*}} = 13 to 16 {
  // TIGHT-NEXT:    affine.for %{{.*}} = 4 to 8 {
  // TIGHT-NEXT:      affine.for %{{.*}} = 13 to 16 {
  // TIGHT:       affine.for %{{.*}} = 13 to 16 {
  // TIGHT-NEXT:    affine.for %{{.*}} = 8 to 12 {
  // TIGHT-NEXT:      affine.for %{{.*}} = 13 to 16 {
  // TIGHT:       affine.for %{{.*}} = 13 to 16 {
  // TIGHT-NEXT:    affine.for %{{.*}} = 12 to 16 {
  // TIGHT-NEXT:      affine.for %{{.*}} = 13 to 16 {
  // TIGHT:             scf.if
  // TIGHT-NOT:   affine.for
  // TIGHT:       return
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

  // TIGHT-LABEL: func.func @k_ge_j(
  // TIGHT:       bufferization.materialize_in_destination
  // Block rows 0-7, columns 0-7: fiber [7, 15]; the piece j, k in [0, 6].
  // TIGHT:       affine.for %{{.*}} = 0 to 8 {
  // TIGHT-NEXT:    affine.for %{{.*}} = 0 to 7 {
  // TIGHT-NEXT:      affine.for %{{.*}} = 0 to 7 {
  // TIGHT:             scf.if
  // Block rows 0-7, columns 8-15: fiber [15, 15]; the piece j, k in [8, 14].
  // TIGHT:       affine.for %{{.*}} = 0 to 8 {
  // TIGHT-NEXT:    affine.for %{{.*}} = 8 to 15 {
  // TIGHT-NEXT:      affine.for %{{.*}} = 8 to 15 {
  // TIGHT:             scf.if
  // Block rows 8-11, columns 0-3: fiber [3, 15]; the piece j, k in [0, 2].
  // TIGHT:       affine.for %{{.*}} = 8 to 12 {
  // TIGHT-NEXT:    affine.for %{{.*}} = 0 to 3 {
  // TIGHT-NEXT:      affine.for %{{.*}} = 0 to 3 {
  // TIGHT:             scf.if
  // TIGHT:       return
  func.func @k_ge_j(%C: memref<16x16xf32> {llvm.noalias}, %A: memref<16x16xf32> {llvm.noalias}, %B: memref<16x16xf32> {llvm.noalias}) {
    affine.for %i = 0 to 16 { affine.for %j = 0 to 16 { affine.for %k = affine_map<(d0) -> (d0)>(%j) to 16 {
      %c = affine.load %C[%i, %j] : memref<16x16xf32>
      %a = affine.load %A[%i, %k] : memref<16x16xf32>
      %b = affine.load %B[%k, %j] : memref<16x16xf32>
      %p = arith.mulf %a, %b : f32
      %s = arith.addf %c, %p : f32
      affine.store %s, %C[%i, %j] : memref<16x16xf32>
    } } }
    func.return
  }

  // TIGHT-LABEL: func.func @between(
  // TIGHT:       bufferization.materialize_in_destination
  // Block rows 0-7, columns 0-7: on the diagonal, no common fiber: the
  // whole block, j <= k <= i, whose bounding box is the block's cube.
  // TIGHT:       affine.for %{{.*}} = 0 to 8 {
  // TIGHT-NEXT:    affine.for %{{.*}} = 0 to 8 {
  // TIGHT-NEXT:      affine.for %{{.*}} = 0 to 8 {
  // TIGHT:             scf.if
  // Block rows 8-11, columns 0-3: fiber [3, 8]; below it j, k in [0, 2]
  // over the block's rows; above it i, k in [9, 11] over its columns.
  // TIGHT:       affine.for %{{.*}} = 8 to 12 {
  // TIGHT-NEXT:    affine.for %{{.*}} = 0 to 3 {
  // TIGHT-NEXT:      affine.for %{{.*}} = 0 to 3 {
  // TIGHT:             scf.if
  // TIGHT:       affine.for %{{.*}} = 9 to 12 {
  // TIGHT-NEXT:    affine.for %{{.*}} = 0 to 4 {
  // TIGHT-NEXT:      affine.for %{{.*}} = 9 to 12 {
  // TIGHT:             scf.if
  // Block rows 8-11, columns 4-7: fiber [7, 8]; both pieces again.
  // TIGHT:       affine.for %{{.*}} = 8 to 12 {
  // TIGHT-NEXT:    affine.for %{{.*}} = 4 to 7 {
  // TIGHT-NEXT:      affine.for %{{.*}} = 4 to 7 {
  // TIGHT:       affine.for %{{.*}} = 9 to 12 {
  // TIGHT-NEXT:    affine.for %{{.*}} = 4 to 8 {
  // TIGHT-NEXT:      affine.for %{{.*}} = 9 to 12 {
  // Block rows 8-11, columns 8-11: on the diagonal, the whole block.
  // TIGHT:       affine.for %{{.*}} = 8 to 12 {
  // TIGHT-NEXT:    affine.for %{{.*}} = 8 to 12 {
  // TIGHT-NEXT:      affine.for %{{.*}} = 8 to 12 {
  // Block rows 12-15, columns 0-3: fiber [3, 12]; both pieces.
  // TIGHT:       affine.for %{{.*}} = 12 to 16 {
  // TIGHT-NEXT:    affine.for %{{.*}} = 0 to 3 {
  // TIGHT-NEXT:      affine.for %{{.*}} = 0 to 3 {
  // TIGHT:       affine.for %{{.*}} = 13 to 16 {
  // TIGHT-NEXT:    affine.for %{{.*}} = 0 to 4 {
  // TIGHT-NEXT:      affine.for %{{.*}} = 13 to 16 {
  // TIGHT:       affine.for %{{.*}} = 12 to 16 {
  // TIGHT-NEXT:    affine.for %{{.*}} = 4 to 7 {
  // TIGHT-NEXT:      affine.for %{{.*}} = 4 to 7 {
  // TIGHT:       affine.for %{{.*}} = 13 to 16 {
  // TIGHT-NEXT:    affine.for %{{.*}} = 4 to 8 {
  // TIGHT-NEXT:      affine.for %{{.*}} = 13 to 16 {
  // Block rows 12-15, columns 8-11: fiber [11, 12]; both pieces.
  // TIGHT:       affine.for %{{.*}} = 12 to 16 {
  // TIGHT-NEXT:    affine.for %{{.*}} = 8 to 11 {
  // TIGHT-NEXT:      affine.for %{{.*}} = 8 to 11 {
  // TIGHT:       affine.for %{{.*}} = 13 to 16 {
  // TIGHT-NEXT:    affine.for %{{.*}} = 8 to 12 {
  // TIGHT-NEXT:      affine.for %{{.*}} = 13 to 16 {
  // Block rows 12-15, columns 12-15: on the diagonal, the whole block.
  // TIGHT:       affine.for %{{.*}} = 12 to 16 {
  // TIGHT-NEXT:    affine.for %{{.*}} = 12 to 16 {
  // TIGHT-NEXT:      affine.for %{{.*}} = 12 to 16 {
  // TIGHT:             scf.if
  // TIGHT-NOT:   affine.for
  // TIGHT:       return
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

  // TIGHT-LABEL: func.func @k_le_i_plus_j(
  // TIGHT:       bufferization.materialize_in_destination
  // Block rows 0-7, columns 0-7: fiber {0}; the piece k in [1, 14] (k <=
  // i + j <= 14), i and j the block's.
  // TIGHT:       affine.for %{{.*}} = 0 to 8 {
  // TIGHT-NEXT:    affine.for %{{.*}} = 0 to 8 {
  // TIGHT-NEXT:      affine.for %{{.*}} = 1 to 15 {
  // TIGHT:             scf.if
  // Block rows 0-7, columns 8-15: fiber [0, 8]; the piece k in [9, 15].
  // TIGHT:       affine.for %{{.*}} = 0 to 8 {
  // TIGHT-NEXT:    affine.for %{{.*}} = 8 to 16 {
  // TIGHT-NEXT:      affine.for %{{.*}} = 9 to 16 {
  // Block rows 8-11, columns 0-3: fiber [0, 8]; the piece k in [9, 14].
  // TIGHT:       affine.for %{{.*}} = 8 to 12 {
  // TIGHT-NEXT:    affine.for %{{.*}} = 0 to 4 {
  // TIGHT-NEXT:      affine.for %{{.*}} = 9 to 15 {
  // Block rows 8-11, columns 4-7: fiber [0, 12]; the piece k in [13, 15].
  // TIGHT:       affine.for %{{.*}} = 8 to 12 {
  // TIGHT-NEXT:    affine.for %{{.*}} = 4 to 8 {
  // TIGHT-NEXT:      affine.for %{{.*}} = 13 to 16 {
  // Block rows 12-15, columns 0-3: fiber [0, 12]; the piece k in [13, 15].
  // The other five blocks have fibers covering the whole K range: no nest.
  // TIGHT:       affine.for %{{.*}} = 12 to 16 {
  // TIGHT-NEXT:    affine.for %{{.*}} = 0 to 4 {
  // TIGHT-NEXT:      affine.for %{{.*}} = 13 to 16 {
  // TIGHT:             scf.if
  // TIGHT-NOT:   affine.for
  // TIGHT:       return
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

  // TIGHT-LABEL: func.func @shifted(
  // TIGHT:       bufferization.materialize_in_destination
  // TIGHT:       affine.for %{{.*}} = 2 to 4 {
  // TIGHT-NEXT:    affine.for %{{.*}} = 0 to 3 {
  // TIGHT-NEXT:      affine.for %{{.*}} = 2 to 4 {
  // TIGHT:             scf.if
  // TIGHT-NOT:   affine.for
  // TIGHT:       return
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
}
