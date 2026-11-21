// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_scatter.json record-driven=true report=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=REMARK
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_scatter.json record-driven=true" | FileCheck %s --check-prefix=IR
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_scatter.json record-driven=true" --lower-systolic-matmul-tile-to-fpga | FileCheck %s --check-prefix=SYNC
//
// Experimental (exp/general-scatter): outputs at non-affine addresses, lowered from their
// records. Inputs/nest_records_scatter.json is what generalized-gemm-nest-report
// --admit-gather says of this file (every pair of roots authorized, 64-bit index), written and
// checked by research/generalized-gemm/test/fuzz/nest_gate_records.py: perm VERIFIED_NOT_MAPPED
// (C's view is not affine), scatter and scatter2 VERIFIED_NOT_MAPPED (two outputs may share a
// cell of C; scatter2's view gathers at a gather).
//
// Track A: (3 i + 1) mod 16 is a bijection of I's range; its inverse (11 m + 5) mod 16 packs A,
// and C is a view. Track B: the targets computed and checked at run time, the nest as written
// in the else branch, the tiles into a zeroed matrix, and the two-layer inverse map.
// REMARK: remark: GEMM 16x8x12 of nest 0 of perm lowered from its record as 5 exact region(s) (inverse: C's rows (3 i + 1) mod 16, a bijection, inverted in closed form (11 m + 5) mod 16; A packed by the inverse, C written as a view)
// REMARK: remark: GEMM 16x8x12 of nest 0 of scatter lowered from its record as 5 exact region(s) (scatter-add: C's address re-emitted at run time through read-only tables, with collisions, every index checked before a write, a two-layer inverse map (bucket offsets, sources sorted by cell of C), each cell of C written once)
// REMARK: remark: GEMM 16x8x12 of nest 0 of scatter2 lowered from its record as 5 exact region(s) (scatter-add:
// REMARK-NOT: remark:
//
// IR-LABEL: func.func @perm
// IR: memref.alloc() : memref<16x12xf32>
// IR: affine.apply #[[INV:.*]](%{{.*}}, %{{.*}})
// IR: systolic.matmul_tile
// IR-NOT: affine.for
// IR: return
// IR-LABEL: func.func @scatter(
// IR: %[[TARGETS:.*]] = memref.alloc() : memref<128xindex>
// IR: memref.load %arg1[%{{.*}}] : memref<16xi32>
// IR: memref.store %{{.*}}, %[[TARGETS]][%{{.*}}] : memref<128xindex>
// IR: scf.if
// IR: systolic.matmul_tile
// IR: %[[OFFSETS:.*]] = memref.alloc() : memref<129xindex>
// IR: memref.alloc() : memref<128xindex>
// IR: %[[SORTED:.*]] = memref.alloc() : memref<128xindex>
// IR: scf.for
// IR: memref.load %[[SORTED]]
// IR: } else {
// IR: affine.for
// IR: arith.addf
// IR: return
// IR-LABEL: func.func @scatter2(
// IR: memref.load %arg2
// IR: memref.load %arg1
// IR: scf.if
// IR: return
// SYNC-LABEL: func.func @scatter2(
// SYNC-NOT: systolic.matmul_tile
// SYNC: return

func.func @perm(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %c3 = arith.constant 3 : index
        %c1 = arith.constant 1 : index
        %c16 = arith.constant 16 : index
        %m = arith.muli %i, %c3 : index
        %s = arith.addi %m, %c1 : index
        %r = arith.remui %s, %c16 : index
        %x = memref.load %a[%i, %k] : memref<16x12xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %z = memref.load %c[%r, %j] : memref<16x8xf32>
        %p = arith.mulf %x, %y fastmath<reassoc> : f32
        %q = arith.addf %z, %p fastmath<reassoc> : f32
        memref.store %q, %c[%r, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
func.func @scatter(%a: memref<16x12xf32>, %dp: memref<16xi32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %t = memref.load %dp[%i] : memref<16xi32>
        %r = arith.index_cast %t : i32 to index
        %x = memref.load %a[%i, %k] : memref<16x12xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %z = memref.load %c[%r, %j] : memref<16x8xf32>
        %p = arith.mulf %x, %y fastmath<reassoc> : f32
        %q = arith.addf %z, %p fastmath<reassoc> : f32
        memref.store %q, %c[%r, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
func.func @scatter2(%a: memref<16x12xf32>, %dp1: memref<16xi32>, %dp2: memref<16xi32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %t2 = memref.load %dp2[%i] : memref<16xi32>
        %u2 = arith.index_cast %t2 : i32 to index
        %t1 = memref.load %dp1[%u2] : memref<16xi32>
        %r = arith.index_cast %t1 : i32 to index
        %x = memref.load %a[%i, %k] : memref<16x12xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %z = memref.load %c[%r, %j] : memref<16x8xf32>
        %p = arith.mulf %x, %y fastmath<reassoc> : f32
        %q = arith.addf %z, %p fastmath<reassoc> : f32
        memref.store %q, %c[%r, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
