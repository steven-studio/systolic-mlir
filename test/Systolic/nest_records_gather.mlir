// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_gather.json record-driven=true report=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=REMARK
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_gather.json record-driven=true" | FileCheck %s --check-prefix=TILES
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_gather.json record-driven=true" --lower-systolic-matmul-tile-to-fpga | FileCheck %s --check-prefix=SYNC
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_gather_off.json record-driven=true report=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=OFF
//
// Experimental (exp/semgus-indirect-dp): C[i, j] += A[dp[i], k] * B[k, j], dp a read-only
// integer row index (MoE's sorted_token_ids, a BSR column index). Inputs/nest_records_gather.json
// is what generalized-gemm-nest-report --admit-gather says of this file (every pair of roots
// authorized, 64-bit index): VERIFIED, with dimension 0 of A's view a `gather` of dp at I.
// The lowering packs A_packed[r][c] = A[dp[r], c] before the tiles and runs the dense tiles on
// it; the systolic array's own schedule is unchanged. Inputs/nest_records_gather_off.json is
// the same tool without --admit-gather: no claim, and the nests stay as they are.
//
// REMARK: remark: GEMM 16x8x12 of nest 0 of gather lowered from its record as {{[0-9]+}} exact region(s) (formulas: {{.*}}; A or B packed by the record's gather before the tiles)
// REMARK: remark: GEMM 16x8x12 of nest 0 of gather_unsigned lowered from its record as {{[0-9]+}} exact region(s) (formulas: {{.*}}; A or B packed by the record's gather before the tiles)
// REMARK-NOT: remark:
//
// TILES-LABEL: func.func @gather
// TILES: %[[P:.*]] = memref.alloc() : memref<16x12xf32>
// TILES-NEXT: affine.for %[[R:.*]] = 0 to 16 {
// TILES-NEXT: affine.for %[[Q:.*]] = 0 to 12 {
// TILES-NEXT: %[[IR:.*]] = affine.apply #{{.*}}(%[[R]], %[[Q]])
// TILES-NEXT: %[[T:.*]] = memref.load %arg1[%[[IR]]] : memref<16xi32>
// TILES-NEXT: %[[TI:.*]] = arith.index_cast %[[T]] : i32 to index
// TILES-NEXT: %[[IK:.*]] = affine.apply #{{.*}}(%[[R]], %[[Q]])
// TILES-NEXT: %[[V:.*]] = memref.load %arg0[%[[TI]], %[[IK]]] : memref<32x12xf32>
// TILES-NEXT: affine.store %[[V]], %[[P]][%[[R]], %[[Q]]] : memref<16x12xf32>
// TILES: bufferization.to_tensor %[[P]]
// TILES: systolic.matmul_tile
// TILES-NOT: affine.for
// TILES: return
// TILES-LABEL: func.func @gather_unsigned
// TILES: memref.alloc() : memref<16x12xf32>
// TILES: arith.index_castui %{{.*}} : i32 to index
// TILES: systolic.matmul_tile
// TILES-NOT: affine.for
// TILES: return
//
// SYNC-LABEL: func.func @gather
// SYNC: arith.index_cast %{{.*}} : i32 to index
// SYNC-NOT: systolic.matmul_tile
// SYNC: return
//
// OFF: remark: GEMM not lowered from its record: nest 0 of gather: the record's status is NOT_VERIFIED: no claim was made about the nest: the gate is UNSUPPORTED (the trace depends on memory contents)
// OFF: remark: GEMM not lowered from its record: nest 0 of gather_unsigned: the record's status is NOT_VERIFIED
// OFF-NOT: remark: GEMM 16x8x12

// C[i, j] += A[dp[i], k] * B[k, j], dp of i32, sign-extended.
func.func @gather(%a: memref<32x12xf32>, %dp: memref<16xi32>, %b: memref<12x8xf32>,
                  %c: memref<16x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %t = memref.load %dp[%i] : memref<16xi32>
        %r = arith.index_cast %t : i32 to index
        %x = memref.load %a[%r, %k] : memref<32x12xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %p = arith.mulf %x, %y fastmath<reassoc> : f32
        %s = arith.addf %z, %p fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}

// The same, dp zero-extended (index_castui).
func.func @gather_unsigned(%a: memref<32x12xf32>, %dp: memref<16xi32>, %b: memref<12x8xf32>,
                           %c: memref<16x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %t = memref.load %dp[%i] : memref<16xi32>
        %r = arith.index_castui %t : i32 to index
        %x = memref.load %a[%r, %k] : memref<32x12xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %p = arith.mulf %x, %y fastmath<reassoc> : f32
        %s = arith.addf %z, %p fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
