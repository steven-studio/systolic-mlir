// RUN: systolic-opt --systolic-declare-fpga-fleet --test-affine-matmul-discovery %s -o /dev/null 2>&1 \
// RUN:   | FileCheck %s --check-prefix=FLEET
// RUN: systolic-opt --test-affine-matmul-discovery="regions=0,0,0,16,16,8,0,0,8,16,16,8" %s -o /dev/null 2>&1 \
// RUN:   | FileCheck %s --check-prefix=SPLITK
// RUN: systolic-opt --test-affine-matmul-discovery="regions=0,0,0,16,16,9,0,0,8,16,16,8" %s -o /dev/null 2>&1 \
// RUN:   | FileCheck %s --check-prefix=OVERLAP
// RUN: systolic-opt --test-affine-matmul-discovery="regions=0,0,0,8,16,16,9,0,0,7,16,16" %s -o /dev/null 2>&1 \
// RUN:   | FileCheck %s --check-prefix=GAP
// RUN: systolic-opt --test-affine-matmul-discovery="regions=0,0,0,16,16,17" %s -o /dev/null 2>&1 \
// RUN:   | FileCheck %s --check-prefix=OUTSIDE

// Affine Partitioning of a 16x16x16 GEMM (loops k, i, j).
//
// With the fleet: the regions the 1x8x8 + 3x4x4 binary-search plan induces,
// in logical (I, J, K) ordinals, each with its accelerator, and the proof
// that they partition the iteration domain exactly: every point in one
// region (ordinal-space cell count) and the regions' source-space domains
// covering the loop nest's own domain with no shared point (Presburger).
//
// FLEET:      dense logical GEMM: yes, I/J/K on loops 1/2/0
// FLEET:      fleet partition: 10 regions, exact
// FLEET-NEXT: region[0] I[0, 8) J[0, 8) K[0, 16) on @acc_8x8_0
// FLEET-NEXT: region[1] I[0, 8) J[8, 16) K[0, 16) on @acc_8x8_0
// FLEET-NEXT: region[2] I[8, 12) J[0, 4) K[0, 16) on @acc_4x4_0
// FLEET-NEXT: region[3] I[8, 12) J[4, 8) K[0, 16) on @acc_4x4_1
// FLEET-NEXT: region[4] I[8, 12) J[8, 12) K[0, 16) on @acc_4x4_2
// FLEET-NEXT: region[5] I[8, 12) J[12, 16) K[0, 16) on @acc_4x4_0
// FLEET-NEXT: region[6] I[12, 16) J[0, 4) K[0, 16) on @acc_4x4_1
// FLEET-NEXT: region[7] I[12, 16) J[4, 8) K[0, 16) on @acc_4x4_2
// FLEET-NEXT: region[8] I[12, 16) J[8, 12) K[0, 16) on @acc_4x4_0
// FLEET-NEXT: region[9] I[12, 16) J[12, 16) K[0, 16) on @acc_4x4_1
//
// Partitions given directly (lower I, J, K, extent I, J, K per region):
// splitting the reduction is exact; the verifier rejects a partition that
// would run iterations twice, lose them, or run outside the domain.
//
// SPLITK:  given partition: 2 regions, exact
// OVERLAP: given partition: not exact (region 1 overlaps an earlier region at (I, J, K) = (0, 0, 8): those iterations would run twice)
// GAP:     given partition: not exact (no region covers (I, J, K) = (8, 0, 0): those iterations would be lost)
// OUTSIDE: given partition: not exact (region 0 is empty or extends outside the iteration domain)

func.func @gemm(%A: memref<16x16xf32> {llvm.noalias},
                %B: memref<16x16xf32> {llvm.noalias},
                %C: memref<16x16xf32> {llvm.noalias}) {
  affine.for %k = 0 to 16 {
    affine.for %i = 0 to 16 {
      affine.for %j = 0 to 16 {
        %a = affine.load %A[%i, %k] : memref<16x16xf32>
        %b = affine.load %B[%k, %j] : memref<16x16xf32>
        %c = affine.load %C[%i, %j] : memref<16x16xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<16x16xf32>
      }
    }
  }
  return
}
