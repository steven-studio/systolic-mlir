// Report-only general certificate for nests the fast path owns.
//
// Since Phase 5.4.3d-3b-5 the guarded preflight accepts clean GEMM-shaped
// nests before the independent general (Presburger exact cover) certificate
// ran, so those nests lost the diagnostic "Presburger exact cover verified"
// remark although nothing about their lowering changed. The certificate is now
// computed, read-only, for preflight-accepted nests too, under report=true
// only. It is a diagnostic, not a lowering decision: the fast path still owns
// the nest, the lowered IR is identical with and without the report and with
// and without general-affine, and a nest whose bounded exact analysis cannot
// prove the domain reports UNKNOWN and is still lowered by the fast path.

// RUN: systolic-opt --systolic-declare-fpga-fleet \
// RUN:   --affine-matmul-to-systolic="general-affine=true allow-fp-reassociation=true report=true" \
// RUN:   %s -o %t.report.mlir 2>&1 | FileCheck %s --check-prefix=REPORT
// RUN: systolic-opt --systolic-declare-fpga-fleet \
// RUN:   --affine-matmul-to-systolic="general-affine=true allow-fp-reassociation=true" \
// RUN:   %s -o %t.quiet.mlir 2>&1 | FileCheck %s --check-prefix=QUIET --allow-empty --implicit-check-not='general affine'
// RUN: systolic-opt --systolic-declare-fpga-fleet \
// RUN:   --affine-matmul-to-systolic="allow-fp-reassociation=true report=true" \
// RUN:   %s -o %t.fast.mlir 2>&1 | FileCheck %s --check-prefix=FAST --implicit-check-not='general affine'
// RUN: diff %t.report.mlir %t.quiet.mlir
// RUN: diff %t.report.mlir %t.fast.mlir
// RUN: FileCheck %s --check-prefix=IR < %t.report.mlir

// Without report=true nothing is computed or printed here.
// QUIET-NOT: remark:

// A clean box GEMM: the fast path lowers it; under report=true the general
// analysis certifies the very same nest (128 source MACs, 32 output fibers).
// REPORT-DAG: remark: general affine subspaces: ranks A/B/C/AB/AC/BC/ABC = 2, 2, 2, 3, 3, 3, 3; intersections AB/AC/BC/shared = 1,1,1,0; {{.*}}; source MACs=128; output fibers=32; Presburger exact cover verified
// REPORT-DAG: remark: affine GEMM 4x8x4 lowered as 2 exact region(s)
// FAST: remark: affine GEMM 4x8x4 lowered as 2 exact region(s)
// IR-LABEL: func.func @box_certified
// IR-NOT:     arith.mulf
// IR:         systolic.matmul_tile
// IR-NOT:     arith.mulf
func.func @box_certified(%A: memref<4x4xf32> {llvm.noalias},
                         %B: memref<4x8xf32> {llvm.noalias},
                         %C: memref<4x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %k] : memref<4x4xf32>
        %b = affine.load %B[%k, %j] : memref<4x8xf32>
        %c = affine.load %C[%i, %j] : memref<4x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<4x8xf32>
      }
    }
  }
  return
}

// The skewed operands of SystolicCC/general_affine.c CASE2: GEMM-shaped (the
// fast path owns it), with the certificate of its exact source cover.
// REPORT-DAG: remark: general affine subspaces: ranks A/B/C/AB/AC/BC/ABC = 2, 2, 2, 3, 3, 3, 3; intersections AB/AC/BC/shared = 1,1,1,0; {{.*}}; source MACs=64; output fibers=16; Presburger exact cover verified
// REPORT-DAG: remark: affine GEMM 4x4x4 lowered as 1 exact region(s)
// FAST: remark: affine GEMM 4x4x4 lowered as 1 exact region(s)
// IR-LABEL: func.func @skew_certified
// IR-NOT:     arith.mulf
// IR:         systolic.matmul_tile
// IR-NOT:     arith.mulf
func.func @skew_certified(%A: memref<10x10xf32> {llvm.noalias},
                          %B: memref<10x10xf32> {llvm.noalias},
                          %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i * 2 + %k, %i + %k * 2] : memref<10x10xf32>
        %b = affine.load %B[%k * 2 + %j, %k + %j * 2] : memref<10x10xf32>
        %c = affine.load %C[%i, %j] : memref<4x4xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<4x4xf32>
      }
    }
  }
  return
}

// 4096 source points exceed the bounded analysis' point budget: the general
// analysis reports UNKNOWN (never a manufactured certificate) and the fast
// path lowers the nest exactly as without general-affine.
// REPORT-DAG: remark: general affine analysis: source domain exceeds the point budget [UNKNOWN: bounded exact analysis did not prove the domain]
// REPORT-DAG: remark: affine GEMM 16x16x16 lowered as
// FAST: remark: affine GEMM 16x16x16 lowered as
// IR-LABEL: func.func @box_over_budget
// IR-NOT:     arith.mulf
// IR:         systolic.matmul_tile
// IR-NOT:     arith.mulf
func.func @box_over_budget(%A: memref<16x16xf32> {llvm.noalias},
                           %B: memref<16x16xf32> {llvm.noalias},
                           %C: memref<16x16xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 16 {
      affine.for %k = 0 to 16 {
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
