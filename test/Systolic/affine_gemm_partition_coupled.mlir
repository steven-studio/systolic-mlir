func.func @affine_gemm_partition_coupled(
    %A: memref<2x5x4xf32>,
    %B: memref<2x4x4xf32>,
    %C: memref<2x5x4xf32>) {
  affine.for %b = 0 to 2 {
    affine.for %i = 0 to 4 {
      affine.for %j = 0 to 4 {
        affine.for %k = 0 to 4 {
          %a = affine.load %A[%b, %b + %i, %k]
              : memref<2x5x4xf32>
          %bb = affine.load %B[%b, %k, %j]
              : memref<2x4x4xf32>
          %c = affine.load %C[%b, %b + %i, %j]
              : memref<2x5x4xf32>
          %mul = arith.mulf %a, %bb : f32
          %add = arith.addf %c, %mul : f32
          affine.store %add, %C[%b, %b + %i, %j]
              : memref<2x5x4xf32>
        }
      }
    }
  }
  return
}
