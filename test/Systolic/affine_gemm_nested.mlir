func.func @affine_gemm_nested(
    %A: memref<16x16xf32>,
    %B: memref<16x16xf32>,
    %C: memref<16x16xf32>) {

  // Extra enclosing loop, analogous to Livermore Kernel 21's `l`.
  affine.for %l = 0 to 2 {
    affine.for %k = 0 to 16 {
      affine.for %i = 0 to 16 {
        affine.for %j = 0 to 16 {
          %a = affine.load %A[%i, %k] : memref<16x16xf32>
          %b = affine.load %B[%k, %j] : memref<16x16xf32>
          %c = affine.load %C[%i, %j] : memref<16x16xf32>

          %product = arith.mulf %a, %b : f32
          %sum = arith.addf %c, %product : f32

          affine.store %sum, %C[%i, %j] : memref<16x16xf32>
        }
      }
    }
  }

  return
}
