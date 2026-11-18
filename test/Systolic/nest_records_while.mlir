// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_while.json record-driven=true report=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=REMARK
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_while.json record-driven=true" | FileCheck %s --check-prefix=TILES
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_while.json record-driven=true" --lower-systolic-matmul-tile-to-fpga=async=true | FileCheck %s --check-prefix=ASYNC
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_while.json record-driven=true" --lower-systolic-matmul-tile-to-fpga | FileCheck %s --check-prefix=SYNC
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_while.json report=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=STAGE2 --allow-empty
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="report=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=OFF --allow-empty
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="report=true" | FileCheck %s --check-prefix=OFFIR --implicit-check-not=systolic.matmul_tile
//
// GEMMs written with scf.while, lowered from their records (stage 3 of
// docs/GENERALIZED_GEMM_SCOPE.md, on scf.while loops). Each scf.while is a counting loop: it
// carries one value, its counter, from a constant; its before region compares the counter with
// a constant and hands it on with scf.condition; its after region, whose argument is the
// counter the body reads, yields counter + 1. Some nests mix scf.while with scf.for and
// affine.for. The production recognizer reads affine loops only and finds no GEMM here; the
// research recognizer verifies the nests, and the lowering takes them from their records as it
// takes for loops (Systolic/NestRecordLowering.h): by the record's closed forms of the views,
// each checked against the nest's own index arithmetic, or, where the research has no affine
// closed form, by tables of the nest's addresses. The first three functions are one GEMM written
// with scf.while, with scf.for and with affine.for. Two nests stay as written: @whl_strict by
// the floating-point policy, and @whl_ne_shifted. Loops that go on by counter != bound are no
// supported form and are recorded apart: the research verifies @whl_en, whose accesses bound
// its counters, and it is lowered; it does not verify @whl_ne_shifted, which stays.
// Inputs/nest_records_while.json is what generalized-gemm-nest-report says of this file (every
// pair of roots authorized, 64-bit index), written and checked by
// research/generalized-gemm/test/fuzz/nest_gate_records.py. The functions are
// nest_while_e2e.py's, which runs them and compares every result with the original program's;
// nest_records_while_refusals.mlir has the refusals. The two functions whose loops go on by
// != are recorded apart in the scope document: they are no supported form.
//
// Nest by nest in program order: formulas, tables, the nest the research does not verify
// and the refusal of the floating-point policy. Nothing is lowered by the production
// recognizer here.
// REMARK-NOT: affine GEMM
// REMARK: remark: GEMM 16x8x12 of nest 0 of whl_gemm lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 0 of whl_twin_scf lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 0 of whl_twin_affine lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 0 of whl_inside lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 0 of whl_for_while_for lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 0 of whl_while_for_while lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 0 of whl_affine_while_for lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 0 of whl_sgt_swapped lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 0 of whl_ult_before lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 0 of whl_ugt lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 0 of whl_en lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM not lowered from its record: nest 0 of whl_ne_shifted: the record's status is NOT_VERIFIED: no candidate for the nest: the search is UNKNOWN (no candidate was verified within the limits)
// REMARK: remark: GEMM 16x8x12 of nest 0 of whl_shifted lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 0 of whl_shifted_mixed lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 0 of whl_transposed lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 0 of whl_flat lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 0 of whl_offsets lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 0 of whl_tables_flat lowered from its record as 5 exact region(s) (tables:
// REMARK: remark: GEMM 16x8x12 of nest 0 of whl_tables_perm lowered from its record as 5 exact region(s) (tables:
// REMARK: remark: GEMM 16x8x12 of nest 0 of whl_shared lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 1 of whl_shared lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 0 of whl_chain lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x8 of nest 1 of whl_chain lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM not lowered from its record: nest 0 of whl_strict: floating-point reassociation is not permitted
// REMARK-NOT: remark:
//
// Each lowered nest is gone, its tiles where it stood; the bound constants before it stay.
// TILES-DAG: memref.global "private" constant @__nest_table_nest_0_of_whl_tables_flat_A : memref<16x12xi64>
// TILES-DAG: memref.global "private" constant @__nest_table_nest_0_of_whl_tables_flat_C : memref<16x8xi64>
// TILES-DAG: memref.global "private" constant @__nest_table_nest_0_of_whl_tables_perm_A : memref<16x12xi64>
// TILES-LABEL: func.func @whl_gemm(
// TILES: bufferization.to_tensor %arg0 restrict : memref<16x12xf32>
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: {{scf\.while|scf\.for|affine\.for}}
// TILES: return
// TILES-LABEL: func.func @whl_twin_scf(
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: {{scf\.while|scf\.for|affine\.for}}
// TILES: return
// TILES-LABEL: func.func @whl_twin_affine(
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: {{scf\.while|scf\.for|affine\.for}}
// TILES: return
// TILES-LABEL: func.func @whl_inside(
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: {{scf\.while|scf\.for|affine\.for}}
// TILES: return
// TILES-LABEL: func.func @whl_for_while_for(
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: {{scf\.while|scf\.for|affine\.for}}
// TILES: return
// TILES-LABEL: func.func @whl_while_for_while(
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: {{scf\.while|scf\.for|affine\.for}}
// TILES: return
// TILES-LABEL: func.func @whl_affine_while_for(
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: {{scf\.while|scf\.for|affine\.for}}
// TILES: return
// TILES-LABEL: func.func @whl_sgt_swapped(
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: {{scf\.while|scf\.for|affine\.for}}
// TILES: return
// TILES-LABEL: func.func @whl_ult_before(
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: {{scf\.while|scf\.for|affine\.for}}
// TILES: return
// TILES-LABEL: func.func @whl_ugt(
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: {{scf\.while|scf\.for|affine\.for}}
// TILES: return
// TILES-LABEL: func.func @whl_en(
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: {{scf\.while|scf\.for|affine\.for}}
// TILES: return
// TILES-LABEL: func.func @whl_ne_shifted(
// TILES-NOT: systolic.matmul_tile
// TILES: scf.while
// TILES: return
// TILES-LABEL: func.func @whl_shifted(
// TILES: memref.subview %arg0[2, 3] [16, 12] [1, 1]
// TILES: memref.subview %arg1[3, 1] [12, 8] [1, 1]
// TILES: memref.subview %arg2[2, 1] [16, 8] [1, 1]
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: {{scf\.while|scf\.for|affine\.for}}
// TILES: return
// TILES-LABEL: func.func @whl_shifted_mixed(
// TILES: memref.subview %arg0[2, 3] [16, 12] [1, 1]
// TILES: memref.subview %arg1[3, 1] [12, 8] [1, 1]
// TILES: memref.subview %arg2[2, 1] [16, 8] [1, 1]
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: {{scf\.while|scf\.for|affine\.for}}
// TILES: return
// TILES-LABEL: func.func @whl_transposed(
// TILES: memref.subview %arg1[0, 0] [8, 12] [1, 1]
// TILES-NEXT: memref.transpose
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: {{scf\.while|scf\.for|affine\.for}}
// TILES: return
// TILES-LABEL: func.func @whl_flat(
// TILES: memref.reinterpret_cast %arg0 to offset: [0], sizes: [16, 12], strides: [12, 1]
// TILES: memref.reinterpret_cast %arg1 to offset: [0], sizes: [12, 8], strides: [8, 1]
// TILES: memref.reinterpret_cast %arg2 to offset: [0], sizes: [16, 8], strides: [8, 1]
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: {{scf\.while|scf\.for|affine\.for}}
// TILES: return
// TILES-LABEL: func.func @whl_offsets(
// TILES: memref.subview %arg0[2, 1] [16, 12] [1, 1]
// TILES: memref.subview %arg1[1, 0] [12, 8] [2, 1]
// TILES: memref.subview %arg2[1, 2] [16, 8] [1, 1]
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: {{scf\.while|scf\.for|affine\.for}}
// TILES: return
// TILES-LABEL: func.func @whl_tables_flat(
// TILES: memref.get_global @__nest_table_nest_0_of_whl_tables_flat_A
// TILES-COUNT-5: systolic.matmul_tile
// TILES: return
// TILES-LABEL: func.func @whl_tables_perm(
// TILES: memref.get_global @__nest_table_nest_0_of_whl_tables_perm_A
// TILES-COUNT-5: systolic.matmul_tile
// TILES: return
// TILES-LABEL: func.func @whl_shared(
// TILES-COUNT-10: systolic.matmul_tile
// TILES-NOT: {{scf\.while|scf\.for|affine\.for}}
// TILES: return
// TILES-LABEL: func.func @whl_chain(
// TILES: bufferization.to_tensor %arg2 restrict writable
// TILES-COUNT-5: systolic.matmul_tile
// TILES: bufferization.materialize_in_destination {{.*}} in restrict writable %arg2
// TILES: bufferization.to_tensor %arg2 restrict :
// TILES: bufferization.to_tensor %arg4 restrict writable
// TILES-COUNT-5: systolic.matmul_tile
// TILES: bufferization.materialize_in_destination {{.*}} in restrict writable %arg4
// TILES-NOT: {{scf\.while|scf\.for|affine\.for}}
// TILES: return
// TILES-LABEL: func.func @whl_strict(
// TILES-NOT: systolic.matmul_tile
// TILES: scf.while
// TILES: return
//
// Asynchronously: each GEMM begins, submits its tiles, is waited for and written back before
// the next statement; in @whl_chain the second GEMM begins after T is written back.
// ASYNC-LABEL: func.func @whl_gemm(
// ASYNC: llvm.call @fpga_gemm_begin
// ASYNC-COUNT-5: llvm.call @fpga_tile_submit
// ASYNC: llvm.call @fpga_gemm_wait
// ASYNC-NEXT: bufferization.materialize_in_destination {{.*}} in restrict writable %arg2
// ASYNC-LABEL: func.func @whl_while_for_while(
// ASYNC: llvm.call @fpga_gemm_begin
// ASYNC-COUNT-5: llvm.call @fpga_tile_submit
// ASYNC: llvm.call @fpga_gemm_wait
// ASYNC-LABEL: func.func @whl_tables_perm(
// ASYNC: %[[PC:.*]] = memref.reinterpret_cast %arg2 to offset: [0], sizes: [128], strides: [1]
// ASYNC-NEXT: %[[PM:.*]] = memref.alloc() : memref<16x8xf32>
// ASYNC: memref.load %[[PC]]
// ASYNC: llvm.call @fpga_gemm_begin
// ASYNC-COUNT-5: llvm.call @fpga_tile_submit
// ASYNC: llvm.call @fpga_gemm_wait
// ASYNC-NEXT: bufferization.materialize_in_destination {{.*}} in restrict writable %[[PM]]
// ASYNC-NEXT: affine.for
// ASYNC: memref.store %{{.*}}, %[[PC]]
// ASYNC-LABEL: func.func @whl_chain(
// ASYNC: llvm.call @fpga_gemm_begin
// ASYNC-COUNT-5: llvm.call @fpga_tile_submit
// ASYNC: llvm.call @fpga_gemm_wait
// ASYNC-NEXT: bufferization.materialize_in_destination {{.*}} in restrict writable %arg2
// ASYNC: llvm.call @fpga_gemm_begin
// ASYNC-COUNT-5: llvm.call @fpga_tile_submit
// ASYNC: llvm.call @fpga_gemm_wait
// ASYNC-NEXT: bufferization.materialize_in_destination {{.*}} in restrict writable %arg4
// ASYNC-LABEL: func.func @whl_strict(
// ASYNC-NOT: llvm.call
// ASYNC: scf.while
//
// SYNC-LABEL: func.func @whl_gemm(
// SYNC-COUNT-5: llvm.call @fpga_matmul_tiled_auto_scheduled
// SYNC-LABEL: func.func @whl_tables_flat(
// SYNC: affine.for
// SYNC-COUNT-5: llvm.call @fpga_matmul_tiled_auto_scheduled
// SYNC: memref.store
// SYNC-LABEL: func.func @whl_shared(
// SYNC-COUNT-10: llvm.call @fpga_matmul_tiled_auto_scheduled
// SYNC-LABEL: func.func @whl_chain(
// SYNC-COUNT-5: llvm.call @fpga_matmul_tiled_auto_scheduled
// SYNC: bufferization.materialize_in_destination {{.*}} in restrict writable %arg2
// SYNC-COUNT-5: llvm.call @fpga_matmul_tiled_auto_scheduled
// SYNC: bufferization.materialize_in_destination {{.*}} in restrict writable %arg4
// SYNC-LABEL: func.func @whl_strict(
// SYNC-NOT: llvm.call
//
// Without record-driven nothing is lowered (no nest here is the production recognizer's), with
// the records or without them, and the scf.while nests stay as written.
// STAGE2-NOT: remark:
// OFF-NOT: remark:
// OFFIR-LABEL: func.func @whl_gemm(
// OFFIR: scf.while
// OFFIR: scf.condition
// OFFIR: scf.while
// OFFIR: scf.while
// OFFIR: scf.yield
// OFFIR-LABEL: func.func @whl_for_while_for(
// OFFIR: scf.for
// OFFIR: scf.while
// OFFIR: scf.for
func.func @whl_gemm(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %ilo0 = arith.constant 0 : index
  %ihi0 = arith.constant 16 : index
  %ist0 = arith.constant 1 : index
  %jlo0 = arith.constant 0 : index
  %jhi0 = arith.constant 8 : index
  %jst0 = arith.constant 1 : index
  %klo0 = arith.constant 0 : index
  %khi0 = arith.constant 12 : index
  %kst0 = arith.constant 1 : index
  %ires0 = scf.while (%iw0 = %ilo0) : (index) -> index {
    %icmp0 = arith.cmpi slt, %iw0, %ihi0 : index
    scf.condition(%icmp0) %iw0 : index
  } do {
  ^bb0(%i0: index):
    %jres0 = scf.while (%jw0 = %jlo0) : (index) -> index {
      %jcmp0 = arith.cmpi slt, %jw0, %jhi0 : index
      scf.condition(%jcmp0) %jw0 : index
    } do {
    ^bb0(%j0: index):
      %kres0 = scf.while (%kw0 = %klo0) : (index) -> index {
        %kcmp0 = arith.cmpi slt, %kw0, %khi0 : index
        scf.condition(%kcmp0) %kw0 : index
      } do {
      ^bb0(%k0: index):
        %z0 = memref.load %c[%i0, %j0] : memref<16x8xf32>
        %x0 = memref.load %a[%i0, %k0] : memref<16x12xf32>
        %y0 = memref.load %b[%k0, %j0] : memref<12x8xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%i0, %j0] : memref<16x8xf32>
        %knx0 = arith.addi %k0, %kst0 : index
        scf.yield %knx0 : index
      }
      %jnx0 = arith.addi %j0, %jst0 : index
      scf.yield %jnx0 : index
    }
    %inx0 = arith.addi %i0, %ist0 : index
    scf.yield %inx0 : index
  }
  return
}
func.func @whl_twin_scf(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
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
func.func @whl_twin_affine(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
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
func.func @whl_inside(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %ilo0 = arith.constant 0 : index
  %ihi0 = arith.constant 16 : index
  %ist0 = arith.constant 1 : index
  %ires0 = scf.while (%iw0 = %ilo0) : (index) -> index {
    %icmp0 = arith.cmpi slt, %iw0, %ihi0 : index
    scf.condition(%icmp0) %iw0 : index
  } do {
  ^bb0(%i0: index):
    %jlo0 = arith.constant 0 : index
    %jhi0 = arith.constant 8 : index
    %jst0 = arith.constant 1 : index
    %jres0 = scf.while (%jw0 = %jlo0) : (index) -> index {
      %jcmp0 = arith.cmpi slt, %jw0, %jhi0 : index
      scf.condition(%jcmp0) %jw0 : index
    } do {
    ^bb0(%j0: index):
      %klo0 = arith.constant 0 : index
      %khi0 = arith.constant 12 : index
      %kst0 = arith.constant 1 : index
      %kres0 = scf.while (%kw0 = %klo0) : (index) -> index {
        %kcmp0 = arith.cmpi slt, %kw0, %khi0 : index
        scf.condition(%kcmp0) %kw0 : index
      } do {
      ^bb0(%k0: index):
        %z0 = memref.load %c[%i0, %j0] : memref<16x8xf32>
        %x0 = memref.load %a[%i0, %k0] : memref<16x12xf32>
        %y0 = memref.load %b[%k0, %j0] : memref<12x8xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%i0, %j0] : memref<16x8xf32>
        %knx0 = arith.addi %k0, %kst0 : index
        scf.yield %knx0 : index
      }
      %jnx0 = arith.addi %j0, %jst0 : index
      scf.yield %jnx0 : index
    }
    %inx0 = arith.addi %i0, %ist0 : index
    scf.yield %inx0 : index
  }
  return
}
func.func @whl_for_while_for(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
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
    %jres0 = scf.while (%jw0 = %jlo0) : (index) -> index {
      %jcmp0 = arith.cmpi slt, %jw0, %jhi0 : index
      scf.condition(%jcmp0) %jw0 : index
    } do {
    ^bb0(%j0: index):
      scf.for %k0 = %klo0 to %khi0 step %kst0 {
        %z0 = memref.load %c[%i0, %j0] : memref<16x8xf32>
        %x0 = memref.load %a[%i0, %k0] : memref<16x12xf32>
        %y0 = memref.load %b[%k0, %j0] : memref<12x8xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%i0, %j0] : memref<16x8xf32>
      }
      %jnx0 = arith.addi %j0, %jst0 : index
      scf.yield %jnx0 : index
    }
  }
  return
}
func.func @whl_while_for_while(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %ilo0 = arith.constant 0 : index
  %ihi0 = arith.constant 16 : index
  %ist0 = arith.constant 1 : index
  %jlo0 = arith.constant 0 : index
  %jhi0 = arith.constant 8 : index
  %jst0 = arith.constant 1 : index
  %klo0 = arith.constant 0 : index
  %khi0 = arith.constant 12 : index
  %kst0 = arith.constant 1 : index
  %ires0 = scf.while (%iw0 = %ilo0) : (index) -> index {
    %icmp0 = arith.cmpi slt, %iw0, %ihi0 : index
    scf.condition(%icmp0) %iw0 : index
  } do {
  ^bb0(%i0: index):
    scf.for %j0 = %jlo0 to %jhi0 step %jst0 {
      %kres0 = scf.while (%kw0 = %klo0) : (index) -> index {
        %kcmp0 = arith.cmpi slt, %kw0, %khi0 : index
        scf.condition(%kcmp0) %kw0 : index
      } do {
      ^bb0(%k0: index):
        %z0 = memref.load %c[%i0, %j0] : memref<16x8xf32>
        %x0 = memref.load %a[%i0, %k0] : memref<16x12xf32>
        %y0 = memref.load %b[%k0, %j0] : memref<12x8xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%i0, %j0] : memref<16x8xf32>
        %knx0 = arith.addi %k0, %kst0 : index
        scf.yield %knx0 : index
      }
    }
    %inx0 = arith.addi %i0, %ist0 : index
    scf.yield %inx0 : index
  }
  return
}
func.func @whl_affine_while_for(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %jlo0 = arith.constant 0 : index
  %jhi0 = arith.constant 8 : index
  %jst0 = arith.constant 1 : index
  %klo0 = arith.constant 0 : index
  %khi0 = arith.constant 12 : index
  %kst0 = arith.constant 1 : index
  affine.for %i0 = 0 to 16 {
    %jres0 = scf.while (%jw0 = %jlo0) : (index) -> index {
      %jcmp0 = arith.cmpi slt, %jw0, %jhi0 : index
      scf.condition(%jcmp0) %jw0 : index
    } do {
    ^bb0(%j0: index):
      scf.for %k0 = %klo0 to %khi0 step %kst0 {
        %z0 = memref.load %c[%i0, %j0] : memref<16x8xf32>
        %x0 = memref.load %a[%i0, %k0] : memref<16x12xf32>
        %y0 = memref.load %b[%k0, %j0] : memref<12x8xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%i0, %j0] : memref<16x8xf32>
      }
      %jnx0 = arith.addi %j0, %jst0 : index
      scf.yield %jnx0 : index
    }
  }
  return
}
func.func @whl_sgt_swapped(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %ilo0 = arith.constant 0 : index
  %ihi0 = arith.constant 16 : index
  %ist0 = arith.constant 1 : index
  %jlo0 = arith.constant 0 : index
  %jhi0 = arith.constant 8 : index
  %jst0 = arith.constant 1 : index
  %klo0 = arith.constant 0 : index
  %khi0 = arith.constant 12 : index
  %kst0 = arith.constant 1 : index
  %ires0 = scf.while (%iw0 = %ilo0) : (index) -> index {
    %icmp0 = arith.cmpi sgt, %ihi0, %iw0 : index
    scf.condition(%icmp0) %iw0 : index
  } do {
  ^bb0(%i0: index):
    %jres0 = scf.while (%jw0 = %jlo0) : (index) -> index {
      %jcmp0 = arith.cmpi sgt, %jhi0, %jw0 : index
      scf.condition(%jcmp0) %jw0 : index
    } do {
    ^bb0(%j0: index):
      %kres0 = scf.while (%kw0 = %klo0) : (index) -> index {
        %kcmp0 = arith.cmpi sgt, %khi0, %kw0 : index
        scf.condition(%kcmp0) %kw0 : index
      } do {
      ^bb0(%k0: index):
        %z0 = memref.load %c[%i0, %j0] : memref<16x8xf32>
        %x0 = memref.load %a[%i0, %k0] : memref<16x12xf32>
        %y0 = memref.load %b[%k0, %j0] : memref<12x8xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%i0, %j0] : memref<16x8xf32>
        %knx0 = arith.addi %kst0, %k0 : index
        scf.yield %knx0 : index
      }
      %jnx0 = arith.addi %jst0, %j0 : index
      scf.yield %jnx0 : index
    }
    %inx0 = arith.addi %ist0, %i0 : index
    scf.yield %inx0 : index
  }
  return
}
func.func @whl_ult_before(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %ilo0 = arith.constant 0 : index
  %ihi0 = arith.constant 16 : index
  %ist0 = arith.constant 1 : index
  %jlo0 = arith.constant 0 : index
  %jhi0 = arith.constant 8 : index
  %jst0 = arith.constant 1 : index
  %klo0 = arith.constant 0 : index
  %khi0 = arith.constant 12 : index
  %kst0 = arith.constant 1 : index
  %ires0 = scf.while (%iw0 = %ilo0) : (index) -> index {
    %ihb0 = arith.constant 16 : index
    %icmp0 = arith.cmpi ult, %iw0, %ihb0 : index
    scf.condition(%icmp0) %iw0 : index
  } do {
  ^bb0(%i0: index):
    %jres0 = scf.while (%jw0 = %jlo0) : (index) -> index {
      %jhb0 = arith.constant 8 : index
      %jcmp0 = arith.cmpi ult, %jw0, %jhb0 : index
      scf.condition(%jcmp0) %jw0 : index
    } do {
    ^bb0(%j0: index):
      %kres0 = scf.while (%kw0 = %klo0) : (index) -> index {
        %khb0 = arith.constant 12 : index
        %kcmp0 = arith.cmpi ult, %kw0, %khb0 : index
        scf.condition(%kcmp0) %kw0 : index
      } do {
      ^bb0(%k0: index):
        %z0 = memref.load %c[%i0, %j0] : memref<16x8xf32>
        %x0 = memref.load %a[%i0, %k0] : memref<16x12xf32>
        %y0 = memref.load %b[%k0, %j0] : memref<12x8xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%i0, %j0] : memref<16x8xf32>
        %knx0 = arith.addi %k0, %kst0 : index
        scf.yield %knx0 : index
      }
      %jnx0 = arith.addi %j0, %jst0 : index
      scf.yield %jnx0 : index
    }
    %inx0 = arith.addi %i0, %ist0 : index
    scf.yield %inx0 : index
  }
  return
}
func.func @whl_ugt(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %ilo0 = arith.constant 0 : index
  %ihi0 = arith.constant 16 : index
  %ist0 = arith.constant 1 : index
  %jlo0 = arith.constant 0 : index
  %jhi0 = arith.constant 8 : index
  %jst0 = arith.constant 1 : index
  %klo0 = arith.constant 0 : index
  %khi0 = arith.constant 12 : index
  %kst0 = arith.constant 1 : index
  %ires0 = scf.while (%iw0 = %ilo0) : (index) -> index {
    %icmp0 = arith.cmpi ugt, %ihi0, %iw0 : index
    scf.condition(%icmp0) %iw0 : index
  } do {
  ^bb0(%i0: index):
    %jres0 = scf.while (%jw0 = %jlo0) : (index) -> index {
      %jcmp0 = arith.cmpi ugt, %jhi0, %jw0 : index
      scf.condition(%jcmp0) %jw0 : index
    } do {
    ^bb0(%j0: index):
      %kres0 = scf.while (%kw0 = %klo0) : (index) -> index {
        %kcmp0 = arith.cmpi ugt, %khi0, %kw0 : index
        scf.condition(%kcmp0) %kw0 : index
      } do {
      ^bb0(%k0: index):
        %z0 = memref.load %c[%i0, %j0] : memref<16x8xf32>
        %x0 = memref.load %a[%i0, %k0] : memref<16x12xf32>
        %y0 = memref.load %b[%k0, %j0] : memref<12x8xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%i0, %j0] : memref<16x8xf32>
        %knx0 = arith.addi %k0, %kst0 : index
        scf.yield %knx0 : index
      }
      %jnx0 = arith.addi %j0, %jst0 : index
      scf.yield %jnx0 : index
    }
    %inx0 = arith.addi %i0, %ist0 : index
    scf.yield %inx0 : index
  }
  return
}
func.func @whl_en(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %ilo0 = arith.constant 0 : index
  %ihi0 = arith.constant 16 : index
  %ist0 = arith.constant 1 : index
  %jlo0 = arith.constant 0 : index
  %jhi0 = arith.constant 8 : index
  %jst0 = arith.constant 1 : index
  %klo0 = arith.constant 0 : index
  %khi0 = arith.constant 12 : index
  %kst0 = arith.constant 1 : index
  %ires0 = scf.while (%iw0 = %ilo0) : (index) -> index {
    %icmp0 = arith.cmpi ne, %ihi0, %iw0 : index
    scf.condition(%icmp0) %iw0 : index
  } do {
  ^bb0(%i0: index):
    %jres0 = scf.while (%jw0 = %jlo0) : (index) -> index {
      %jcmp0 = arith.cmpi ne, %jhi0, %jw0 : index
      scf.condition(%jcmp0) %jw0 : index
    } do {
    ^bb0(%j0: index):
      %kres0 = scf.while (%kw0 = %klo0) : (index) -> index {
        %kcmp0 = arith.cmpi ne, %khi0, %kw0 : index
        scf.condition(%kcmp0) %kw0 : index
      } do {
      ^bb0(%k0: index):
        %z0 = memref.load %c[%i0, %j0] : memref<16x8xf32>
        %x0 = memref.load %a[%i0, %k0] : memref<16x12xf32>
        %y0 = memref.load %b[%k0, %j0] : memref<12x8xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%i0, %j0] : memref<16x8xf32>
        %knx0 = arith.addi %k0, %kst0 : index
        scf.yield %knx0 : index
      }
      %jnx0 = arith.addi %j0, %jst0 : index
      scf.yield %jnx0 : index
    }
    %inx0 = arith.addi %i0, %ist0 : index
    scf.yield %inx0 : index
  }
  return
}
func.func @whl_ne_shifted(%a: memref<18x15xf32>, %b: memref<15x9xf32>, %c: memref<18x9xf32> {llvm.noalias}) {
  %ilo0 = arith.constant 2 : index
  %ihi0 = arith.constant 18 : index
  %ist0 = arith.constant 1 : index
  %jlo0 = arith.constant 1 : index
  %jhi0 = arith.constant 9 : index
  %jst0 = arith.constant 1 : index
  %klo0 = arith.constant 3 : index
  %khi0 = arith.constant 15 : index
  %kst0 = arith.constant 1 : index
  %ires0 = scf.while (%iw0 = %ilo0) : (index) -> index {
    %icmp0 = arith.cmpi ne, %iw0, %ihi0 : index
    scf.condition(%icmp0) %iw0 : index
  } do {
  ^bb0(%i0: index):
    %jres0 = scf.while (%jw0 = %jlo0) : (index) -> index {
      %jcmp0 = arith.cmpi ne, %jw0, %jhi0 : index
      scf.condition(%jcmp0) %jw0 : index
    } do {
    ^bb0(%j0: index):
      %kres0 = scf.while (%kw0 = %klo0) : (index) -> index {
        %kcmp0 = arith.cmpi ne, %kw0, %khi0 : index
        scf.condition(%kcmp0) %kw0 : index
      } do {
      ^bb0(%k0: index):
        %z0 = memref.load %c[%i0, %j0] : memref<18x9xf32>
        %x0 = memref.load %a[%i0, %k0] : memref<18x15xf32>
        %y0 = memref.load %b[%k0, %j0] : memref<15x9xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%i0, %j0] : memref<18x9xf32>
        %knx0 = arith.addi %k0, %kst0 : index
        scf.yield %knx0 : index
      }
      %jnx0 = arith.addi %j0, %jst0 : index
      scf.yield %jnx0 : index
    }
    %inx0 = arith.addi %i0, %ist0 : index
    scf.yield %inx0 : index
  }
  return
}
func.func @whl_shifted(%a: memref<18x15xf32>, %b: memref<15x9xf32>, %c: memref<18x9xf32> {llvm.noalias}) {
  %ilo0 = arith.constant 2 : index
  %ihi0 = arith.constant 18 : index
  %ist0 = arith.constant 1 : index
  %jlo0 = arith.constant 1 : index
  %jhi0 = arith.constant 9 : index
  %jst0 = arith.constant 1 : index
  %klo0 = arith.constant 3 : index
  %khi0 = arith.constant 15 : index
  %kst0 = arith.constant 1 : index
  %ires0 = scf.while (%iw0 = %ilo0) : (index) -> index {
    %icmp0 = arith.cmpi slt, %iw0, %ihi0 : index
    scf.condition(%icmp0) %iw0 : index
  } do {
  ^bb0(%i0: index):
    %jres0 = scf.while (%jw0 = %jlo0) : (index) -> index {
      %jcmp0 = arith.cmpi slt, %jw0, %jhi0 : index
      scf.condition(%jcmp0) %jw0 : index
    } do {
    ^bb0(%j0: index):
      %kres0 = scf.while (%kw0 = %klo0) : (index) -> index {
        %kcmp0 = arith.cmpi slt, %kw0, %khi0 : index
        scf.condition(%kcmp0) %kw0 : index
      } do {
      ^bb0(%k0: index):
        %z0 = memref.load %c[%i0, %j0] : memref<18x9xf32>
        %x0 = memref.load %a[%i0, %k0] : memref<18x15xf32>
        %y0 = memref.load %b[%k0, %j0] : memref<15x9xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%i0, %j0] : memref<18x9xf32>
        %knx0 = arith.addi %k0, %kst0 : index
        scf.yield %knx0 : index
      }
      %jnx0 = arith.addi %j0, %jst0 : index
      scf.yield %jnx0 : index
    }
    %inx0 = arith.addi %i0, %ist0 : index
    scf.yield %inx0 : index
  }
  return
}
func.func @whl_shifted_mixed(%a: memref<18x15xf32>, %b: memref<15x9xf32>, %c: memref<18x9xf32> {llvm.noalias}) {
  %ilo0 = arith.constant 2 : index
  %ihi0 = arith.constant 18 : index
  %ist0 = arith.constant 1 : index
  %jlo0 = arith.constant 1 : index
  %jhi0 = arith.constant 9 : index
  %jst0 = arith.constant 1 : index
  %klo0 = arith.constant 3 : index
  %khi0 = arith.constant 15 : index
  %kst0 = arith.constant 1 : index
  %ires0 = scf.while (%iw0 = %ilo0) : (index) -> index {
    %icmp0 = arith.cmpi slt, %iw0, %ihi0 : index
    scf.condition(%icmp0) %iw0 : index
  } do {
  ^bb0(%i0: index):
    scf.for %j0 = %jlo0 to %jhi0 step %jst0 {
      %kres0 = scf.while (%kw0 = %klo0) : (index) -> index {
        %kcmp0 = arith.cmpi slt, %kw0, %khi0 : index
        scf.condition(%kcmp0) %kw0 : index
      } do {
      ^bb0(%k0: index):
        %z0 = memref.load %c[%i0, %j0] : memref<18x9xf32>
        %x0 = memref.load %a[%i0, %k0] : memref<18x15xf32>
        %y0 = memref.load %b[%k0, %j0] : memref<15x9xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%i0, %j0] : memref<18x9xf32>
        %knx0 = arith.addi %k0, %kst0 : index
        scf.yield %knx0 : index
      }
    }
    %inx0 = arith.addi %i0, %ist0 : index
    scf.yield %inx0 : index
  }
  return
}
func.func @whl_transposed(%a: memref<16x12xf32>, %b: memref<8x12xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %klo0 = arith.constant 0 : index
  %khi0 = arith.constant 12 : index
  %kst0 = arith.constant 1 : index
  %ilo0 = arith.constant 0 : index
  %ihi0 = arith.constant 16 : index
  %ist0 = arith.constant 1 : index
  %jlo0 = arith.constant 0 : index
  %jhi0 = arith.constant 8 : index
  %jst0 = arith.constant 1 : index
  %kres0 = scf.while (%kw0 = %klo0) : (index) -> index {
    %kcmp0 = arith.cmpi slt, %kw0, %khi0 : index
    scf.condition(%kcmp0) %kw0 : index
  } do {
  ^bb0(%k0: index):
    %ires0 = scf.while (%iw0 = %ilo0) : (index) -> index {
      %icmp0 = arith.cmpi slt, %iw0, %ihi0 : index
      scf.condition(%icmp0) %iw0 : index
    } do {
    ^bb0(%i0: index):
      %jres0 = scf.while (%jw0 = %jlo0) : (index) -> index {
        %jcmp0 = arith.cmpi slt, %jw0, %jhi0 : index
        scf.condition(%jcmp0) %jw0 : index
      } do {
      ^bb0(%j0: index):
        %z0 = memref.load %c[%i0, %j0] : memref<16x8xf32>
        %x0 = memref.load %a[%i0, %k0] : memref<16x12xf32>
        %y0 = memref.load %b[%j0, %k0] : memref<8x12xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%i0, %j0] : memref<16x8xf32>
        %jnx0 = arith.addi %j0, %jst0 : index
        scf.yield %jnx0 : index
      }
      %inx0 = arith.addi %i0, %ist0 : index
      scf.yield %inx0 : index
    }
    %knx0 = arith.addi %k0, %kst0 : index
    scf.yield %knx0 : index
  }
  return
}
func.func @whl_flat(%a: memref<192xf32>, %b: memref<96xf32>, %c: memref<128xf32> {llvm.noalias}) {
  %ilo0 = arith.constant 0 : index
  %ihi0 = arith.constant 16 : index
  %ist0 = arith.constant 1 : index
  %jlo0 = arith.constant 0 : index
  %jhi0 = arith.constant 8 : index
  %jst0 = arith.constant 1 : index
  %klo0 = arith.constant 0 : index
  %khi0 = arith.constant 12 : index
  %kst0 = arith.constant 1 : index
  %ires0 = scf.while (%iw0 = %ilo0) : (index) -> index {
    %icmp0 = arith.cmpi slt, %iw0, %ihi0 : index
    scf.condition(%icmp0) %iw0 : index
  } do {
  ^bb0(%i0: index):
    %jres0 = scf.while (%jw0 = %jlo0) : (index) -> index {
      %jcmp0 = arith.cmpi slt, %jw0, %jhi0 : index
      scf.condition(%jcmp0) %jw0 : index
    } do {
    ^bb0(%j0: index):
      %kres0 = scf.while (%kw0 = %klo0) : (index) -> index {
        %kcmp0 = arith.cmpi slt, %kw0, %khi0 : index
        scf.condition(%kcmp0) %kw0 : index
      } do {
      ^bb0(%k0: index):
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
        %knx0 = arith.addi %k0, %kst0 : index
        scf.yield %knx0 : index
      }
      %jnx0 = arith.addi %j0, %jst0 : index
      scf.yield %jnx0 : index
    }
    %inx0 = arith.addi %i0, %ist0 : index
    scf.yield %inx0 : index
  }
  return
}
func.func @whl_offsets(%a: memref<18x13xf32>, %b: memref<25x8xf32>, %c: memref<17x10xf32> {llvm.noalias}) {
  %ilo0 = arith.constant 0 : index
  %ihi0 = arith.constant 16 : index
  %ist0 = arith.constant 1 : index
  %jlo0 = arith.constant 0 : index
  %jhi0 = arith.constant 8 : index
  %jst0 = arith.constant 1 : index
  %klo0 = arith.constant 0 : index
  %khi0 = arith.constant 12 : index
  %kst0 = arith.constant 1 : index
  %ires0 = scf.while (%iw0 = %ilo0) : (index) -> index {
    %icmp0 = arith.cmpi slt, %iw0, %ihi0 : index
    scf.condition(%icmp0) %iw0 : index
  } do {
  ^bb0(%i0: index):
    %jres0 = scf.while (%jw0 = %jlo0) : (index) -> index {
      %jcmp0 = arith.cmpi slt, %jw0, %jhi0 : index
      scf.condition(%jcmp0) %jw0 : index
    } do {
    ^bb0(%j0: index):
      %kres0 = scf.while (%kw0 = %klo0) : (index) -> index {
        %kcmp0 = arith.cmpi slt, %kw0, %khi0 : index
        scf.condition(%kcmp0) %kw0 : index
      } do {
      ^bb0(%k0: index):
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
        %knx0 = arith.addi %k0, %kst0 : index
        scf.yield %knx0 : index
      }
      %jnx0 = arith.addi %j0, %jst0 : index
      scf.yield %jnx0 : index
    }
    %inx0 = arith.addi %i0, %ist0 : index
    scf.yield %inx0 : index
  }
  return
}
func.func @whl_tables_flat(%a: memref<192xf32>, %b: memref<96xf32>, %c: memref<128xf32> {llvm.noalias}) {
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
  %ires0 = scf.while (%iw0 = %ilo0) : (index) -> index {
    %icmp0 = arith.cmpi slt, %iw0, %ihi0 : index
    scf.condition(%icmp0) %iw0 : index
  } do {
  ^bb0(%i0: index):
    %jres0 = scf.while (%jw0 = %jlo0) : (index) -> index {
      %jcmp0 = arith.cmpi slt, %jw0, %jhi0 : index
      scf.condition(%jcmp0) %jw0 : index
    } do {
    ^bb0(%j0: index):
      %kres0 = scf.while (%kw0 = %klo0) : (index) -> index {
        %kcmp0 = arith.cmpi slt, %kw0, %khi0 : index
        scf.condition(%kcmp0) %kw0 : index
      } do {
      ^bb0(%k0: index):
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
        %knx0 = arith.addi %k0, %kst0 : index
        scf.yield %knx0 : index
      }
      %jnx0 = arith.addi %j0, %jst0 : index
      scf.yield %jnx0 : index
    }
    %inx0 = arith.addi %i0, %ist0 : index
    scf.yield %inx0 : index
  }
  return
}
func.func @whl_tables_perm(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %ilo0 = arith.constant 0 : index
  %ihi0 = arith.constant 16 : index
  %ist0 = arith.constant 1 : index
  %jlo0 = arith.constant 0 : index
  %jhi0 = arith.constant 8 : index
  %jst0 = arith.constant 1 : index
  %klo0 = arith.constant 0 : index
  %khi0 = arith.constant 12 : index
  %kst0 = arith.constant 1 : index
  %ires0 = scf.while (%iw0 = %ilo0) : (index) -> index {
    %icmp0 = arith.cmpi slt, %iw0, %ihi0 : index
    scf.condition(%icmp0) %iw0 : index
  } do {
  ^bb0(%i0: index):
    %jres0 = scf.while (%jw0 = %jlo0) : (index) -> index {
      %jcmp0 = arith.cmpi slt, %jw0, %jhi0 : index
      scf.condition(%jcmp0) %jw0 : index
    } do {
    ^bb0(%j0: index):
      %kres0 = scf.while (%kw0 = %klo0) : (index) -> index {
        %kcmp0 = arith.cmpi slt, %kw0, %khi0 : index
        scf.condition(%kcmp0) %kw0 : index
      } do {
      ^bb0(%k0: index):
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
        %knx0 = arith.addi %k0, %kst0 : index
        scf.yield %knx0 : index
      }
      %jnx0 = arith.addi %j0, %jst0 : index
      scf.yield %jnx0 : index
    }
    %inx0 = arith.addi %i0, %ist0 : index
    scf.yield %inx0 : index
  }
  return
}
func.func @whl_shared(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}, %d: memref<12x8xf32>) {
  %ilo0 = arith.constant 0 : index
  %ihi0 = arith.constant 16 : index
  %ist0 = arith.constant 1 : index
  %jlo0 = arith.constant 0 : index
  %jhi0 = arith.constant 8 : index
  %jst0 = arith.constant 1 : index
  %klo0 = arith.constant 0 : index
  %khi0 = arith.constant 12 : index
  %kst0 = arith.constant 1 : index
  %ires0 = scf.while (%iw0 = %ilo0) : (index) -> index {
    %icmp0 = arith.cmpi slt, %iw0, %ihi0 : index
    scf.condition(%icmp0) %iw0 : index
  } do {
  ^bb0(%i0: index):
    %jres0 = scf.while (%jw0 = %jlo0) : (index) -> index {
      %jcmp0 = arith.cmpi slt, %jw0, %jhi0 : index
      scf.condition(%jcmp0) %jw0 : index
    } do {
    ^bb0(%j0: index):
      %kres0 = scf.while (%kw0 = %klo0) : (index) -> index {
        %kcmp0 = arith.cmpi slt, %kw0, %khi0 : index
        scf.condition(%kcmp0) %kw0 : index
      } do {
      ^bb0(%k0: index):
        %z0 = memref.load %c[%i0, %j0] : memref<16x8xf32>
        %x0 = memref.load %a[%i0, %k0] : memref<16x12xf32>
        %y0 = memref.load %b[%k0, %j0] : memref<12x8xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%i0, %j0] : memref<16x8xf32>
        %knx0 = arith.addi %k0, %kst0 : index
        scf.yield %knx0 : index
      }
      %jnx0 = arith.addi %j0, %jst0 : index
      scf.yield %jnx0 : index
    }
    %inx0 = arith.addi %i0, %ist0 : index
    scf.yield %inx0 : index
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
  %ires1 = scf.while (%iw1 = %ilo1) : (index) -> index {
    %icmp1 = arith.cmpi slt, %iw1, %ihi1 : index
    scf.condition(%icmp1) %iw1 : index
  } do {
  ^bb0(%i1: index):
    %jres1 = scf.while (%jw1 = %jlo1) : (index) -> index {
      %jcmp1 = arith.cmpi slt, %jw1, %jhi1 : index
      scf.condition(%jcmp1) %jw1 : index
    } do {
    ^bb0(%j1: index):
      %kres1 = scf.while (%kw1 = %klo1) : (index) -> index {
        %kcmp1 = arith.cmpi slt, %kw1, %khi1 : index
        scf.condition(%kcmp1) %kw1 : index
      } do {
      ^bb0(%k1: index):
        %z1 = memref.load %c[%i1, %j1] : memref<16x8xf32>
        %x1 = memref.load %a[%i1, %k1] : memref<16x12xf32>
        %y1 = memref.load %d[%k1, %j1] : memref<12x8xf32>
        %m1 = arith.mulf %x1, %y1 : f32
        %s1 = arith.addf %z1, %m1 fastmath<reassoc> : f32
        memref.store %s1, %c[%i1, %j1] : memref<16x8xf32>
        %knx1 = arith.addi %k1, %kst1 : index
        scf.yield %knx1 : index
      }
      %jnx1 = arith.addi %j1, %jst1 : index
      scf.yield %jnx1 : index
    }
    %inx1 = arith.addi %i1, %ist1 : index
    scf.yield %inx1 : index
  }
  return
}
func.func @whl_chain(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %t: memref<16x8xf32> {llvm.noalias}, %d: memref<8x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %ilo0 = arith.constant 0 : index
  %ihi0 = arith.constant 16 : index
  %ist0 = arith.constant 1 : index
  %jlo0 = arith.constant 0 : index
  %jhi0 = arith.constant 8 : index
  %jst0 = arith.constant 1 : index
  %klo0 = arith.constant 0 : index
  %khi0 = arith.constant 12 : index
  %kst0 = arith.constant 1 : index
  %ires0 = scf.while (%iw0 = %ilo0) : (index) -> index {
    %icmp0 = arith.cmpi slt, %iw0, %ihi0 : index
    scf.condition(%icmp0) %iw0 : index
  } do {
  ^bb0(%i0: index):
    %jres0 = scf.while (%jw0 = %jlo0) : (index) -> index {
      %jcmp0 = arith.cmpi slt, %jw0, %jhi0 : index
      scf.condition(%jcmp0) %jw0 : index
    } do {
    ^bb0(%j0: index):
      %kres0 = scf.while (%kw0 = %klo0) : (index) -> index {
        %kcmp0 = arith.cmpi slt, %kw0, %khi0 : index
        scf.condition(%kcmp0) %kw0 : index
      } do {
      ^bb0(%k0: index):
        %z0 = memref.load %t[%i0, %j0] : memref<16x8xf32>
        %x0 = memref.load %a[%i0, %k0] : memref<16x12xf32>
        %y0 = memref.load %b[%k0, %j0] : memref<12x8xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %t[%i0, %j0] : memref<16x8xf32>
        %knx0 = arith.addi %k0, %kst0 : index
        scf.yield %knx0 : index
      }
      %jnx0 = arith.addi %j0, %jst0 : index
      scf.yield %jnx0 : index
    }
    %inx0 = arith.addi %i0, %ist0 : index
    scf.yield %inx0 : index
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
  %ires1 = scf.while (%iw1 = %ilo1) : (index) -> index {
    %icmp1 = arith.cmpi slt, %iw1, %ihi1 : index
    scf.condition(%icmp1) %iw1 : index
  } do {
  ^bb0(%i1: index):
    %jres1 = scf.while (%jw1 = %jlo1) : (index) -> index {
      %jcmp1 = arith.cmpi slt, %jw1, %jhi1 : index
      scf.condition(%jcmp1) %jw1 : index
    } do {
    ^bb0(%j1: index):
      scf.for %k1 = %klo1 to %khi1 step %kst1 {
        %z1 = memref.load %c[%i1, %j1] : memref<16x8xf32>
        %x1 = memref.load %t[%i1, %k1] : memref<16x8xf32>
        %y1 = memref.load %d[%k1, %j1] : memref<8x8xf32>
        %m1 = arith.mulf %x1, %y1 : f32
        %s1 = arith.addf %z1, %m1 fastmath<reassoc> : f32
        memref.store %s1, %c[%i1, %j1] : memref<16x8xf32>
      }
      %jnx1 = arith.addi %j1, %jst1 : index
      scf.yield %jnx1 : index
    }
    %inx1 = arith.addi %i1, %ist1 : index
    scf.yield %inx1 : index
  }
  return
}
func.func @whl_strict(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %ilo0 = arith.constant 0 : index
  %ihi0 = arith.constant 16 : index
  %ist0 = arith.constant 1 : index
  %jlo0 = arith.constant 0 : index
  %jhi0 = arith.constant 8 : index
  %jst0 = arith.constant 1 : index
  %klo0 = arith.constant 0 : index
  %khi0 = arith.constant 12 : index
  %kst0 = arith.constant 1 : index
  %ires0 = scf.while (%iw0 = %ilo0) : (index) -> index {
    %icmp0 = arith.cmpi slt, %iw0, %ihi0 : index
    scf.condition(%icmp0) %iw0 : index
  } do {
  ^bb0(%i0: index):
    %jres0 = scf.while (%jw0 = %jlo0) : (index) -> index {
      %jcmp0 = arith.cmpi slt, %jw0, %jhi0 : index
      scf.condition(%jcmp0) %jw0 : index
    } do {
    ^bb0(%j0: index):
      %kres0 = scf.while (%kw0 = %klo0) : (index) -> index {
        %kcmp0 = arith.cmpi slt, %kw0, %khi0 : index
        scf.condition(%kcmp0) %kw0 : index
      } do {
      ^bb0(%k0: index):
        %z0 = memref.load %c[%i0, %j0] : memref<16x8xf32>
        %x0 = memref.load %a[%i0, %k0] : memref<16x12xf32>
        %y0 = memref.load %b[%k0, %j0] : memref<12x8xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 : f32
        memref.store %s0, %c[%i0, %j0] : memref<16x8xf32>
        %knx0 = arith.addi %k0, %kst0 : index
        scf.yield %knx0 : index
      }
      %jnx0 = arith.addi %j0, %jst0 : index
      scf.yield %jnx0 : index
    }
    %inx0 = arith.addi %i0, %ist0 : index
    scf.yield %inx0 : index
  }
  return
}
