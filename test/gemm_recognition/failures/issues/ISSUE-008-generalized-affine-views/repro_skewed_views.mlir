// ISSUE-008: GEMMs whose operand views are affine but not strided sub-matrices
// (docs/AFFINE_GEMM_MAPPING.md section 19) -- a dimension combining both of an
// operand's roles. Recognized and verified since 2026-10-16 (the readers
// analysis: G1, G2); the lowering does not build such a view yet (it needs a
// memref.reinterpret_cast, not a memref.subview) and refuses them with "X has a
// skewed or merged view ... which is not lowered yet"
// [VALID_BUT_LOWERING_UNSUPPORTED]. The specification requires them lowered:
// this is the XFAIL reproducer of the open part of the issue; once the lowering
// builds the views it XPASSes, then the XFAIL line is removed and the file is a
// permanent regression test.
//
//   @test1     Test 1 of the request: C[i][j] += A[i][i + k] * B[j + k][j]
//              (showcase ext_026)
//   @skewed_c  the user's seed C[2i + j][j] += A[2i + 3][k + 1] * B[k + 2][3j + 2]
//              (showcase case_088)
//   @skewed_a  C[i][j] += A[i][i + k] * B[k][j] (showcase case_126)
//
// XFAIL: *
// RUN: systolic-opt "--affine-matmul-to-systolic=report=true allow-fp-reassociation=true classify=true" \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s
//
// CHECK:     remark: affine GEMM 4x4x4 lowered as 1 exact region(s)
// CHECK:     remark: affine GEMM 4x4x4 lowered as 1 exact region(s)
// CHECK:     remark: affine GEMM 8x8x8 lowered as 1 exact region(s)
// CHECK-NOT: remark:

func.func @test1(%A: memref<4x7xf32> {llvm.noalias}, %B: memref<7x4xf32> {llvm.noalias}, %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %i + %k] : memref<4x7xf32>
        %b = affine.load %B[%j + %k, %j] : memref<7x4xf32>
        %c = affine.load %C[%i, %j] : memref<4x4xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<4x4xf32>
      }
    }
  }
  return
}

// -----
func.func @skewed_c(%A: memref<10x5xf32> {llvm.noalias}, %B: memref<6x12xf32> {llvm.noalias}, %C: memref<10x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i * 2 + 3, %k + 1] : memref<10x5xf32>
        %b = affine.load %B[%k + 2, %j * 3 + 2] : memref<6x12xf32>
        %c = affine.load %C[%i * 2 + %j, %j] : memref<10x4xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i * 2 + %j, %j] : memref<10x4xf32>
      }
    }
  }
  return
}

// -----
func.func @skewed_a(%A: memref<8x15xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i, %i + %k] : memref<8x15xf32>
        %b = affine.load %B[%k, %j] : memref<8x8xf32>
        %c = affine.load %C[%i, %j] : memref<8x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<8x8xf32>
      }
    }
  }
  return
}
