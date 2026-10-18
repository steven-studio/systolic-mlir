// strided_permuted.mlir -- an affine GEMM that is not a plain row-major
// triple loop, for run_strided.sh: the loops in k, i, j order, and the
// logical matrices embedded in larger memrefs -- A and C rows at 3 + 2 * i,
// B and C columns at 1 + j. The previous --affine-matmul-to-systolic
// gathered this nest's operands assuming loops in I, J, K order and loaded
// the wrong elements; strided_main.c checks every element of C (the rows and
// columns the GEMM does not touch included) against the same loops in C.
func.func @permuted_strided(%A: memref<20x4xf32> {llvm.noalias},
                            %B: memref<4x5xf32> {llvm.noalias},
                            %C: memref<20x5xf32> {llvm.noalias}) {
  affine.for %k = 0 to 4 {
    affine.for %i = 0 to 6 {
      affine.for %j = 0 to 4 {
        %a = affine.load %A[3 + 2 * %i, %k] : memref<20x4xf32>
        %b = affine.load %B[%k, %j + 1] : memref<4x5xf32>
        %c = affine.load %C[3 + 2 * %i, %j + 1] : memref<20x5xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[3 + 2 * %i, %j + 1] : memref<20x5xf32>
      }
    }
  }
  return
}
