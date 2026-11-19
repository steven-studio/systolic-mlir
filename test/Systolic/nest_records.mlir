// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records.json report=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=REMARK
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records.json" | FileCheck %s --check-prefix=TILES
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records.json" --lower-systolic-matmul-tile-to-fpga=async=true | FileCheck %s --check-prefix=ASYNC
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records.json" --lower-systolic-matmul-tile-to-fpga | FileCheck %s --check-prefix=SYNC
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="report=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=OFF
//
// Stage 2 of docs/GENERALIZED_GEMM_SCOPE.md: the dense lowering, gated by the research
// recognizer's records of loop nests (Systolic/NestRecordGate.h). Inputs/nest_records.json
// is what generalized-gemm-nest-report says of this file, every pair of roots authorized,
// at a 64-bit index; research/generalized-gemm/test/fuzz/nest_gate_records.py writes it
// and checks that it still does. Each nest is recognized and lowered by itself, where it
// stands: two nests on one output stay two GEMMs, in order, and so do a nest and the
// nest that reads what it wrote; the loop that zeroes the output of @initialized stays
// before both, and its two GEMMs are nests 1 and 2 of their function, as their records say.
// A record never replaces a check of the lowering: @strict has a record that agrees, and
// the floating-point policy still refuses it.
// nest_records_refusals.mlir has the records that do not let a lowering go ahead.
//
// REMARK: remark: nest record agrees: nest 0 of single (binding {{[0-9a-f]+}}, index width 64): VERIFIED for runs that complete; 16x8x12, the loops of I, J and K, the roots and views of A, B and C, the element type, the update and its flags agree; separations assumed: (0, 2), (1, 2), which the lowering's alias gate checks itself
// REMARK-NEXT: affine.for
// REMARK: remark: affine GEMM 16x8x12 lowered as 5 exact region(s)
// REMARK: remark: nest record agrees: nest 0 of strict
// REMARK: remark: affine GEMM not lowered to systolic.matmul_tile: floating-point reassociation is not permitted
// REMARK: remark: nest record agrees: nest 0 of shared
// REMARK: remark: affine GEMM 16x8x12 lowered as 5 exact region(s)
// REMARK: remark: nest record agrees: nest 1 of shared ({{.*}}separations assumed: (0, 2), (2, 3), which
// REMARK: remark: affine GEMM 16x8x12 lowered as 5 exact region(s)
// REMARK: remark: nest record agrees: nest 0 of stored
// REMARK: remark: affine GEMM 16x8x12 lowered as 5 exact region(s)
// REMARK: remark: nest record agrees: nest 1 of stored ({{.*}}separations assumed: (0, 3), (1, 3), which
// REMARK: remark: affine GEMM 16x8x12 lowered as 5 exact region(s)
// REMARK: remark: nest record agrees: nest 0 of reads
// REMARK: remark: affine GEMM 16x8x12 lowered as 5 exact region(s)
// REMARK: remark: nest record agrees: nest 1 of reads ({{.*}}): VERIFIED for runs that complete; 16x8x8, {{.*}}separations assumed: (2, 4), (3, 4), which
// REMARK: remark: affine GEMM 16x8x8 lowered as 5 exact region(s)
// REMARK: remark: nest record agrees: nest 1 of initialized
// REMARK: remark: affine GEMM 16x8x12 lowered as 5 exact region(s)
// REMARK: remark: nest record agrees: nest 2 of initialized
// REMARK: remark: affine GEMM 16x8x12 lowered as 5 exact region(s)
// REMARK-NOT: remark:
//
// Each GEMM where its nest stood; the statements between two nests stay between them.
// TILES-LABEL: func.func @single
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: affine.for
// TILES: return
// TILES-LABEL: func.func @strict
// TILES-NOT: systolic.matmul_tile
// TILES: affine.for
// TILES: arith.addf
// TILES: return
// TILES-LABEL: func.func @shared
// TILES-COUNT-10: systolic.matmul_tile
// TILES-NOT: affine.for
// TILES: return
// TILES-LABEL: func.func @stored
// TILES-COUNT-5: systolic.matmul_tile
// TILES: memref.load %arg2[%c0, %c0]
// TILES-NEXT: memref.store %{{.*}}, %arg3[%c1, %c1]
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: affine.for
// TILES: return
// TILES-LABEL: func.func @reads
// TILES-COUNT-10: systolic.matmul_tile
// TILES-NOT: affine.for
// TILES: return
// TILES-LABEL: func.func @initialized
// TILES: affine.for
// TILES-NEXT: affine.for
// TILES-NEXT: affine.store %{{.*}}, %arg2[%{{.*}}, %{{.*}}]
// TILES-COUNT-10: systolic.matmul_tile
// TILES-NOT: affine.for
// TILES: return
//
// Asynchronously: each GEMM is waited for, and its C written back, before the next
// statement: the load of C in @stored comes after the first GEMM's wait, and the second
// GEMM of @shared, of @reads and of @initialized begins after the first one's. The first
// GEMM of @initialized reads C after the loop that zeroes it.
// ASYNC-LABEL: func.func @single
// ASYNC: llvm.call @fpga_gemm_begin
// ASYNC-COUNT-5: llvm.call @fpga_tile_submit
// ASYNC: llvm.call @fpga_gemm_wait
// ASYNC-NEXT: bufferization.materialize_in_destination {{.*}} in restrict writable %arg2
// ASYNC-LABEL: func.func @strict
// ASYNC-NOT: llvm.call
// ASYNC: affine.for
// ASYNC-LABEL: func.func @shared
// ASYNC: llvm.call @fpga_gemm_begin
// ASYNC-COUNT-5: llvm.call @fpga_tile_submit
// ASYNC: llvm.call @fpga_gemm_wait
// ASYNC-NEXT: bufferization.materialize_in_destination {{.*}} in restrict writable %arg2
// ASYNC: llvm.call @fpga_gemm_begin
// ASYNC-COUNT-5: llvm.call @fpga_tile_submit
// ASYNC: llvm.call @fpga_gemm_wait
// ASYNC-NEXT: bufferization.materialize_in_destination {{.*}} in restrict writable %arg2
// ASYNC-LABEL: func.func @stored
// ASYNC: llvm.call @fpga_gemm_begin
// ASYNC-COUNT-5: llvm.call @fpga_tile_submit
// ASYNC: llvm.call @fpga_gemm_wait
// ASYNC-NEXT: bufferization.materialize_in_destination {{.*}} in restrict writable %arg2
// ASYNC-NEXT: memref.load %arg2[%c0, %c0]
// ASYNC-NEXT: memref.store %{{.*}}, %arg3[%c1, %c1]
// ASYNC: llvm.call @fpga_gemm_begin
// ASYNC-COUNT-5: llvm.call @fpga_tile_submit
// ASYNC: llvm.call @fpga_gemm_wait
// ASYNC-NEXT: bufferization.materialize_in_destination {{.*}} in restrict writable %arg3
// ASYNC-LABEL: func.func @reads
// ASYNC: llvm.call @fpga_gemm_begin
// ASYNC-COUNT-5: llvm.call @fpga_tile_submit
// ASYNC: llvm.call @fpga_gemm_wait
// ASYNC-NEXT: bufferization.materialize_in_destination {{.*}} in restrict writable %arg2
// ASYNC: llvm.call @fpga_gemm_begin
// ASYNC-COUNT-5: llvm.call @fpga_tile_submit
// ASYNC: llvm.call @fpga_gemm_wait
// ASYNC-NEXT: bufferization.materialize_in_destination {{.*}} in restrict writable %arg4
// ASYNC-LABEL: func.func @initialized
// ASYNC: affine.store %{{.*}}, %arg2[%{{.*}}, %{{.*}}]
// ASYNC: bufferization.to_tensor %arg2
// ASYNC: llvm.call @fpga_gemm_begin
// ASYNC-COUNT-5: llvm.call @fpga_tile_submit
// ASYNC: llvm.call @fpga_gemm_wait
// ASYNC-NEXT: bufferization.materialize_in_destination {{.*}} in restrict writable %arg2
// ASYNC: llvm.call @fpga_gemm_begin
// ASYNC-COUNT-5: llvm.call @fpga_tile_submit
// ASYNC: llvm.call @fpga_gemm_wait
// ASYNC-NEXT: bufferization.materialize_in_destination {{.*}} in restrict writable %arg2
//
// SYNC-LABEL: func.func @single
// SYNC-COUNT-5: llvm.call @fpga_matmul_tiled_auto_scheduled
// SYNC-LABEL: func.func @strict
// SYNC-NOT: llvm.call
// SYNC-LABEL: func.func @shared
// SYNC-COUNT-10: llvm.call @fpga_matmul_tiled_auto_scheduled
// SYNC-LABEL: func.func @stored
// SYNC-COUNT-5: llvm.call @fpga_matmul_tiled_auto_scheduled
// SYNC: memref.load %arg2[%c0, %c0]
// SYNC-NEXT: memref.store %{{.*}}, %arg3[%c1, %c1]
// SYNC-COUNT-5: llvm.call @fpga_matmul_tiled_auto_scheduled
// SYNC-LABEL: func.func @reads
// SYNC-COUNT-10: llvm.call @fpga_matmul_tiled_auto_scheduled
// SYNC-LABEL: func.func @initialized
// SYNC: affine.store %{{.*}}, %arg2[%{{.*}}, %{{.*}}]
// SYNC-COUNT-10: llvm.call @fpga_matmul_tiled_auto_scheduled
//
// Without the option nothing reads a record, and the same nests are lowered as before.
// OFF-NOT: nest record
// OFF: remark: affine GEMM 16x8x12 lowered as 5 exact region(s)
// OFF: remark: affine GEMM not lowered to systolic.matmul_tile: floating-point reassociation is not permitted
// OFF-COUNT-8: remark: affine GEMM {{16x8x12|16x8x8}} lowered as 5 exact region(s)
// OFF-NOT: nest record
// One product over a constant box, in binary32 with reassoc on the accumulation.
func.func @single(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %z = affine.load %c[%i, %j] : memref<16x8xf32>
        %x = affine.load %a[%i, %k] : memref<16x12xf32>
        %y = affine.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        affine.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
// The same product with no flag: the record agrees, and the lowering's floating-point
// policy refuses on its own.
func.func @strict(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %z = affine.load %c[%i, %j] : memref<16x8xf32>
        %x = affine.load %a[%i, %k] : memref<16x12xf32>
        %y = affine.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m : f32
        affine.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
// Two nests on one output: C += A B, then C += A D.
func.func @shared(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}, %d: memref<12x8xf32>) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %z = affine.load %c[%i, %j] : memref<16x8xf32>
        %x = affine.load %a[%i, %k] : memref<16x12xf32>
        %y = affine.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        affine.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  affine.for %i2 = 0 to 16 {
    affine.for %j2 = 0 to 8 {
      affine.for %k2 = 0 to 12 {
        %z2 = affine.load %c[%i2, %j2] : memref<16x8xf32>
        %x2 = affine.load %a[%i2, %k2] : memref<16x12xf32>
        %y2 = affine.load %d[%k2, %j2] : memref<12x8xf32>
        %m2 = arith.mulf %x2, %y2 : f32
        %s2 = arith.addf %z2, %m2 fastmath<reassoc> : f32
        affine.store %s2, %c[%i2, %j2] : memref<16x8xf32>
      }
    }
  }
  return
}
// A nest on C, a value of C stored into D, a nest on D.
func.func @stored(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}, %d: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %z = affine.load %c[%i, %j] : memref<16x8xf32>
        %x = affine.load %a[%i, %k] : memref<16x12xf32>
        %y = affine.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        affine.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  %v = memref.load %c[%c0, %c0] : memref<16x8xf32>
  memref.store %v, %d[%c1, %c1] : memref<16x8xf32>
  affine.for %i2 = 0 to 16 {
    affine.for %j2 = 0 to 8 {
      affine.for %k2 = 0 to 12 {
        %z2 = affine.load %d[%i2, %j2] : memref<16x8xf32>
        %x2 = affine.load %a[%i2, %k2] : memref<16x12xf32>
        %y2 = affine.load %b[%k2, %j2] : memref<12x8xf32>
        %m2 = arith.mulf %x2, %y2 : f32
        %s2 = arith.addf %z2, %m2 fastmath<reassoc> : f32
        affine.store %s2, %d[%i2, %j2] : memref<16x8xf32>
      }
    }
  }
  return
}
// The second nest reads, as its A, the C the first one wrote: D += C E.
func.func @reads(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}, %e: memref<8x8xf32>, %d: memref<16x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %z = affine.load %c[%i, %j] : memref<16x8xf32>
        %x = affine.load %a[%i, %k] : memref<16x12xf32>
        %y = affine.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        affine.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  affine.for %i2 = 0 to 16 {
    affine.for %j2 = 0 to 8 {
      affine.for %k2 = 0 to 8 {
        %z2 = affine.load %d[%i2, %j2] : memref<16x8xf32>
        %x2 = affine.load %c[%i2, %k2] : memref<16x8xf32>
        %y2 = affine.load %e[%k2, %j2] : memref<8x8xf32>
        %m2 = arith.mulf %x2, %y2 : f32
        %s2 = arith.addf %z2, %m2 fastmath<reassoc> : f32
        affine.store %s2, %d[%i2, %j2] : memref<16x8xf32>
      }
    }
  }
  return
}
// C zeroed by a loop, then two nests on it, C += A B and C += A D. The loop is no GEMM and
// stays; each nest's record is about the nest at its position, 1 and 2.
func.func @initialized(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}, %d: memref<12x8xf32>) {
  %zero = arith.constant 0.000000e+00 : f32
  affine.for %i0 = 0 to 16 {
    affine.for %j0 = 0 to 8 {
      affine.store %zero, %c[%i0, %j0] : memref<16x8xf32>
    }
  }
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %z = affine.load %c[%i, %j] : memref<16x8xf32>
        %x = affine.load %a[%i, %k] : memref<16x12xf32>
        %y = affine.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        affine.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  affine.for %i2 = 0 to 16 {
    affine.for %j2 = 0 to 8 {
      affine.for %k2 = 0 to 12 {
        %z2 = affine.load %c[%i2, %j2] : memref<16x8xf32>
        %x2 = affine.load %a[%i2, %k2] : memref<16x12xf32>
        %y2 = affine.load %d[%k2, %j2] : memref<12x8xf32>
        %m2 = arith.mulf %x2, %y2 : f32
        %s2 = arith.addf %z2, %m2 fastmath<reassoc> : f32
        affine.store %s2, %c[%i2, %j2] : memref<16x8xf32>
      }
    }
  }
  return
}
