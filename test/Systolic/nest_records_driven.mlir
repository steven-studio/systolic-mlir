// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_driven.json record-driven=true report=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=REMARK
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_driven.json record-driven=true" | FileCheck %s --check-prefix=TILES
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_driven.json record-driven=true" --lower-systolic-matmul-tile-to-fpga=async=true | FileCheck %s --check-prefix=ASYNC
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_driven.json record-driven=true" --lower-systolic-matmul-tile-to-fpga | FileCheck %s --check-prefix=SYNC
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_driven.json report=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=STAGE2
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="report=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=OFF
// RUN: not systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=NORECORDS
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_driven.json record-driven=true" --mlir-print-elementsattrs-with-hex-if-larger=-1 | FileCheck %s --check-prefix=PERMTABLE
//
// Stage 3 of docs/GENERALIZED_GEMM_SCOPE.md: GEMMs the production recognizer finds no GEMM in
// (they read and write with memref.load and memref.store, their indices computed by arith),
// lowered from their records (Systolic/NestRecordLowering.h): by the record's closed forms of
// the views, each checked against the nest's own index arithmetic, or, where the research has
// no affine closed form, by tables of the nest's addresses over its static domain, gathered and
// scattered. Inputs/nest_records_driven.json is what generalized-gemm-nest-report says of this
// file (every pair of roots authorized, 64-bit index), written and checked by
// research/generalized-gemm/test/fuzz/nest_gate_records.py. The functions are
// nest_driven_e2e.py's, which runs them; nest_records_driven_refusals.mlir has the refusals.
//
// Stage 2 first (the recognizer's GEMMs, each with its record), then stage 3, nest by nest in
// program order: four by the record's formulas, two by tables, two nests on one output, the
// stage 3 nest after a stage 2 one, and the refusal of the floating-point policy.
// REMARK: remark: nest record agrees: nest 0 of mixed
// REMARK: remark: affine GEMM 16x8x12 lowered as 5 exact region(s)
// REMARK: remark: GEMM 16x8x12 of nest 0 of formulas lowered from its record as 5 exact region(s) (formulas: the record's views of A, B and C are the nest's own index arithmetic)
// REMARK: remark: GEMM 16x8x12 of nest 0 of transposed lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 0 of flat lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 0 of offsets lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 0 of tables_flat lowered from its record as 5 exact region(s) (tables: the nest's own addresses at its 1536 iterations, A one per (I, K), B one per (K, J), C one per (I, J) and one to one; each (I, J, K) is one iteration)
// REMARK: remark: GEMM 16x8x12 of nest 0 of tables_perm lowered from its record as 5 exact region(s) (tables:
// REMARK: remark: GEMM 16x8x12 of nest 0 of shared lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 1 of shared lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 1 of mixed lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM not lowered from its record: nest 0 of strict: floating-point reassociation is not permitted
// REMARK: remark: GEMM 16x8x12 of nest 0 of tables_affine_mod lowered from its record as 5 exact region(s) (tables:
// REMARK: remark: GEMM 16x8x12 of nest 0 of formulas_apply lowered from its record as 5 exact region(s) (formulas:
// REMARK-NOT: remark:
//
// The operands: views by the formulas (whole memrefs, a transposed subview, explicit views of
// the flat arrays, subviews at the offsets and the scale); the tables as constant globals, the
// operands gathered, the tiles, C scattered back.
// TILES-DAG: memref.global "private" constant @__nest_table_nest_0_of_tables_flat_A : memref<16x12xi64>
// TILES-DAG: memref.global "private" constant @__nest_table_nest_0_of_tables_flat_B : memref<12x8xi64>
// TILES-DAG: memref.global "private" constant @__nest_table_nest_0_of_tables_flat_C : memref<16x8xi64>
// TILES-DAG: memref.global "private" constant @__nest_table_nest_0_of_tables_perm_A : memref<16x12xi64>
// TILES-LABEL: func.func @formulas
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: affine.for
// TILES: return
// TILES-LABEL: func.func @transposed
// TILES: memref.subview %arg1[0, 0] [8, 12] [1, 1]
// TILES-NEXT: memref.transpose
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: affine.for
// TILES: return
// TILES-LABEL: func.func @flat
// TILES: memref.reinterpret_cast %arg0 to offset: [0], sizes: [16, 12], strides: [12, 1]
// TILES: memref.reinterpret_cast %arg1 to offset: [0], sizes: [12, 8], strides: [8, 1]
// TILES: memref.reinterpret_cast %arg2 to offset: [0], sizes: [16, 8], strides: [8, 1]
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: affine.for
// TILES: return
// TILES-LABEL: func.func @offsets
// TILES: memref.subview %arg0[2, 1] [16, 12] [1, 1]
// TILES: memref.subview %arg1[1, 0] [12, 8] [2, 1]
// TILES: memref.subview %arg2[1, 2] [16, 8] [1, 1]
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: affine.for
// TILES: return
// TILES-LABEL: func.func @tables_flat
// TILES: %[[TA:.*]] = memref.get_global @__nest_table_nest_0_of_tables_flat_A : memref<16x12xi64>
// TILES-NEXT: %[[FA:.*]] = memref.reinterpret_cast %arg0 to offset: [0], sizes: [192], strides: [1]
// TILES-NEXT: %[[MA:.*]] = memref.alloc() : memref<16x12xf32>
// TILES: %[[TC:.*]] = memref.get_global @__nest_table_nest_0_of_tables_flat_C : memref<16x8xi64>
// TILES-NEXT: %[[FC:.*]] = memref.reinterpret_cast %arg2 to offset: [0], sizes: [128], strides: [1]
// TILES-NEXT: %[[MC:.*]] = memref.alloc() : memref<16x8xf32>
// TILES: affine.for %[[R:.*]] = 0 to 16 {
// TILES-NEXT: affine.for %[[Q:.*]] = 0 to 12 {
// TILES-NEXT: %[[O:.*]] = affine.load %[[TA]][%[[R]], %[[Q]]] : memref<16x12xi64>
// TILES-NEXT: %[[OI:.*]] = arith.index_cast %[[O]] : i64 to index
// TILES-NEXT: %[[V:.*]] = memref.load %[[FA]][%[[OI]]] : memref<192xf32>
// TILES-NEXT: affine.store %[[V]], %[[MA]][%[[R]], %[[Q]]] : memref<16x12xf32>
// TILES: memref.load %[[FC]]
// TILES: bufferization.to_tensor %[[MA]] restrict
// TILES-COUNT-5: systolic.matmul_tile
// TILES: bufferization.materialize_in_destination %{{.*}} in restrict writable %[[MC]]
// TILES-NEXT: affine.for %[[R2:.*]] = 0 to 16 {
// TILES-NEXT: affine.for %[[Q2:.*]] = 0 to 8 {
// TILES-NEXT: %[[O2:.*]] = affine.load %[[TC]][%[[R2]], %[[Q2]]] : memref<16x8xi64>
// TILES-NEXT: %[[OI2:.*]] = arith.index_cast %[[O2]] : i64 to index
// TILES-NEXT: %[[V2:.*]] = affine.load %[[MC]][%[[R2]], %[[Q2]]] : memref<16x8xf32>
// TILES-NEXT: memref.store %[[V2]], %[[FC]][%[[OI2]]] : memref<128xf32>
// TILES: return
// TILES-LABEL: func.func @tables_perm
// TILES: memref.get_global @__nest_table_nest_0_of_tables_perm_A
// TILES-COUNT-5: systolic.matmul_tile
// TILES: return
// TILES-LABEL: func.func @shared
// TILES-COUNT-10: systolic.matmul_tile
// TILES-NOT: affine.for
// TILES: return
// TILES-LABEL: func.func @mixed
// TILES-COUNT-10: systolic.matmul_tile
// TILES-NOT: affine.for
// TILES: return
// TILES-LABEL: func.func @strict
// TILES-NOT: systolic.matmul_tile
// TILES: affine.for
// TILES: return
// TILES-LABEL: func.func @tables_affine_mod
// TILES: memref.get_global @__nest_table_nest_0_of_tables_affine_mod_A
// TILES-COUNT-5: systolic.matmul_tile
// TILES: return
// TILES-LABEL: func.func @formulas_apply
// TILES: memref.subview %arg0[1, 0] [16, 12] [1, 1]
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: affine.for
// TILES: return
//
// A's table holds the rows the nest reads, in its order: rows 0, 5, 10, 15, 4, ...
// PERMTABLE: memref.global "private" constant @__nest_table_nest_0_of_tables_perm_A : memref<16x12xi64> = dense<{{\[\[}}0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11], [60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71], [120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131], [180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191], [48, 49,
//
// Asynchronously: the gathers before the GEMM begins; its wait and C's write-back before the
// scatter; each GEMM done before the next statement.
// ASYNC-LABEL: func.func @formulas
// ASYNC: llvm.call @fpga_gemm_begin
// ASYNC-COUNT-5: llvm.call @fpga_tile_submit
// ASYNC: llvm.call @fpga_gemm_wait
// ASYNC-NEXT: bufferization.materialize_in_destination {{.*}} in restrict writable %arg2
// ASYNC-LABEL: func.func @tables_perm
// ASYNC: %[[PC:.*]] = memref.reinterpret_cast %arg2 to offset: [0], sizes: [128], strides: [1]
// ASYNC-NEXT: %[[PM:.*]] = memref.alloc() : memref<16x8xf32>
// ASYNC: memref.load %[[PC]]
// ASYNC: llvm.call @fpga_gemm_begin
// ASYNC-COUNT-5: llvm.call @fpga_tile_submit
// ASYNC: llvm.call @fpga_gemm_wait
// ASYNC-NEXT: bufferization.materialize_in_destination {{.*}} in restrict writable %[[PM]]
// ASYNC-NEXT: affine.for
// ASYNC: memref.store %{{.*}}, %[[PC]]
// ASYNC-LABEL: func.func @shared
// ASYNC: llvm.call @fpga_gemm_begin
// ASYNC-COUNT-5: llvm.call @fpga_tile_submit
// ASYNC: llvm.call @fpga_gemm_wait
// ASYNC-NEXT: bufferization.materialize_in_destination {{.*}} in restrict writable %arg2
// ASYNC: llvm.call @fpga_gemm_begin
// ASYNC-COUNT-5: llvm.call @fpga_tile_submit
// ASYNC: llvm.call @fpga_gemm_wait
// ASYNC-NEXT: bufferization.materialize_in_destination {{.*}} in restrict writable %arg2
// ASYNC-LABEL: func.func @mixed
// ASYNC: llvm.call @fpga_gemm_begin
// ASYNC-COUNT-5: llvm.call @fpga_tile_submit
// ASYNC: llvm.call @fpga_gemm_wait
// ASYNC-NEXT: bufferization.materialize_in_destination {{.*}} in restrict writable %arg2
// ASYNC: llvm.call @fpga_gemm_begin
// ASYNC-COUNT-5: llvm.call @fpga_tile_submit
// ASYNC: llvm.call @fpga_gemm_wait
// ASYNC-NEXT: bufferization.materialize_in_destination {{.*}} in restrict writable %arg2
// ASYNC-LABEL: func.func @strict
// ASYNC-NOT: llvm.call
// ASYNC: affine.for
//
// SYNC-LABEL: func.func @formulas
// SYNC-COUNT-5: llvm.call @fpga_matmul_tiled_auto_scheduled
// SYNC-LABEL: func.func @tables_flat
// SYNC: affine.for
// SYNC-COUNT-5: llvm.call @fpga_matmul_tiled_auto_scheduled
// SYNC: memref.store
// SYNC-LABEL: func.func @shared
// SYNC-COUNT-10: llvm.call @fpga_matmul_tiled_auto_scheduled
// SYNC-LABEL: func.func @strict
// SYNC-NOT: llvm.call
// SYNC-LABEL: func.func @tables_affine_mod
// SYNC-COUNT-5: llvm.call @fpga_matmul_tiled_auto_scheduled
//
// Without record-driven, stage 2 alone: the recognizer's GEMM in @mixed, nothing from a record.
// STAGE2-NOT: from its record
// STAGE2: remark: nest record agrees: nest 0 of mixed
// STAGE2: remark: affine GEMM 16x8x12 lowered as 5 exact region(s)
// STAGE2-NOT: from its record
// Without records: the pass as it is; the recognizer finds its one GEMM.
// OFF-NOT: {{nest record|from its record}}
// OFF: remark: affine GEMM 16x8x12 lowered as 5 exact region(s)
// OFF-NOT: {{nest record|from its record}}
// record-driven reads nest-records' records: alone, it is an error.
// NORECORDS: error: affine-matmul-to-systolic: record-driven reads the records of nest-records, and none is given
// The research verifies the views (I, K), (K, J), (I, J); the formulas are the nest's indices.
func.func @formulas(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
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

// B stored as B[j][k], the loops in the order k, i, j: a transposed view, by formulas.
func.func @transposed(%a: memref<16x12xf32>, %b: memref<8x12xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  affine.for %k0 = 0 to 12 {
    affine.for %i0 = 0 to 16 {
      affine.for %j0 = 0 to 8 {
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

// Flat arrays, each index row * columns + column computed in the body: closed forms
// 12 I + K, 8 K + J, 8 I + J, one explicit view each.
func.func @flat(%a: memref<192xf32>, %b: memref<96xf32>, %c: memref<128xf32> {llvm.noalias}) {
  affine.for %i0 = 0 to 16 {
    affine.for %j0 = 0 to 8 {
      affine.for %k0 = 0 to 12 {
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

// Offsets and a scale computed by arith: A[i + 2][k + 1], B[2 k + 1][j], C[i + 1][j + 2].
func.func @offsets(%a: memref<18x13xf32>, %b: memref<25x8xf32>, %c: memref<17x10xf32> {llvm.noalias}) {
  affine.for %i0 = 0 to 16 {
    affine.for %j0 = 0 to 8 {
      affine.for %k0 = 0 to 12 {
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

// Flat arrays, the index constants defined before the nest: the research proposes no
// closed form for these views. The tables: gather, tiles, scatter.
func.func @tables_flat(%a: memref<192xf32>, %b: memref<96xf32>, %c: memref<128xf32> {llvm.noalias}) {
  %cw0 = arith.constant 8 : index
  %aw0 = arith.constant 12 : index
  %bw0 = arith.constant 8 : index
  affine.for %i0 = 0 to 16 {
    affine.for %j0 = 0 to 8 {
      affine.for %k0 = 0 to 12 {
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

// A's rows read in another order, (5 i) mod 16: no affine formula. The tables.
func.func @tables_perm(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  affine.for %i0 = 0 to 16 {
    affine.for %j0 = 0 to 8 {
      affine.for %k0 = 0 to 12 {
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

// Two nests on one output, each lowered from its own record, in order.
func.func @shared(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}, %d: memref<12x8xf32>) {
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
  affine.for %i1 = 0 to 16 {
    affine.for %j1 = 0 to 8 {
      affine.for %k1 = 0 to 12 {
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

// An affine nest (the production recognizer and its record, stage 2), then a nest only
// stage 3 lowers, on one output.
func.func @mixed(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}, %d: memref<12x8xf32>) {
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
  affine.for %i1 = 0 to 16 {
    affine.for %j1 = 0 to 8 {
      affine.for %k1 = 0 to 12 {
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

// No reassoc on the accumulation: the floating-point policy refuses it.
func.func @strict(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  affine.for %i0 = 0 to 16 {
    affine.for %j0 = 0 to 8 {
      affine.for %k0 = 0 to 12 {
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
// An affine.load whose map takes A's rows (5 i) mod 16: no affine form, so the tables, with
// the map evaluated at every iteration.
func.func @tables_affine_mod(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %x = affine.load %a[(%i * 5) mod 16, %k] : memref<16x12xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
// Indices by affine.apply: A[i + 1][k], C[i][j] through maps, read as forms.
func.func @formulas_apply(%a: memref<17x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %r = affine.apply affine_map<(d0) -> (d0 + 1)>(%i)
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %x = memref.load %a[%r, %k] : memref<17x12xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
