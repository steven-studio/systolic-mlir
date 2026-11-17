// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_scf.json record-driven=true report=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=REMARK
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_scf.json record-driven=true" | FileCheck %s --check-prefix=TILES
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_scf.json record-driven=true" --lower-systolic-matmul-tile-to-fpga=async=true | FileCheck %s --check-prefix=ASYNC
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_scf.json record-driven=true" --lower-systolic-matmul-tile-to-fpga | FileCheck %s --check-prefix=SYNC
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_scf.json report=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=STAGE2
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="report=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=OFF
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="report=true" | FileCheck %s --check-prefix=OFFIR
//
// GEMMs written with scf.for, lowered from their records (stage 3 of
// docs/GENERALIZED_GEMM_SCOPE.md, on scf.for loops): memref.load and memref.store, the loops'
// bounds and steps arith.constant values defined before the nest (in @scf_inside, each in the
// body of the loop around). The production recognizer reads affine loops only and finds no
// GEMM here; the research recognizer verifies the nests, and the lowering takes them from
// their records as it takes affine.for nests (Systolic/NestRecordLowering.h): by the record's
// closed forms of the views, each checked against the nest's own index arithmetic, or, where
// the research has no affine closed form, by tables of the nest's addresses. The first three
// functions are one GEMM written with scf.for, with affine.for and affine.load (stage 2: the
// recognizer and its record), and with affine.for and memref.load.
// Inputs/nest_records_scf.json is what generalized-gemm-nest-report says of this file (every
// pair of roots authorized, 64-bit index), written and checked by
// research/generalized-gemm/test/fuzz/nest_gate_records.py. The functions are
// nest_scf_e2e.py's, which runs them and compares every result with the original program's;
// nest_records_scf_refusals.mlir has the refusals.
//
// REMARK: remark: nest record agrees: nest 0 of scf_twin_affine
// REMARK: remark: affine GEMM 16x8x12 lowered as 5 exact region(s)
// REMARK: remark: GEMM 16x8x12 of nest 0 of scf_gemm lowered from its record as 5 exact region(s) (formulas: the record's views of A, B and C are the nest's own index arithmetic)
// REMARK: remark: GEMM 16x8x12 of nest 0 of scf_twin_memref lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 0 of scf_inside lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 0 of scf_transposed lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 0 of scf_flat lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 0 of scf_offsets lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 0 of scf_shifted lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 0 of scf_tables_flat lowered from its record as 5 exact region(s) (tables: the nest's own addresses at its 1536 iterations, A one per (I, K), B one per (K, J), C one per (I, J) and one to one; each (I, J, K) is one iteration)
// REMARK: remark: GEMM 16x8x12 of nest 0 of scf_tables_perm lowered from its record as 5 exact region(s) (tables:
// REMARK: remark: GEMM 16x8x12 of nest 0 of scf_shared lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 1 of scf_shared lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 0 of scf_chain lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x8 of nest 1 of scf_chain lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM not lowered from its record: nest 0 of scf_strict: floating-point reassociation is not permitted
// REMARK-NOT: remark:
//
// Each lowered nest is gone, its tiles where it stood; the bound constants before it stay.
// TILES-DAG: memref.global "private" constant @__nest_table_nest_0_of_scf_tables_flat_A : memref<16x12xi64>
// TILES-DAG: memref.global "private" constant @__nest_table_nest_0_of_scf_tables_flat_B : memref<12x8xi64>
// TILES-DAG: memref.global "private" constant @__nest_table_nest_0_of_scf_tables_flat_C : memref<16x8xi64>
// TILES-DAG: memref.global "private" constant @__nest_table_nest_0_of_scf_tables_perm_A : memref<16x12xi64>
// TILES-LABEL: func.func @scf_gemm
// TILES: bufferization.to_tensor %arg0 restrict : memref<16x12xf32>
// TILES-COUNT-5: systolic.matmul_tile
// TILES: bufferization.materialize_in_destination {{.*}} in restrict writable %arg2
// TILES-NOT: scf.for
// TILES: return
// TILES-LABEL: func.func @scf_twin_affine
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: affine.for
// TILES: return
// TILES-LABEL: func.func @scf_twin_memref
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: affine.for
// TILES: return
// TILES-LABEL: func.func @scf_inside
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: scf.for
// TILES: return
// TILES-LABEL: func.func @scf_transposed
// TILES: memref.subview %arg1[0, 0] [8, 12] [1, 1]
// TILES-NEXT: memref.transpose
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: scf.for
// TILES: return
// TILES-LABEL: func.func @scf_flat
// TILES: memref.reinterpret_cast %arg0 to offset: [0], sizes: [16, 12], strides: [12, 1]
// TILES: memref.reinterpret_cast %arg1 to offset: [0], sizes: [12, 8], strides: [8, 1]
// TILES: memref.reinterpret_cast %arg2 to offset: [0], sizes: [16, 8], strides: [8, 1]
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: scf.for
// TILES: return
// TILES-LABEL: func.func @scf_offsets
// TILES: memref.subview %arg0[2, 1] [16, 12] [1, 1]
// TILES: memref.subview %arg1[1, 0] [12, 8] [2, 1]
// TILES: memref.subview %arg2[1, 2] [16, 8] [1, 1]
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: scf.for
// TILES: return
// TILES-LABEL: func.func @scf_shifted
// TILES: memref.subview %arg0[2, 3] [16, 12] [1, 1]
// TILES: memref.subview %arg1[3, 1] [12, 8] [1, 1]
// TILES: memref.subview %arg2[2, 1] [16, 8] [1, 1]
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: scf.for
// TILES: return
// TILES-LABEL: func.func @scf_tables_flat
// TILES: memref.get_global @__nest_table_nest_0_of_scf_tables_flat_A : memref<16x12xi64>
// TILES-NEXT: memref.reinterpret_cast %arg0 to offset: [0], sizes: [192], strides: [1]
// TILES-NEXT: memref.alloc() : memref<16x12xf32>
// TILES: memref.get_global @__nest_table_nest_0_of_scf_tables_flat_C : memref<16x8xi64>
// TILES-COUNT-5: systolic.matmul_tile
// TILES: bufferization.materialize_in_destination
// TILES: memref.store
// TILES-NOT: scf.for
// TILES: return
// TILES-LABEL: func.func @scf_tables_perm
// TILES: memref.get_global @__nest_table_nest_0_of_scf_tables_perm_A
// TILES-COUNT-5: systolic.matmul_tile
// TILES: return
// TILES-LABEL: func.func @scf_shared
// TILES-COUNT-10: systolic.matmul_tile
// TILES-NOT: scf.for
// TILES: return
// The second nest reads T only after the first wrote T back.
// TILES-LABEL: func.func @scf_chain
// TILES: bufferization.to_tensor %arg2 restrict writable
// TILES-COUNT-5: systolic.matmul_tile
// TILES: bufferization.materialize_in_destination {{.*}} in restrict writable %arg2
// TILES: bufferization.to_tensor %arg2 restrict :
// TILES: bufferization.to_tensor %arg4 restrict writable
// TILES-COUNT-5: systolic.matmul_tile
// TILES: bufferization.materialize_in_destination {{.*}} in restrict writable %arg4
// TILES-NOT: scf.for
// TILES: return
// TILES-LABEL: func.func @scf_strict
// TILES-NOT: systolic.matmul_tile
// TILES: scf.for
// TILES: return
//
// Asynchronously: each GEMM begins, submits its tiles, is waited for and written back before
// the next statement; in @scf_chain the second GEMM begins after T is written back.
// ASYNC-LABEL: func.func @scf_gemm
// ASYNC: llvm.call @fpga_gemm_begin
// ASYNC-COUNT-5: llvm.call @fpga_tile_submit
// ASYNC: llvm.call @fpga_gemm_wait
// ASYNC-NEXT: bufferization.materialize_in_destination {{.*}} in restrict writable %arg2
// ASYNC-LABEL: func.func @scf_tables_perm
// ASYNC: %[[PC:.*]] = memref.reinterpret_cast %arg2 to offset: [0], sizes: [128], strides: [1]
// ASYNC-NEXT: %[[PM:.*]] = memref.alloc() : memref<16x8xf32>
// ASYNC: memref.load %[[PC]]
// ASYNC: llvm.call @fpga_gemm_begin
// ASYNC-COUNT-5: llvm.call @fpga_tile_submit
// ASYNC: llvm.call @fpga_gemm_wait
// ASYNC-NEXT: bufferization.materialize_in_destination {{.*}} in restrict writable %[[PM]]
// ASYNC-NEXT: affine.for
// ASYNC: memref.store %{{.*}}, %[[PC]]
// ASYNC-LABEL: func.func @scf_chain
// ASYNC: llvm.call @fpga_gemm_begin
// ASYNC-COUNT-5: llvm.call @fpga_tile_submit
// ASYNC: llvm.call @fpga_gemm_wait
// ASYNC-NEXT: bufferization.materialize_in_destination {{.*}} in restrict writable %arg2
// ASYNC: llvm.call @fpga_gemm_begin
// ASYNC-COUNT-5: llvm.call @fpga_tile_submit
// ASYNC: llvm.call @fpga_gemm_wait
// ASYNC-NEXT: bufferization.materialize_in_destination {{.*}} in restrict writable %arg4
// ASYNC-LABEL: func.func @scf_strict
// ASYNC-NOT: llvm.call
// ASYNC: scf.for
//
// SYNC-LABEL: func.func @scf_gemm
// SYNC-COUNT-5: llvm.call @fpga_matmul_tiled_auto_scheduled
// SYNC-LABEL: func.func @scf_tables_flat
// SYNC: affine.for
// SYNC-COUNT-5: llvm.call @fpga_matmul_tiled_auto_scheduled
// SYNC: memref.store
// SYNC-LABEL: func.func @scf_shared
// SYNC-COUNT-10: llvm.call @fpga_matmul_tiled_auto_scheduled
// SYNC-LABEL: func.func @scf_chain
// SYNC-COUNT-5: llvm.call @fpga_matmul_tiled_auto_scheduled
// SYNC: bufferization.materialize_in_destination {{.*}} in restrict writable %arg2
// SYNC-COUNT-5: llvm.call @fpga_matmul_tiled_auto_scheduled
// SYNC: bufferization.materialize_in_destination {{.*}} in restrict writable %arg4
// SYNC-LABEL: func.func @scf_strict
// SYNC-NOT: llvm.call
//
// Without record-driven, stage 2 alone: the recognizer's GEMM in @scf_twin_affine, nothing
// from a record. Without records, the recognizer alone; the scf.for nests stay as written.
// STAGE2: remark: nest record agrees: nest 0 of scf_twin_affine
// STAGE2: remark: affine GEMM 16x8x12 lowered as 5 exact region(s)
// STAGE2-NOT: from its record
// OFF: remark: affine GEMM 16x8x12 lowered as 5 exact region(s)
// OFF-NOT: {{nest record|from its record}}
// OFFIR-LABEL: func.func @scf_gemm
// OFFIR-NOT: systolic.matmul_tile
// OFFIR: scf.for
// OFFIR-LABEL: func.func @scf_twin_affine
// OFFIR-COUNT-5: systolic.matmul_tile
// OFFIR-LABEL: func.func @scf_twin_memref
// OFFIR-NOT: systolic.matmul_tile
// OFFIR: affine.for
// OFFIR-LABEL: func.func @scf_inside
// OFFIR-NOT: systolic.matmul_tile
// OFFIR: scf.for
func.func @scf_gemm(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %ilo0 = arith.constant 0 : index
  %ihi0 = arith.constant 16 : index
  %ist0 = arith.constant 1 : index
  %jlo0 = arith.constant 0 : index
  %jhi0 = arith.constant 8 : index
  %jst0 = arith.constant 1 : index
  %klo0 = arith.constant 0 : index
  %khi0 = arith.constant 12 : index
  %kst0 = arith.constant 1 : index
  scf.for %i0 = %ilo0 to %ihi0 step %ist0 {
    scf.for %j0 = %jlo0 to %jhi0 step %jst0 {
      scf.for %k0 = %klo0 to %khi0 step %kst0 {
        %z0 = memref.load %c[%i0, %j0] : memref<16x8xf32>
        %x0 = memref.load %a[%i0, %k0] : memref<16x12xf32>
        %y0 = memref.load %b[%k0, %j0] : memref<12x8xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%i0, %j0] : memref<16x8xf32>
      }
    }
  }
  return
}
func.func @scf_twin_affine(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  affine.for %i0 = 0 to 16 {
    affine.for %j0 = 0 to 8 {
      affine.for %k0 = 0 to 12 {
        %z0 = affine.load %c[%i0, %j0] : memref<16x8xf32>
        %x0 = affine.load %a[%i0, %k0] : memref<16x12xf32>
        %y0 = affine.load %b[%k0, %j0] : memref<12x8xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        affine.store %s0, %c[%i0, %j0] : memref<16x8xf32>
      }
    }
  }
  return
}
func.func @scf_twin_memref(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  affine.for %i0 = 0 to 16 {
    affine.for %j0 = 0 to 8 {
      affine.for %k0 = 0 to 12 {
        %z0 = memref.load %c[%i0, %j0] : memref<16x8xf32>
        %x0 = memref.load %a[%i0, %k0] : memref<16x12xf32>
        %y0 = memref.load %b[%k0, %j0] : memref<12x8xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%i0, %j0] : memref<16x8xf32>
      }
    }
  }
  return
}
func.func @scf_inside(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %ilo0 = arith.constant 0 : index
  %ihi0 = arith.constant 16 : index
  %ist0 = arith.constant 1 : index
  scf.for %i0 = %ilo0 to %ihi0 step %ist0 {
    %jlo0 = arith.constant 0 : index
    %jhi0 = arith.constant 8 : index
    %jst0 = arith.constant 1 : index
    scf.for %j0 = %jlo0 to %jhi0 step %jst0 {
      %klo0 = arith.constant 0 : index
      %khi0 = arith.constant 12 : index
      %kst0 = arith.constant 1 : index
      scf.for %k0 = %klo0 to %khi0 step %kst0 {
        %z0 = memref.load %c[%i0, %j0] : memref<16x8xf32>
        %x0 = memref.load %a[%i0, %k0] : memref<16x12xf32>
        %y0 = memref.load %b[%k0, %j0] : memref<12x8xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%i0, %j0] : memref<16x8xf32>
      }
    }
  }
  return
}
func.func @scf_transposed(%a: memref<16x12xf32>, %b: memref<8x12xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %klo0 = arith.constant 0 : index
  %khi0 = arith.constant 12 : index
  %kst0 = arith.constant 1 : index
  %ilo0 = arith.constant 0 : index
  %ihi0 = arith.constant 16 : index
  %ist0 = arith.constant 1 : index
  %jlo0 = arith.constant 0 : index
  %jhi0 = arith.constant 8 : index
  %jst0 = arith.constant 1 : index
  scf.for %k0 = %klo0 to %khi0 step %kst0 {
    scf.for %i0 = %ilo0 to %ihi0 step %ist0 {
      scf.for %j0 = %jlo0 to %jhi0 step %jst0 {
        %z0 = memref.load %c[%i0, %j0] : memref<16x8xf32>
        %x0 = memref.load %a[%i0, %k0] : memref<16x12xf32>
        %y0 = memref.load %b[%j0, %k0] : memref<8x12xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%i0, %j0] : memref<16x8xf32>
      }
    }
  }
  return
}
func.func @scf_flat(%a: memref<192xf32>, %b: memref<96xf32>, %c: memref<128xf32> {llvm.noalias}) {
  %ilo0 = arith.constant 0 : index
  %ihi0 = arith.constant 16 : index
  %ist0 = arith.constant 1 : index
  %jlo0 = arith.constant 0 : index
  %jhi0 = arith.constant 8 : index
  %jst0 = arith.constant 1 : index
  %klo0 = arith.constant 0 : index
  %khi0 = arith.constant 12 : index
  %kst0 = arith.constant 1 : index
  scf.for %i0 = %ilo0 to %ihi0 step %ist0 {
    scf.for %j0 = %jlo0 to %jhi0 step %jst0 {
      scf.for %k0 = %klo0 to %khi0 step %kst0 {
        %cw0 = arith.constant 8 : index
        %aw0 = arith.constant 12 : index
        %bw0 = arith.constant 8 : index
        %cr0 = arith.muli %i0, %cw0 : index
        %cx0 = arith.addi %cr0, %j0 : index
        %ar0 = arith.muli %i0, %aw0 : index
        %ax0 = arith.addi %ar0, %k0 : index
        %br0 = arith.muli %k0, %bw0 : index
        %bx0 = arith.addi %br0, %j0 : index
        %z0 = memref.load %c[%cx0] : memref<128xf32>
        %x0 = memref.load %a[%ax0] : memref<192xf32>
        %y0 = memref.load %b[%bx0] : memref<96xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%cx0] : memref<128xf32>
      }
    }
  }
  return
}
func.func @scf_offsets(%a: memref<18x13xf32>, %b: memref<25x8xf32>, %c: memref<17x10xf32> {llvm.noalias}) {
  %ilo0 = arith.constant 0 : index
  %ihi0 = arith.constant 16 : index
  %ist0 = arith.constant 1 : index
  %jlo0 = arith.constant 0 : index
  %jhi0 = arith.constant 8 : index
  %jst0 = arith.constant 1 : index
  %klo0 = arith.constant 0 : index
  %khi0 = arith.constant 12 : index
  %kst0 = arith.constant 1 : index
  scf.for %i0 = %ilo0 to %ihi0 step %ist0 {
    scf.for %j0 = %jlo0 to %jhi0 step %jst0 {
      scf.for %k0 = %klo0 to %khi0 step %kst0 {
        %co0c0 = arith.constant 1 : index
        %co1c0 = arith.constant 2 : index
        %ao0c0 = arith.constant 2 : index
        %ao1c0 = arith.constant 1 : index
        %bs0c0 = arith.constant 2 : index
        %bo0c0 = arith.constant 1 : index
        %co00 = arith.addi %i0, %co0c0 : index
        %co10 = arith.addi %j0, %co1c0 : index
        %ao00 = arith.addi %i0, %ao0c0 : index
        %ao10 = arith.addi %k0, %ao1c0 : index
        %bs00 = arith.muli %k0, %bs0c0 : index
        %bo00 = arith.addi %bs00, %bo0c0 : index
        %z0 = memref.load %c[%co00, %co10] : memref<17x10xf32>
        %x0 = memref.load %a[%ao00, %ao10] : memref<18x13xf32>
        %y0 = memref.load %b[%bo00, %j0] : memref<25x8xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%co00, %co10] : memref<17x10xf32>
      }
    }
  }
  return
}
func.func @scf_shifted(%a: memref<18x15xf32>, %b: memref<15x9xf32>, %c: memref<18x9xf32> {llvm.noalias}) {
  %ilo0 = arith.constant 2 : index
  %ihi0 = arith.constant 18 : index
  %ist0 = arith.constant 1 : index
  %jlo0 = arith.constant 1 : index
  %jhi0 = arith.constant 9 : index
  %jst0 = arith.constant 1 : index
  %klo0 = arith.constant 3 : index
  %khi0 = arith.constant 15 : index
  %kst0 = arith.constant 1 : index
  scf.for %i0 = %ilo0 to %ihi0 step %ist0 {
    scf.for %j0 = %jlo0 to %jhi0 step %jst0 {
      scf.for %k0 = %klo0 to %khi0 step %kst0 {
        %z0 = memref.load %c[%i0, %j0] : memref<18x9xf32>
        %x0 = memref.load %a[%i0, %k0] : memref<18x15xf32>
        %y0 = memref.load %b[%k0, %j0] : memref<15x9xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%i0, %j0] : memref<18x9xf32>
      }
    }
  }
  return
}
func.func @scf_tables_flat(%a: memref<192xf32>, %b: memref<96xf32>, %c: memref<128xf32> {llvm.noalias}) {
  %cw0 = arith.constant 8 : index
  %aw0 = arith.constant 12 : index
  %bw0 = arith.constant 8 : index
  %ilo0 = arith.constant 0 : index
  %ihi0 = arith.constant 16 : index
  %ist0 = arith.constant 1 : index
  %jlo0 = arith.constant 0 : index
  %jhi0 = arith.constant 8 : index
  %jst0 = arith.constant 1 : index
  %klo0 = arith.constant 0 : index
  %khi0 = arith.constant 12 : index
  %kst0 = arith.constant 1 : index
  scf.for %i0 = %ilo0 to %ihi0 step %ist0 {
    scf.for %j0 = %jlo0 to %jhi0 step %jst0 {
      scf.for %k0 = %klo0 to %khi0 step %kst0 {
        %cr0 = arith.muli %i0, %cw0 : index
        %cx0 = arith.addi %cr0, %j0 : index
        %ar0 = arith.muli %i0, %aw0 : index
        %ax0 = arith.addi %ar0, %k0 : index
        %br0 = arith.muli %k0, %bw0 : index
        %bx0 = arith.addi %br0, %j0 : index
        %z0 = memref.load %c[%cx0] : memref<128xf32>
        %x0 = memref.load %a[%ax0] : memref<192xf32>
        %y0 = memref.load %b[%bx0] : memref<96xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%cx0] : memref<128xf32>
      }
    }
  }
  return
}
func.func @scf_tables_perm(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %ilo0 = arith.constant 0 : index
  %ihi0 = arith.constant 16 : index
  %ist0 = arith.constant 1 : index
  %jlo0 = arith.constant 0 : index
  %jhi0 = arith.constant 8 : index
  %jst0 = arith.constant 1 : index
  %klo0 = arith.constant 0 : index
  %khi0 = arith.constant 12 : index
  %kst0 = arith.constant 1 : index
  scf.for %i0 = %ilo0 to %ihi0 step %ist0 {
    scf.for %j0 = %jlo0 to %jhi0 step %jst0 {
      scf.for %k0 = %klo0 to %khi0 step %kst0 {
        %am0c0 = arith.constant 5 : index
        %aq0c0 = arith.constant 16 : index
        %ap00 = arith.muli %i0, %am0c0 : index
        %ar00 = arith.remsi %ap00, %aq0c0 : index
        %z0 = memref.load %c[%i0, %j0] : memref<16x8xf32>
        %x0 = memref.load %a[%ar00, %k0] : memref<16x12xf32>
        %y0 = memref.load %b[%k0, %j0] : memref<12x8xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%i0, %j0] : memref<16x8xf32>
      }
    }
  }
  return
}
func.func @scf_shared(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}, %d: memref<12x8xf32>) {
  %ilo0 = arith.constant 0 : index
  %ihi0 = arith.constant 16 : index
  %ist0 = arith.constant 1 : index
  %jlo0 = arith.constant 0 : index
  %jhi0 = arith.constant 8 : index
  %jst0 = arith.constant 1 : index
  %klo0 = arith.constant 0 : index
  %khi0 = arith.constant 12 : index
  %kst0 = arith.constant 1 : index
  scf.for %i0 = %ilo0 to %ihi0 step %ist0 {
    scf.for %j0 = %jlo0 to %jhi0 step %jst0 {
      scf.for %k0 = %klo0 to %khi0 step %kst0 {
        %z0 = memref.load %c[%i0, %j0] : memref<16x8xf32>
        %x0 = memref.load %a[%i0, %k0] : memref<16x12xf32>
        %y0 = memref.load %b[%k0, %j0] : memref<12x8xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%i0, %j0] : memref<16x8xf32>
      }
    }
  }
  %ilo1 = arith.constant 0 : index
  %ihi1 = arith.constant 16 : index
  %ist1 = arith.constant 1 : index
  %jlo1 = arith.constant 0 : index
  %jhi1 = arith.constant 8 : index
  %jst1 = arith.constant 1 : index
  %klo1 = arith.constant 0 : index
  %khi1 = arith.constant 12 : index
  %kst1 = arith.constant 1 : index
  scf.for %i1 = %ilo1 to %ihi1 step %ist1 {
    scf.for %j1 = %jlo1 to %jhi1 step %jst1 {
      scf.for %k1 = %klo1 to %khi1 step %kst1 {
        %z1 = memref.load %c[%i1, %j1] : memref<16x8xf32>
        %x1 = memref.load %a[%i1, %k1] : memref<16x12xf32>
        %y1 = memref.load %d[%k1, %j1] : memref<12x8xf32>
        %m1 = arith.mulf %x1, %y1 : f32
        %s1 = arith.addf %z1, %m1 fastmath<reassoc> : f32
        memref.store %s1, %c[%i1, %j1] : memref<16x8xf32>
      }
    }
  }
  return
}
func.func @scf_chain(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %t: memref<16x8xf32> {llvm.noalias}, %d: memref<8x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %ilo0 = arith.constant 0 : index
  %ihi0 = arith.constant 16 : index
  %ist0 = arith.constant 1 : index
  %jlo0 = arith.constant 0 : index
  %jhi0 = arith.constant 8 : index
  %jst0 = arith.constant 1 : index
  %klo0 = arith.constant 0 : index
  %khi0 = arith.constant 12 : index
  %kst0 = arith.constant 1 : index
  scf.for %i0 = %ilo0 to %ihi0 step %ist0 {
    scf.for %j0 = %jlo0 to %jhi0 step %jst0 {
      scf.for %k0 = %klo0 to %khi0 step %kst0 {
        %z0 = memref.load %t[%i0, %j0] : memref<16x8xf32>
        %x0 = memref.load %a[%i0, %k0] : memref<16x12xf32>
        %y0 = memref.load %b[%k0, %j0] : memref<12x8xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %t[%i0, %j0] : memref<16x8xf32>
      }
    }
  }
  %ilo1 = arith.constant 0 : index
  %ihi1 = arith.constant 16 : index
  %ist1 = arith.constant 1 : index
  %jlo1 = arith.constant 0 : index
  %jhi1 = arith.constant 8 : index
  %jst1 = arith.constant 1 : index
  %klo1 = arith.constant 0 : index
  %khi1 = arith.constant 8 : index
  %kst1 = arith.constant 1 : index
  scf.for %i1 = %ilo1 to %ihi1 step %ist1 {
    scf.for %j1 = %jlo1 to %jhi1 step %jst1 {
      scf.for %k1 = %klo1 to %khi1 step %kst1 {
        %z1 = memref.load %c[%i1, %j1] : memref<16x8xf32>
        %x1 = memref.load %t[%i1, %k1] : memref<16x8xf32>
        %y1 = memref.load %d[%k1, %j1] : memref<8x8xf32>
        %m1 = arith.mulf %x1, %y1 : f32
        %s1 = arith.addf %z1, %m1 fastmath<reassoc> : f32
        memref.store %s1, %c[%i1, %j1] : memref<16x8xf32>
      }
    }
  }
  return
}
func.func @scf_strict(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %ilo0 = arith.constant 0 : index
  %ihi0 = arith.constant 16 : index
  %ist0 = arith.constant 1 : index
  %jlo0 = arith.constant 0 : index
  %jhi0 = arith.constant 8 : index
  %jst0 = arith.constant 1 : index
  %klo0 = arith.constant 0 : index
  %khi0 = arith.constant 12 : index
  %kst0 = arith.constant 1 : index
  scf.for %i0 = %ilo0 to %ihi0 step %ist0 {
    scf.for %j0 = %jlo0 to %jhi0 step %jst0 {
      scf.for %k0 = %klo0 to %khi0 step %kst0 {
        %z0 = memref.load %c[%i0, %j0] : memref<16x8xf32>
        %x0 = memref.load %a[%i0, %k0] : memref<16x12xf32>
        %y0 = memref.load %b[%k0, %j0] : memref<12x8xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 : f32
        memref.store %s0, %c[%i0, %j0] : memref<16x8xf32>
      }
    }
  }
  return
}
