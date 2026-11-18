// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_carried.json record-driven=true report=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=REMARK
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_carried.json record-driven=true" | FileCheck %s --check-prefix=TILES
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_carried.json record-driven=true" --lower-systolic-matmul-tile-to-fpga=async=true | FileCheck %s --check-prefix=ASYNC
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_carried_searchless.json record-driven=true report=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=SEARCHLESS
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_carried.json report=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=STAGE2 --allow-empty
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="report=true" | FileCheck %s --check-prefix=OFFIR --implicit-check-not=systolic.matmul_tile
// GEMMs whose subscripts their loops carry, lowered from their records (stage 3 of
// docs/GENERALIZED_GEMM_SCOPE.md; docs/GENERALIZED_GEMM_STATEMENT_BV.md, Section 14.9). A
// subscript here is no arithmetic of a counter: an scf.for (iter_args) or a counting scf.while
// carries it, an index started at an arith.constant on every entry of the loop, handed on as
// itself plus or minus a constant, and read by nobody after the loop. The research proposes no
// closed form of such a view by itself; with index invariants
// (generalized-gemm-nest-report --index-invariants) it finds subscript == start + step * counter,
// proves it by induction and proves the views with it, and the record is VERIFIED. The lowering
// does not take the recurrence on the record's word (Systolic/NestRecordLowering.h): it reads
// start and step from the loop in front of it, as start + step * (counter - lower), and the
// record's view must be that form, dimension by dimension.
// Inputs/nest_records_carried.json is what generalized-gemm-nest-report --index-invariants says
// of this file (every pair of roots authorized, 64-bit index);
// Inputs/nest_records_carried_searchless.json what it says without --index-invariants. Both are
// written and checked by research/generalized-gemm/test/fuzz/nest_gate_records.py. The @car_
// functions are nest_carried_e2e.py's, which runs them and compares every result with the
// original program's; the five after them are read here only. nest_records_carried_refusals.mlir
// has the refusals.
// Nest by nest in program order. One nest stays as written: @car_strict, by the floating-point
// policy. Nothing is lowered by the production recognizer here.
//
// A level of the record that is a loop's counter plus a constant. Of @car_plain, the control
// that carries nothing (its subscript is k + 2 by arith), of @offset_while_second and of
// @step_then_use the research takes the subscript itself for the K level: from 2 to 14, where
// the loop runs from 0 to 12, and views that read the level as it is. The lowering takes the
// level for the counter plus the difference of the lower bounds, 2, rewrites the record's
// forms to the counter, and they must then be the forms it read from the nest.
// REMARK-NOT: affine GEMM
// REMARK: remark: GEMM 16x8x12 of nest 0 of car_iter lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 0 of car_while lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 0 of car_plain lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 0 of car_mixed lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 0 of car_outer lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 0 of car_step2 lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 0 of car_shifted lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM not lowered from its record: nest 0 of car_strict: floating-point reassociation is not permitted
// REMARK: remark: GEMM 16x8x12 of nest 0 of offset_iter lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 0 of offset_while lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 0 of down lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 0 of offset_while_second lowered from its record as 5 exact region(s) (formulas:
// REMARK: remark: GEMM 16x8x12 of nest 0 of step_then_use lowered from its record as 5 exact region(s) (formulas:
// REMARK-NOT: remark:
//
// Each lowered nest is gone, its tiles where it stood: no loop is left to carry the index.
// TILES-LABEL: func.func @car_iter(
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: {{scf\.while|scf\.for|affine\.for|iter_args}}
// TILES: return
// TILES-LABEL: func.func @car_while(
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: {{scf\.while|scf\.for|affine\.for|iter_args}}
// TILES: return
// TILES-LABEL: func.func @car_plain(
// TILES: memref.subview %arg0[0, 2] [16, 12] [1, 1]
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: {{scf\.while|scf\.for|affine\.for|iter_args}}
// TILES: return
// TILES-LABEL: func.func @car_mixed(
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: {{scf\.while|scf\.for|affine\.for|iter_args}}
// TILES: return
// TILES-LABEL: func.func @car_outer(
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: {{scf\.while|scf\.for|affine\.for|iter_args}}
// TILES: return
// TILES-LABEL: func.func @car_step2(
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: {{scf\.while|scf\.for|affine\.for|iter_args}}
// TILES: return
// TILES-LABEL: func.func @car_shifted(
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: {{scf\.while|scf\.for|affine\.for|iter_args}}
// TILES: return
// TILES-LABEL: func.func @car_strict(
// TILES-NOT: systolic.matmul_tile
// TILES: scf.for
// TILES: return
// TILES-LABEL: func.func @offset_iter(
// TILES: memref.subview %arg0[0, 2] [16, 12] [1, 1]
// TILES: memref.subview %arg1[2, 0] [12, 8] [1, 1]
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: {{scf\.while|scf\.for|affine\.for|iter_args}}
// TILES: return
// TILES-LABEL: func.func @offset_while(
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: {{scf\.while|scf\.for|affine\.for|iter_args}}
// TILES: return
// TILES-LABEL: func.func @down(
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: {{scf\.while|scf\.for|affine\.for|iter_args}}
// TILES: return
// TILES-LABEL: func.func @offset_while_second(
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: {{scf\.while|scf\.for|affine\.for|iter_args}}
// TILES: return
// TILES-LABEL: func.func @step_then_use(
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: {{scf\.while|scf\.for|affine\.for|iter_args}}
// TILES: return
//
// (The operands of @offset_iter, checked above, are subviews that start at column 2 of A and
// row 2 of B: the recurrence's start, read from the loop, and the tiles read 12 columns and 12
// rows from there.)
//
// Down to the runtime, each lowered nest is one asynchronous GEMM.
// ASYNC-LABEL: func.func @car_iter(
// ASYNC: call @fpga_gemm_begin
// ASYNC: call @fpga_gemm_wait
// ASYNC-LABEL: func.func @car_while(
// ASYNC: call @fpga_gemm_begin
// ASYNC: call @fpga_gemm_wait
// ASYNC-LABEL: func.func @car_plain(
// ASYNC: call @fpga_gemm_begin
// ASYNC: call @fpga_gemm_wait
// ASYNC-LABEL: func.func @car_mixed(
// ASYNC: call @fpga_gemm_begin
// ASYNC: call @fpga_gemm_wait
// ASYNC-LABEL: func.func @car_outer(
// ASYNC: call @fpga_gemm_begin
// ASYNC: call @fpga_gemm_wait
// ASYNC-LABEL: func.func @car_step2(
// ASYNC: call @fpga_gemm_begin
// ASYNC: call @fpga_gemm_wait
// ASYNC-LABEL: func.func @car_shifted(
// ASYNC: call @fpga_gemm_begin
// ASYNC: call @fpga_gemm_wait
// ASYNC-LABEL: func.func @car_strict(
// ASYNC-LABEL: func.func @offset_iter(
// ASYNC: call @fpga_gemm_begin
// ASYNC: call @fpga_gemm_wait
// ASYNC-LABEL: func.func @offset_while(
// ASYNC: call @fpga_gemm_begin
// ASYNC: call @fpga_gemm_wait
// ASYNC-LABEL: func.func @down(
// ASYNC: call @fpga_gemm_begin
// ASYNC: call @fpga_gemm_wait
//
// Records made without index invariants: no carrying nest is verified, and none is lowered.
// @car_plain's record is the same either way, and so is its lowering.
// SEARCHLESS: remark: GEMM not lowered from its record: nest 0 of car_iter: the record's status is NOT_VERIFIED: no candidate for the nest: the search is UNKNOWN (no candidate was verified within the limits)
// SEARCHLESS: remark: GEMM not lowered from its record: nest 0 of car_while: the record's status is NOT_VERIFIED: no candidate for the nest: the search is UNKNOWN (no candidate was verified within the limits)
// SEARCHLESS: remark: GEMM 16x8x12 of nest 0 of car_plain lowered from its record as 5 exact region(s) (formulas:
// SEARCHLESS: remark: GEMM not lowered from its record: nest 0 of car_mixed: the record's status is NOT_VERIFIED: no candidate for the nest: the search is UNKNOWN (no candidate was verified within the limits)
// SEARCHLESS: remark: GEMM not lowered from its record: nest 0 of car_outer: the record's status is NOT_VERIFIED: no candidate for the nest: the search is UNKNOWN (no candidate was verified within the limits)
// SEARCHLESS: remark: GEMM not lowered from its record: nest 0 of car_step2: the record's status is NOT_VERIFIED: no candidate for the nest: the search is UNKNOWN (no candidate was verified within the limits)
// SEARCHLESS: remark: GEMM not lowered from its record: nest 0 of car_shifted: the record's status is NOT_VERIFIED: no candidate for the nest: the search is UNKNOWN (no candidate was verified within the limits)
// SEARCHLESS: remark: GEMM not lowered from its record: nest 0 of car_strict: the record's status is NOT_VERIFIED: no candidate for the nest: the search is UNKNOWN (no candidate was verified within the limits)
// SEARCHLESS: remark: GEMM not lowered from its record: nest 0 of offset_iter: the record's status is NOT_VERIFIED: no candidate for the nest: the search is UNKNOWN (no candidate was verified within the limits)
// SEARCHLESS: remark: GEMM not lowered from its record: nest 0 of offset_while: the record's status is NOT_VERIFIED: no candidate for the nest: the search is UNKNOWN (no candidate was verified within the limits)
// SEARCHLESS: remark: GEMM not lowered from its record: nest 0 of down: the record's status is NOT_VERIFIED: no candidate for the nest: the search is UNKNOWN (no candidate was verified within the limits)
// SEARCHLESS: remark: GEMM not lowered from its record: nest 0 of offset_while_second: the record's status is NOT_VERIFIED: no candidate for the nest: the search is UNKNOWN (no candidate was verified within the limits)
// SEARCHLESS: remark: GEMM not lowered from its record: nest 0 of step_then_use: the record's status is NOT_VERIFIED: the ordered fold is verified at POINT only, and not over the scope
// SEARCHLESS-NOT: remark:
//
// The record must say what the loop does. A record of @offset_iter whose views start one
// further on (K + 3), and one of @offset_while whose view of A steps by 2, are refused with the
// form the lowering read from the nest.
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_carried.json k-offset@offset_iter > %t.offset.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.offset.json record-driven=true" 2>&1 | FileCheck %s --check-prefix=OFFSET
// OFFSET: remark: GEMM not lowered from its record: nest 0 of offset_iter: dimension 1 of the record's view of A is not the nest's index: 1 * loop 2 + 2
// OFFSET-LABEL: func.func @offset_iter(
// OFFSET-NOT: systolic.matmul_tile
// OFFSET: iter_args
// OFFSET-LABEL: func.func @offset_while(
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_carried.json k-step@offset_while > %t.step.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.step.json record-driven=true" 2>&1 | FileCheck %s --check-prefix=STEP
// STEP: remark: GEMM not lowered from its record: nest 0 of offset_while: dimension 1 of the record's view of A is not the nest's index: 1 * loop 2 + 2
// STEP-LABEL: func.func @offset_while(
// STEP-NOT: systolic.matmul_tile
// STEP: scf.while
// STEP-LABEL: func.func @down(
//
// A level that is the counter plus a constant is checked through the views, and by nothing
// else. The record of @car_plain with its K level one further on (from 3 to 15) and its views
// as they were is a record of other cells of A and B: refused with the form read from the
// nest. The record of @offset_iter with its K level named 3 further on and its views of A and B
// 3 less says the same nest otherwise: lowered, to the same tiles.
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_carried.json k-level-shift@car_plain > %t.levelshift.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.levelshift.json record-driven=true" 2>&1 | FileCheck %s --check-prefix=LEVELSHIFT
// LEVELSHIFT: remark: GEMM not lowered from its record: nest 0 of car_plain: dimension 1 of the record's view of A is not the nest's index: 1 * loop 2 + 2
// LEVELSHIFT-LABEL: func.func @car_plain(
// LEVELSHIFT-NOT: systolic.matmul_tile
// LEVELSHIFT: scf.for
// LEVELSHIFT-LABEL: func.func @car_mixed(
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_carried.json k-level-rename@offset_iter > %t.rename.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.rename.json record-driven=true report=true" 2>&1 | FileCheck %s --check-prefix=RENAME
// RENAME: remark: GEMM 16x8x12 of nest 0 of offset_iter lowered from its record as 5 exact region(s) (formulas:
// RENAME-LABEL: func.func @offset_iter(
// RENAME: memref.subview %arg0[0, 2] [16, 12] [1, 1]
// RENAME: memref.subview %arg1[2, 0] [12, 8] [1, 1]
// RENAME-COUNT-5: systolic.matmul_tile
// RENAME-LABEL: func.func @offset_while(
//
// The program changed after its records were made: the recurrences of @offset_iter,
// @offset_while and @offset_while_second start at 3. The start is a constant the binding reads, so the records are of
// another version of those nests; the other nests are lowered as before.
// RUN: sed -e 's/%c2 = arith.constant 2 : index/%c2 = arith.constant 3 : index/' %s > %t.stale.mlir
// RUN: systolic-opt %t.stale.mlir --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_carried.json record-driven=true" 2>&1 | FileCheck %s --check-prefix=STALE
// STALE: remark: GEMM not lowered from its record: nest 0 of offset_iter: the record's binding is not this nest's: the record is about another version of the nest, or another nest
// STALE: remark: GEMM not lowered from its record: nest 0 of offset_while: the record's binding is not this nest's: the record is about another version of the nest, or another nest
// STALE: remark: GEMM not lowered from its record: nest 0 of offset_while_second: the record's binding is not this nest's: the record is about another version of the nest, or another nest
// STALE-LABEL: func.func @car_iter(
// STALE-COUNT-5: systolic.matmul_tile
// STALE-LABEL: func.func @offset_iter(
// STALE-NOT: systolic.matmul_tile
// STALE: iter_args(%{{.*}} = %c3)
// STALE-LABEL: func.func @offset_while(
// STALE-NOT: systolic.matmul_tile
// STALE: scf.while
// STALE-LABEL: func.func @down(
// STALE-COUNT-5: systolic.matmul_tile
//
// Stage 2 alone says nothing of these nests (the production recognizer finds no GEMM in them),
// and without records the module is as written.
// STAGE2-NOT: remark:
// OFFIR-LABEL: func.func @car_iter(
// OFFIR: iter_args
// OFFIR-LABEL: func.func @offset_while(
// OFFIR: scf.while

