func.func @affine_gemm_nonzero_origin(
    %A: memref<5x5xf32>,
    %B: memref<5x5xf32>,
    %C: memref<5x5xf32>) {
  affine.for %x = 1 to 5 {
    affine.for %y = 1 to 5 {
      affine.for %z = 1 to 5 {
        %a = affine.load %A[%x, %z] : memref<5x5xf32>
        %b = affine.load %B[%z, %y] : memref<5x5xf32>
        %c = affine.load %C[%x, %y] : memref<5x5xf32>

        %product = arith.mulf %a, %b : f32
        %sum = arith.addf %c, %product : f32

        affine.store %sum, %C[%x, %y] : memref<5x5xf32>
      }
    }
  }

  return
}
