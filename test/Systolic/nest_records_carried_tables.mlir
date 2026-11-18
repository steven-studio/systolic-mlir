// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_carried_tables.json record-driven=true report=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=REMARK
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_carried_tables.json record-driven=true" | FileCheck %s --check-prefix=TILES
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_carried_tables_searchless.json record-driven=true report=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=SEARCHLESS
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_carried_tables.json report=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=STAGE2 --allow-empty
//
// Stage 3 by tables on subscripts a loop carries (docs/GENERALIZED_GEMM_STATEMENT_BV.md,
// Section 14.9). The arrays are flat and the index constants are before the nest: the research
// proposes no closed form for such views, and a record of them is VERIFIED_NOT_MAPPED at best.
// The lowering then takes the nest's own addresses at each iteration, and a flat address here is
// arith of an index the K loop carries. The value of that index at an iteration is the one of
// the recurrence the lowering read from the IR (start + step * (counter - lower bound)): no
// record says it.
//
// The functions are those of research/generalized-gemm/test/fuzz/nest_carried_e2e.py, which
// lowers, links and runs them and compares every result with the original. The records are the
// research tool's (research/generalized-gemm/test/fuzz/nest_gate_records.py):
// nest_records_carried_tables.json with --index-invariants, and
// nest_records_carried_tables_searchless.json without.
//
//   car_tables_iter   an scf.for carries the K subscript of A and of B, from 2 by 1
//   car_tables_twin   the same from 0 by 1: the carried subscript is the counter's twin
//   car_tables_while  an scf.while carries the row of B, from 1 by 2
//
// REMARK: remark: GEMM 16x8x12 of nest 0 of car_tables_iter lowered from its record as 5 exact region(s) (tables: the nest's own addresses at its 1536 iterations, A one per (I, K), B one per (K, J), C one per (I, J) and one to one; each (I, J, K) is one iteration)
// REMARK: remark: GEMM 16x8x12 of nest 0 of car_tables_twin lowered from its record as 5 exact region(s) (tables:
// REMARK: remark: GEMM 16x8x12 of nest 0 of car_tables_while lowered from its record as 5 exact region(s) (tables:
// REMARK-NOT: remark:
//
// The tables hold the addresses the recurrences give. B of car_tables_iter is read at the flat
// address (K + 2) * 8 + J, B of car_tables_while at (2 K + 1) * 8 + J, B of car_tables_twin at
// K * 8 + J: the first two rows of each table.
// TILES-DAG: memref.global "private" constant @__nest_table_nest_0_of_car_tables_iter_B : memref<12x8xi64> = dense<{{\[\[}}16, 17, 18, 19, 20, 21, 22, 23], [24, 25, 26, 27, 28, 29, 30, 31],
// TILES-DAG: memref.global "private" constant @__nest_table_nest_0_of_car_tables_twin_B : memref<12x8xi64> = dense<{{\[\[}}0, 1, 2, 3, 4, 5, 6, 7], [8, 9, 10, 11, 12, 13, 14, 15],
// TILES-DAG: memref.global "private" constant @__nest_table_nest_0_of_car_tables_while_B : memref<12x8xi64> = dense<{{\[\[}}8, 9, 10, 11, 12, 13, 14, 15], [24, 25, 26, 27, 28, 29, 30, 31],
// TILES-LABEL: func.func @car_tables_iter
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: scf.for
// TILES: return
// TILES-LABEL: func.func @car_tables_twin
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: scf.for
// TILES: return
// TILES-LABEL: func.func @car_tables_while
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: scf.while
// TILES: return
//
// Without index invariants the research verifies the twin alone (its record is
// VERIFIED_NOT_MAPPED all the same, and it is lowered by tables all the same); the two whose
// subscript is no twin of the counter are not verified, and stay.
// SEARCHLESS: remark: GEMM not lowered from its record: nest 0 of car_tables_iter: the record's status is NOT_VERIFIED: the ordered fold is not verified: Theorem 13 is UNKNOWN (not checked)
// SEARCHLESS: remark: GEMM 16x8x12 of nest 0 of car_tables_twin lowered from its record as 5 exact region(s) (tables:
// SEARCHLESS: remark: GEMM not lowered from its record: nest 0 of car_tables_while: the record's status is NOT_VERIFIED: the ordered fold is not verified: Theorem 13 is UNKNOWN (not checked)
// SEARCHLESS-NOT: remark:
//
// Without record-driven nothing is lowered and nothing is said.
// STAGE2-NOT: remark:

