// Exact non-box domains larger than one tile (docs/AFFINE_GEMM_MAPPING.md sections 19.13,
// 19.13.4): a GEMM normalized from a domain in disguise, and A x A through two views (A . A^T)
// on one, partitioned over the declared fleet into many exact regions -- each region checked
// by partitionLogicalMatmul, which first re-proves the logical mapping a bijection. And the
// certificate itself: on deliberately wrong mappings it must refuse (perturb-certificate is a
// test option of the discovery pass), so a vacuous certificate would fail here. A . A^T is also
// taken through the tile lowering (TILE, TILEBUF): per-tile slab copies only.
//
// RUN: systolic-opt -split-input-file --systolic-declare-fpga-fleet \
// RUN:   "--affine-matmul-to-systolic=report=true allow-fp-reassociation=true classify=true" \
// RUN:   %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=REMARK
// RUN: systolic-opt -split-input-file --systolic-declare-fpga-fleet \
// RUN:   "--affine-matmul-to-systolic=allow-fp-reassociation=true" %s | FileCheck %s
// RUN: systolic-opt -split-input-file "--test-affine-matmul-discovery=perturb-certificate=true" \
// RUN:   %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=CERT
// RUN: systolic-opt -split-input-file --systolic-declare-fpga-fleet \
// RUN:   "--affine-matmul-to-systolic=allow-fp-reassociation=true" --lower-systolic-matmul-tile-to-fpga \
// RUN:   %s | FileCheck %s --check-prefix=TILE
// RUN: systolic-opt -split-input-file --systolic-declare-fpga-fleet \
// RUN:   "--affine-matmul-to-systolic=allow-fp-reassociation=true" --lower-systolic-matmul-tile-to-fpga \
// RUN:   %s | FileCheck %s --check-prefix=TILEBUF

// A 20 x 20 x 24 GEMM in disguise: j in [max(0, i - 30), 20) is [0, 20) (the max never
// binds), k in [i, i + 24) shifts with i; the exact set is a parallelotope, normalized and
// partitioned into 26 regions on the 4 accelerators.
// REMARK:      exact iteration set: the written bounds do not shift together
// REMARK:      affine GEMM 20x20x24 lowered as 26 exact region(s) on 4 accelerator(s)
// CHECK-LABEL: func.func @disguise_large
// CHECK-COUNT-26: systolic.matmul_tile
// CHECK-NOT:   systolic.matmul_tile
// CERT:        dense extents: M 20, N 20, K 24
// CERT-NEXT:   logical bijection: verified
// CERT-NEXT:   perturbed mapping (M + 1): not verified (the iteration domain has 9600 points and the logical box 10080: logical iterations would be missing)
// CERT-NEXT:   perturbed mapping (M - 1): not verified (the logical coordinates of the iteration domain leave the box of the GEMM)
// CERT-NEXT:   perturbed mapping (I's scale doubled): not verified (a logical coordinate is not integral on the iteration domain)
// CERT-NEXT:   perturbed mapping (loop 0's origin + 1): not verified (the iteration domain is not the box of its loops' ordinals)
// CERT-NEXT:   perturbed mapping (I's row of T zero): not verified (two points of the iteration domain have the same logical coordinates: logical iterations would be duplicated)
func.func @disguise_large(%A: memref<20x24xf32> {llvm.noalias}, %B: memref<24x20xf32> {llvm.noalias}, %C: memref<20x20xf32> {llvm.noalias}) {
  affine.for %i = 0 to 20 {
    affine.for %j = max affine_map<(d0) -> (0, d0 - 30)>(%i) to 20 {
      affine.for %k = affine_map<(d0) -> (d0)>(%i) to affine_map<(d0) -> (d0 + 24)>(%i) {
        %a = affine.load %A[%i, %k - %i] : memref<20x24xf32>
        %b = affine.load %B[%k - %i, %j] : memref<24x20xf32>
        %c = affine.load %C[%i, %j] : memref<20x20xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<20x20xf32>
      }
    }
  }
  return
}

