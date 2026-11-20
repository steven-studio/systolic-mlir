// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/generalized_fallback.json generalized-fallback=true general-affine=true report=true" -o %t.mlir 2>&1 | FileCheck %s --check-prefix=REMARK
// RUN: FileCheck %s --check-prefix=TILES --input-file=%t.mlir
// RUN: systolic-opt %t.mlir --lower-systolic-matmul-tile-to-fpga | FileCheck %s --check-prefix=SYNC
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="general-affine=true report=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=OFF
// RUN: not systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="generalized-fallback=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=NORECORDS
// RUN: not systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/no_such_records.json generalized-fallback=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=NOFILE
//
// generalized-fallback (docs/GENERALIZED_GEMM_SCOPE.md, "The fallback in systolic-cc"): the
// pass runs every path it has without records first (general-affine here, as systolic-cc
// passes it); then each nest left as it was at the input, in which the recognizer finds no
// GEMM, is lowered from its record, where it stands. Inputs/generalized_fallback.json is what
// generalized-gemm-nest-report says of this file (--index-bit-width=64, every pair of roots
// authorized, --max-level=1 --candidates-per-group=1 --timeout-ms=120000), as systolic-cc
// runs it.
//
// @plain: the standard path's. @perm: rows (5 i) mod 16, no affine view; the standard path
// refuses it and the fallback lowers it by its record's tables. @strict: a GEMM the recognizer
// finds and the floating-point policy refuses (no reassoc): the fallback leaves it alone,
// whatever its record says. @perm_strict: the fallback's own floating-point check refuses.
// REMARK-DAG: remark: affine GEMM 16x8x12 lowered as 5 exact region(s)
// REMARK-DAG: remark: GEMM 16x8x12 of nest 0 of perm lowered from its record as 5 exact region(s) (tables:
// REMARK-DAG: remark: GEMM not lowered from its record: nest 0 of perm_strict: floating-point reassociation is not permitted
// REMARK-NOT: of nest 0 of strict lowered from its record
//
// TILES-LABEL: func.func @plain
// TILES-COUNT-5: systolic.matmul_tile
// TILES-NOT: affine.for
// TILES: return
// TILES-LABEL: func.func @perm
// TILES: memref.get_global @__nest_table_nest_0_of_perm_A
// TILES-COUNT-5: systolic.matmul_tile
// TILES: return
// TILES-LABEL: func.func @strict
// TILES-NOT: systolic.matmul_tile
// TILES: affine.for
// TILES-LABEL: func.func @perm_strict
// TILES-NOT: systolic.matmul_tile
// TILES: affine.for
//
// SYNC-LABEL: func.func @plain
// SYNC-COUNT-5: llvm.call @fpga_matmul_tiled_auto_scheduled
// SYNC-LABEL: func.func @perm
// SYNC-COUNT-5: llvm.call @fpga_matmul_tiled_auto_scheduled
//
// Without the option: the pass as it is; @perm stays on the host.
// OFF-NOT: from its record
// OFF: remark: affine GEMM 16x8x12 lowered as 5 exact region(s)
// OFF-NOT: from its record
//
// NORECORDS: error: affine-matmul-to-systolic: generalized-fallback reads the records of nest-records, and none is given
// NOFILE: error: affine-matmul-to-systolic: nest-records:

func.func @plain(%a: memref<16x12xf32> {llvm.noalias}, %b: memref<12x8xf32> {llvm.noalias}, %c: memref<16x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %z = affine.load %c[%i, %j] : memref<16x8xf32>
        %x = affine.load %a[%i, %k] : memref<16x12xf32>
        %y = affine.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y fastmath<reassoc> : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        affine.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}

func.func @perm(%a: memref<16x12xf32> {llvm.noalias}, %b: memref<12x8xf32> {llvm.noalias}, %c: memref<16x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %z = affine.load %c[%i, %j] : memref<16x8xf32>
        %x = affine.load %a[(%i * 5) mod 16, %k] : memref<16x12xf32>
        %y = affine.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y fastmath<reassoc> : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        affine.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}

func.func @strict(%a: memref<16x12xf32> {llvm.noalias}, %b: memref<12x8xf32> {llvm.noalias}, %c: memref<16x8xf32> {llvm.noalias}) {
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

func.func @perm_strict(%a: memref<16x12xf32> {llvm.noalias}, %b: memref<12x8xf32> {llvm.noalias}, %c: memref<16x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %z = affine.load %c[%i, %j] : memref<16x8xf32>
        %x = affine.load %a[(%i * 5) mod 16, %k] : memref<16x12xf32>
        %y = affine.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m : f32
        affine.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