func.func @car_tables_iter(%a: memref<240xf32>, %b: memref<120xf32>, %c: memref<128xf32> {llvm.noalias}) {
  %cw0 = arith.constant 8 : index
  %aw0 = arith.constant 15 : index
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
  %acx1n0start = arith.constant 2 : index
  %acx1n0step = arith.constant 1 : index
  %bcx0n0start = arith.constant 2 : index
  %bcx0n0step = arith.constant 1 : index
  scf.for %i0 = %ilo0 to %ihi0 step %ist0 {
    scf.for %j0 = %jlo0 to %jhi0 step %jst0 {
      %kres0:2 = scf.for %k0 = %klo0 to %khi0 step %kst0 iter_args(%acx1n0 = %acx1n0start, %bcx0n0 = %bcx0n0start) -> (index, index) {
        %cr0 = arith.muli %i0, %cw0 : index
        %cx0 = arith.addi %cr0, %j0 : index
        %ar0 = arith.muli %i0, %aw0 : index
        %ax0 = arith.addi %ar0, %acx1n0 : index
        %br0 = arith.muli %bcx0n0, %bw0 : index
        %bx0 = arith.addi %br0, %j0 : index
        %z0 = memref.load %c[%cx0] : memref<128xf32>
        %x0 = memref.load %a[%ax0] : memref<240xf32>
        %y0 = memref.load %b[%bx0] : memref<120xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%cx0] : memref<128xf32>
        %acx1n0next = arith.addi %acx1n0, %acx1n0step : index
        %bcx0n0next = arith.addi %bcx0n0, %bcx0n0step : index
        scf.yield %acx1n0next, %bcx0n0next : index, index
      }
    }
  }
  return
}
func.func @car_tables_twin(%a: memref<192xf32>, %b: memref<96xf32>, %c: memref<128xf32> {llvm.noalias}) {
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
  %acx1n0start = arith.constant 0 : index
  %acx1n0step = arith.constant 1 : index
  %bcx0n0start = arith.constant 0 : index
  %bcx0n0step = arith.constant 1 : index
  scf.for %i0 = %ilo0 to %ihi0 step %ist0 {
    scf.for %j0 = %jlo0 to %jhi0 step %jst0 {
      %kres0:2 = scf.for %k0 = %klo0 to %khi0 step %kst0 iter_args(%acx1n0 = %acx1n0start, %bcx0n0 = %bcx0n0start) -> (index, index) {
        %cr0 = arith.muli %i0, %cw0 : index
        %cx0 = arith.addi %cr0, %j0 : index
        %ar0 = arith.muli %i0, %aw0 : index
        %ax0 = arith.addi %ar0, %acx1n0 : index
        %br0 = arith.muli %bcx0n0, %bw0 : index
        %bx0 = arith.addi %br0, %j0 : index
        %z0 = memref.load %c[%cx0] : memref<128xf32>
        %x0 = memref.load %a[%ax0] : memref<192xf32>
        %y0 = memref.load %b[%bx0] : memref<96xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%cx0] : memref<128xf32>
        %acx1n0next = arith.addi %acx1n0, %acx1n0step : index
        %bcx0n0next = arith.addi %bcx0n0, %bcx0n0step : index
        scf.yield %acx1n0next, %bcx0n0next : index, index
      }
    }
  }
  return
}
func.func @car_tables_while(%a: memref<192xf32>, %b: memref<192xf32>, %c: memref<128xf32> {llvm.noalias}) {
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
        %cr0 = arith.muli %i0, %cw0 : index
        %cx0 = arith.addi %cr0, %j0 : index
        %ar0 = arith.muli %i0, %aw0 : index
        %ax0 = arith.addi %ar0, %k0 : index
        %br0 = arith.muli %bcx0n0, %bw0 : index
        %bx0 = arith.addi %br0, %j0 : index
        %z0 = memref.load %c[%cx0] : memref<128xf32>
        %x0 = memref.load %a[%ax0] : memref<192xf32>
        %y0 = memref.load %b[%bx0] : memref<192xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%cx0] : memref<128xf32>
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