// -----
// A . A^T, 16 x 16 x 16, on a domain in disguise (j in [max(0, i - 20), 16)): two read-only
// tensors of %A -- the memref and its transpose -- and no copy of A at this level; 10 regions,
// every tile reading both factors from them. (Further down, --systolic-runtime-to-llvm
// materializes the transposed tensor contiguously for each tile, as it does for any transposed
// operand -- a plain B[j][k] included: a cost of transposed views, not of A x A.)
// REMARK:      affine GEMM 16x16x16 lowered as 10 exact region(s) on 4 accelerator(s)
// CHECK-LABEL: func.func @axa_t_large
// CHECK-NEXT:    %[[A:.*]] = bufferization.to_tensor %arg0 restrict :
// CHECK-NEXT:    %[[SV:.*]] = memref.subview %arg0[0, 0] [16, 16] [1, 1]
// CHECK-NEXT:    %[[T:.*]] = memref.transpose %[[SV]] (d0, d1) -> (d1, d0)
// CHECK-NEXT:    %[[AT:.*]] = bufferization.to_tensor %[[T]] restrict :
// CHECK-NOT:     memref.copy
// CHECK-COUNT-10: tensor.extract_slice %[[AT]]
// CHECK-NOT:     memref.copy
// CHECK:         return
// After --lower-systolic-matmul-tile-to-fpga each of the 10 tiles copies its two slabs -- rows
// of A and columns of A^T, 8 x 16 / 16 x 8 or 4 x 16 / 16 x 4 -- and nothing else: no copy of
// the 16 x 16 matrix. The transposed tensor is converted to an identity-layout buffer once per
// tile (TILEBUF); that conversion is where --systolic-runtime-to-llvm materializes it (docs
// section 19.13.7).
// TILE-LABEL:    func.func @axa_t_large
// TILE:          bufferization.to_tensor %arg0 restrict :
// TILE:          bufferization.to_tensor %{{.*}} restrict : memref<16x16xf32, strided<[1, 16]>>
// TILE-COUNT-20: memref.copy {{.*}} : memref<{{(8x16|16x8|4x16|16x4)}}xf32, strided<[16, 1]{{.*}}>> to memref<{{(8x16|16x8|4x16|16x4)}}xf32>
// TILE-NOT:      memref.copy
// TILE:          return
// TILEBUF-LABEL:    func.func @axa_t_large
// TILEBUF:          %[[AT:.*]] = bufferization.to_tensor %{{.*}} restrict : memref<16x16xf32, strided<[1, 16]>>
// TILEBUF-COUNT-10: bufferization.to_buffer %[[AT]] read_only : tensor<16x16xf32> to memref<16x16xf32>
// TILEBUF-NOT:      bufferization.to_buffer %[[AT]]
// TILEBUF:          return
// CERT:        dense extents: M 16, N 16, K 16
// CERT-NEXT:   logical bijection: verified
// CERT-NEXT:   perturbed mapping (M + 1): not verified ({{.*}}logical iterations would be missing)
// CERT-NEXT:   perturbed mapping (M - 1): not verified (the logical coordinates of the iteration domain leave the box of the GEMM)
// CERT-NEXT:   perturbed mapping (I's scale doubled): not verified (a logical coordinate is not integral on the iteration domain)
// CERT-NEXT:   perturbed mapping (loop 0's origin + 1): not verified (the iteration domain is not the box of its loops' ordinals)
// CERT-NEXT:   perturbed mapping (I's row of T zero): not verified ({{.*}}logical iterations would be duplicated)
func.func @axa_t_large(%A: memref<16x16xf32> {llvm.noalias}, %C: memref<16x16xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = max affine_map<(d0) -> (0, d0 - 20)>(%i) to 16 {
      affine.for %k = 0 to 16 {
        %a = affine.load %A[%i, %k] : memref<16x16xf32>
        %b = affine.load %A[%j, %k] : memref<16x16xf32>
        %c = affine.load %C[%i, %j] : memref<16x16xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<16x16xf32>
      }
    }
  }
  return
}

// -----
// det T = 2 (C's row index 2i) on a stepped, shifted domain: the certificate's integrality
// and injectivity tests see the scale and the step; the wrong mappings are refused too.
// REMARK:      affine GEMM 4x4x4 lowered as {{[0-9]+}} exact region(s)
// CHECK-LABEL: func.func @det2_stepped
// CHECK:       systolic.matmul_tile
// CERT:        dense extents: M 4, N 4, K 4
// CERT-NEXT:   logical bijection: verified
// CERT-NEXT:   perturbed mapping (M + 1): not verified ({{.*}}logical iterations would be missing)
// CERT-NEXT:   perturbed mapping (M - 1): not verified (the logical coordinates of the iteration domain leave the box of the GEMM)
// CERT-NEXT:   perturbed mapping (I's scale doubled): not verified (a logical coordinate is not integral on the iteration domain)
// CERT-NEXT:   perturbed mapping (loop 0's origin + 1): not verified (the iteration domain is not the box of its loops' ordinals)
// CERT-NEXT:   perturbed mapping (I's row of T zero): not verified ({{.*}}logical iterations would be duplicated)
func.func @det2_stepped(%A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = affine_map<(d0) -> (d0 * 2)>(%i) to affine_map<(d0) -> (d0 * 2 + 8)>(%i) step 2 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %k] : memref<4x4xf32>
        %b = affine.load %B[%k, %j - %i * 2] : memref<4x8xf32>
        %c = affine.load %C[%i * 2, %j - %i * 2] : memref<8x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i * 2, %j - %i * 2] : memref<8x8xf32>
      }
    }
  }
  return
}
