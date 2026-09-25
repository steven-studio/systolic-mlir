func.func @affine_gemm_nonunimodular(
    %A: memref<7x4xf32>,
    %B: memref<4x4xf32>,
    %C: memref<7x4xf32>) {
  affine.for %x = 0 to 4 {
    affine.for %y = 0 to 4 {
      affine.for %z = 0 to 4 {
        %a = affine.load %A[2 * %x, %z] : memref<7x4xf32>
        %b = affine.load %B[%z, %y] : memref<4x4xf32>
        %c = affine.load %C[2 * %x, %y] : memref<7x4xf32>

        %product = arith.mulf %a, %b : f32
        %sum = arith.addf %c, %product : f32

        affine.store %sum, %C[2 * %x, %y] : memref<7x4xf32>
      }
    }
  }

  return
}
