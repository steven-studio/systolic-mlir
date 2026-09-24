func.func @affine_gemm(
    %A: memref<4x4xf32>,
    %B: memref<4x4xf32>,
    %C: memref<4x4xf32>) {
  affine.for %k = 0 to 4 {
    affine.for %i = 0 to 4 {
      affine.for %j = 0 to 4 {
        %a = affine.load %A[%i, %k] : memref<4x4xf32>
        %b = affine.load %B[%k, %j] : memref<4x4xf32>
        %c = affine.load %C[%i, %j] : memref<4x4xf32>

        %product = arith.mulf %a, %b : f32
        %sum = arith.addf %c, %product : f32

        affine.store %sum, %C[%i, %j] : memref<4x4xf32>
      }
    }
  }

  return
}