func.func @car_iter(%a: memref<16x15xf32>, %b: memref<15x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %ilo0 = arith.constant 0 : index
  %ihi0 = arith.constant 16 : index
  %ist0 = arith.constant 1 : index
  %jlo0 = arith.constant 0 : index
  %jhi0 = arith.constant 8 : index
  %jst0 = arith.constant 1 : index
  %klo0 = arith.constant 0 : index
  %khi0 = arith.constant 12 : index
  %kst0 = arith.constant 1 : index
  %acx1n0start = arith.constant 2 : index
  %acx1n0step = arith.constant 1 : index
  %bcx0n0start = arith.constant 2 : index
  %bcx0n0step = arith.constant 1 : index
  scf.for %i0 = %ilo0 to %ihi0 step %ist0 {
    scf.for %j0 = %jlo0 to %jhi0 step %jst0 {
      %kres0:2 = scf.for %k0 = %klo0 to %khi0 step %kst0 iter_args(%acx1n0 = %acx1n0start, %bcx0n0 = %bcx0n0start) -> (index, index) {
        %z0 = memref.load %c[%i0, %j0] : memref<16x8xf32>
        %x0 = memref.load %a[%i0, %acx1n0] : memref<16x15xf32>
        %y0 = memref.load %b[%bcx0n0, %j0] : memref<15x8xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%i0, %j0] : memref<16x8xf32>
        %acx1n0next = arith.addi %acx1n0, %acx1n0step : index
        %bcx0n0next = arith.addi %bcx0n0, %bcx0n0step : index
        scf.yield %acx1n0next, %bcx0n0next : index, index
      }
    }
  }
  return
}
func.func @car_while(%a: memref<16x15xf32>, %b: memref<15x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %ilo0 = arith.constant 0 : index
  %ihi0 = arith.constant 16 : index
  %ist0 = arith.constant 1 : index
  %jlo0 = arith.constant 0 : index
  %jhi0 = arith.constant 8 : index
  %jst0 = arith.constant 1 : index
  %klo0 = arith.constant 0 : index
  %khi0 = arith.constant 12 : index
  %kst0 = arith.constant 1 : index
  %acx1n0start = arith.constant 2 : index
  %acx1n0step = arith.constant 1 : index
  %bcx0n0start = arith.constant 2 : index
  %bcx0n0step = arith.constant 1 : index
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
      %kres0:3 = scf.while (%kw0 = %klo0, %acx1n0w = %acx1n0start, %bcx0n0w = %bcx0n0start) : (index, index, index) -> (index, index, index) {
        %kcmp0 = arith.cmpi slt, %kw0, %khi0 : index
        scf.condition(%kcmp0) %kw0, %acx1n0w, %bcx0n0w : index, index, index
      } do {
      ^bb0(%k0: index, %acx1n0: index, %bcx0n0: index):
        %z0 = memref.load %c[%i0, %j0] : memref<16x8xf32>
        %x0 = memref.load %a[%i0, %acx1n0] : memref<16x15xf32>
        %y0 = memref.load %b[%bcx0n0, %j0] : memref<15x8xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%i0, %j0] : memref<16x8xf32>
        %knx0 = arith.addi %k0, %kst0 : index
        %acx1n0next = arith.addi %acx1n0, %acx1n0step : index
        %bcx0n0next = arith.addi %bcx0n0, %bcx0n0step : index
        scf.yield %knx0, %acx1n0next, %bcx0n0next : index, index, index
      }
      %jnx0 = arith.addi %j0, %jst0 : index
      scf.yield %jnx0 : index
    }
    %inx0 = arith.addi %i0, %ist0 : index
    scf.yield %inx0 : index
  }
  return
}
func.func @car_plain(%a: memref<16x15xf32>, %b: memref<15x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
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
        %ao1c0 = arith.constant 2 : index
        %bo0c0 = arith.constant 2 : index
        %ao10 = arith.addi %k0, %ao1c0 : index
        %bo00 = arith.addi %k0, %bo0c0 : index
        %z0 = memref.load %c[%i0, %j0] : memref<16x8xf32>
        %x0 = memref.load %a[%i0, %ao10] : memref<16x15xf32>
        %y0 = memref.load %b[%bo00, %j0] : memref<15x8xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%i0, %j0] : memref<16x8xf32>
      }
    }
  }
  return
}
func.func @car_mixed(%a: memref<16x15xf32>, %b: memref<15x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %jlo0 = arith.constant 0 : index
  %jhi0 = arith.constant 8 : index
  %jst0 = arith.constant 1 : index
  %klo0 = arith.constant 0 : index
  %khi0 = arith.constant 12 : index
  %kst0 = arith.constant 1 : index
  %acx1n0start = arith.constant 2 : index
  %acx1n0step = arith.constant 1 : index
  %bcx0n0start = arith.constant 2 : index
  %bcx0n0step = arith.constant 1 : index
  affine.for %i0 = 0 to 16 {
    %jres0 = scf.while (%jw0 = %jlo0) : (index) -> index {
      %jcmp0 = arith.cmpi slt, %jw0, %jhi0 : index
      scf.condition(%jcmp0) %jw0 : index
    } do {
    ^bb0(%j0: index):
      %kres0:2 = scf.for %k0 = %klo0 to %khi0 step %kst0 iter_args(%acx1n0 = %acx1n0start, %bcx0n0 = %bcx0n0start) -> (index, index) {
        %z0 = memref.load %c[%i0, %j0] : memref<16x8xf32>
        %x0 = memref.load %a[%i0, %acx1n0] : memref<16x15xf32>
        %y0 = memref.load %b[%bcx0n0, %j0] : memref<15x8xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%i0, %j0] : memref<16x8xf32>
        %acx1n0next = arith.addi %acx1n0, %acx1n0step : index
        %bcx0n0next = arith.addi %bcx0n0, %bcx0n0step : index
        scf.yield %acx1n0next, %bcx0n0next : index, index
      }
      %jnx0 = arith.addi %j0, %jst0 : index
      scf.yield %jnx0 : index
    }
  }
  return
}
func.func @car_outer(%a: memref<17x12xf32>, %b: memref<12x8xf32>, %c: memref<17x8xf32> {llvm.noalias}) {
  %ilo0 = arith.constant 0 : index
  %ihi0 = arith.constant 16 : index
  %ist0 = arith.constant 1 : index
  %ccx0n0start = arith.constant 1 : index
  %ccx0n0step = arith.constant 1 : index
  %acx0n0start = arith.constant 1 : index
  %acx0n0step = arith.constant 1 : index
  %jlo0 = arith.constant 0 : index
  %jhi0 = arith.constant 8 : index
  %jst0 = arith.constant 1 : index
  %klo0 = arith.constant 0 : index
  %khi0 = arith.constant 12 : index
  %kst0 = arith.constant 1 : index
  %ires0:3 = scf.while (%iw0 = %ilo0, %ccx0n0w = %ccx0n0start, %acx0n0w = %acx0n0start) : (index, index, index) -> (index, index, index) {
    %icmp0 = arith.cmpi slt, %iw0, %ihi0 : index
    scf.condition(%icmp0) %iw0, %ccx0n0w, %acx0n0w : index, index, index
  } do {
  ^bb0(%i0: index, %ccx0n0: index, %acx0n0: index):
    scf.for %j0 = %jlo0 to %jhi0 step %jst0 {
      scf.for %k0 = %klo0 to %khi0 step %kst0 {
        %z0 = memref.load %c[%ccx0n0, %j0] : memref<17x8xf32>
        %x0 = memref.load %a[%acx0n0, %k0] : memref<17x12xf32>
        %y0 = memref.load %b[%k0, %j0] : memref<12x8xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%ccx0n0, %j0] : memref<17x8xf32>
      }
    }
    %inx0 = arith.addi %i0, %ist0 : index
    %ccx0n0next = arith.addi %ccx0n0, %ccx0n0step : index
    %acx0n0next = arith.addi %acx0n0, %acx0n0step : index
    scf.yield %inx0, %ccx0n0next, %acx0n0next : index, index, index
  }
  return
}
func.func @car_step2(%a: memref<16x12xf32>, %b: memref<24x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %ilo0 = arith.constant 0 : index
  %ihi0 = arith.constant 16 : index
  %ist0 = arith.constant 1 : index
  %jlo0 = arith.constant 0 : index
  %jhi0 = arith.constant 8 : index
  %jst0 = arith.constant 1 : index
  %klo0 = arith.constant 0 : index
  %khi0 = arith.constant 12 : index
  %kst0 = arith.constant 1 : index
  %bcx0n0start = arith.constant 1 : index
  %bcx0n0step = arith.constant 2 : index
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
      %kres0:2 = scf.while (%kw0 = %klo0, %bcx0n0w = %bcx0n0start) : (index, index) -> (index, index) {
        %kcmp0 = arith.cmpi slt, %kw0, %khi0 : index
        scf.condition(%kcmp0) %kw0, %bcx0n0w : index, index
      } do {
      ^bb0(%k0: index, %bcx0n0: index):
        %z0 = memref.load %c[%i0, %j0] : memref<16x8xf32>
        %x0 = memref.load %a[%i0, %k0] : memref<16x12xf32>
        %y0 = memref.load %b[%bcx0n0, %j0] : memref<24x8xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%i0, %j0] : memref<16x8xf32>
        %knx0 = arith.addi %k0, %kst0 : index
        %bcx0n0next = arith.addi %bcx0n0, %bcx0n0step : index
        scf.yield %knx0, %bcx0n0next : index, index
      }
      %jnx0 = arith.addi %j0, %jst0 : index
      scf.yield %jnx0 : index
    }
    %inx0 = arith.addi %i0, %ist0 : index
    scf.yield %inx0 : index
  }
  return
}
func.func @car_shifted(%a: memref<18x17xf32>, %b: memref<17x9xf32>, %c: memref<18x9xf32> {llvm.noalias}) {
  %ilo0 = arith.constant 2 : index
  %ihi0 = arith.constant 18 : index
  %ist0 = arith.constant 1 : index
  %jlo0 = arith.constant 1 : index
  %jhi0 = arith.constant 9 : index
  %jst0 = arith.constant 1 : index
  %klo0 = arith.constant 3 : index
  %khi0 = arith.constant 15 : index
  %kst0 = arith.constant 1 : index
  %acx1n0start = arith.constant 5 : index
  %acx1n0step = arith.constant 1 : index
  %bcx0n0start = arith.constant 5 : index
  %bcx0n0step = arith.constant 1 : index
  scf.for %i0 = %ilo0 to %ihi0 step %ist0 {
    %jres0 = scf.while (%jw0 = %jlo0) : (index) -> index {
      %jcmp0 = arith.cmpi slt, %jw0, %jhi0 : index
      scf.condition(%jcmp0) %jw0 : index
    } do {
    ^bb0(%j0: index):
      %kres0:2 = scf.for %k0 = %klo0 to %khi0 step %kst0 iter_args(%acx1n0 = %acx1n0start, %bcx0n0 = %bcx0n0start) -> (index, index) {
        %z0 = memref.load %c[%i0, %j0] : memref<18x9xf32>
        %x0 = memref.load %a[%i0, %acx1n0] : memref<18x17xf32>
        %y0 = memref.load %b[%bcx0n0, %j0] : memref<17x9xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%i0, %j0] : memref<18x9xf32>
        %acx1n0next = arith.addi %acx1n0, %acx1n0step : index
        %bcx0n0next = arith.addi %bcx0n0, %bcx0n0step : index
        scf.yield %acx1n0next, %bcx0n0next : index, index
      }
      %jnx0 = arith.addi %j0, %jst0 : index
      scf.yield %jnx0 : index
    }
  }
  return
}
func.func @car_strict(%a: memref<16x15xf32>, %b: memref<15x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %ilo0 = arith.constant 0 : index
  %ihi0 = arith.constant 16 : index
  %ist0 = arith.constant 1 : index
  %jlo0 = arith.constant 0 : index
  %jhi0 = arith.constant 8 : index
  %jst0 = arith.constant 1 : index
  %klo0 = arith.constant 0 : index
  %khi0 = arith.constant 12 : index
  %kst0 = arith.constant 1 : index
  %acx1n0start = arith.constant 2 : index
  %acx1n0step = arith.constant 1 : index
  %bcx0n0start = arith.constant 2 : index
  %bcx0n0step = arith.constant 1 : index
  scf.for %i0 = %ilo0 to %ihi0 step %ist0 {
    scf.for %j0 = %jlo0 to %jhi0 step %jst0 {
      %kres0:2 = scf.for %k0 = %klo0 to %khi0 step %kst0 iter_args(%acx1n0 = %acx1n0start, %bcx0n0 = %bcx0n0start) -> (index, index) {
        %z0 = memref.load %c[%i0, %j0] : memref<16x8xf32>
        %x0 = memref.load %a[%i0, %acx1n0] : memref<16x15xf32>
        %y0 = memref.load %b[%bcx0n0, %j0] : memref<15x8xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 : f32
        memref.store %s0, %c[%i0, %j0] : memref<16x8xf32>
        %acx1n0next = arith.addi %acx1n0, %acx1n0step : index
        %bcx0n0next = arith.addi %bcx0n0, %bcx0n0step : index
        scf.yield %acx1n0next, %bcx0n0next : index, index
      }
    }
  }
  return
}
// One carried index read by A and by B: x starts at 2 and steps by 1 with K, in an scf.for.
func.func @offset_iter(%a: memref<16x15xf32>, %b: memref<15x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  scf.for %i = %c0 to %c16 step %c1 {
    scf.for %j = %c0 to %c8 step %c1 {
      %xf = scf.for %k = %c0 to %c12 step %c1 iter_args(%x = %c2) -> (index) {
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %p = memref.load %a[%i, %x] : memref<16x15xf32>
        %q = memref.load %b[%x, %j] : memref<15x8xf32>
        %m = arith.mulf %p, %q : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
        %x1 = arith.addi %x, %c1 : index
        scf.yield %x1 : index
      }
    }
  }
  return
}
// The same, x the second value an scf.while carries beside its counter.
func.func @offset_while(%a: memref<16x15xf32>, %b: memref<15x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  scf.for %i = %c0 to %c16 step %c1 {
    scf.for %j = %c0 to %c8 step %c1 {
      %kf, %xf = scf.while (%k = %c0, %x = %c2) : (index, index) -> (index, index) {
        %go = arith.cmpi slt, %k, %c12 : index
        scf.condition(%go) %k, %x : index, index
      } do {
      ^bb0(%kb: index, %xb: index):
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %p = memref.load %a[%i, %xb] : memref<16x15xf32>
        %q = memref.load %b[%xb, %j] : memref<15x8xf32>
        %m = arith.mulf %p, %q : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
        %k1 = arith.addi %kb, %c1 : index
        %x1 = arith.addi %xb, %c1 : index
        scf.yield %k1, %x1 : index, index
      }
    }
  }
  return
}
// The index counts down by arith.subi from 13: the subscript is 13 - K.
func.func @down(%a: memref<16x15xf32>, %b: memref<15x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c13 = arith.constant 13 : index
  %c16 = arith.constant 16 : index
  scf.for %i = %c0 to %c16 step %c1 {
    scf.for %j = %c0 to %c8 step %c1 {
      %xf = scf.for %k = %c0 to %c12 step %c1 iter_args(%x = %c13) -> (index) {
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %p = memref.load %a[%i, %x] : memref<16x15xf32>
        %q = memref.load %b[%x, %j] : memref<15x8xf32>
        %m = arith.mulf %p, %q : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
        %x1 = arith.subi %x, %c1 : index
        scf.yield %x1 : index
      }
    }
  }
  return
}
// The counter is the second carried value and the index the first: the counter is the one
// the comparison reads, whatever its place.
func.func @offset_while_second(%a: memref<16x15xf32>, %b: memref<15x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  scf.for %i = %c0 to %c16 step %c1 {
    scf.for %j = %c0 to %c8 step %c1 {
      %xf, %kf = scf.while (%x = %c2, %k = %c0) : (index, index) -> (index, index) {
        %go = arith.cmpi slt, %k, %c12 : index
        scf.condition(%go) %x, %k : index, index
      } do {
      ^bb0(%xb: index, %kb: index):
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %p = memref.load %a[%i, %xb] : memref<16x15xf32>
        %q = memref.load %b[%xb, %j] : memref<15x8xf32>
        %m = arith.mulf %p, %q : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
        %x1 = arith.addi %xb, %c1 : index
        %k1 = arith.addi %kb, %c1 : index
        scf.yield %x1, %k1 : index, index
      }
    }
  }
  return
}
// The index is stepped before it is read: the subscript is x + 1, from 1, so K + 2.
func.func @step_then_use(%a: memref<16x15xf32>, %b: memref<15x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  scf.for %i = %c0 to %c16 step %c1 {
    scf.for %j = %c0 to %c8 step %c1 {
      %xf = scf.for %k = %c0 to %c12 step %c1 iter_args(%x = %c1) -> (index) {
        %x1 = arith.addi %x, %c1 : index
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %p = memref.load %a[%i, %x1] : memref<16x15xf32>
        %q = memref.load %b[%x1, %j] : memref<15x8xf32>
        %m = arith.mulf %p, %q : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
        scf.yield %x1 : index
      }
    }
  }
  return
}
