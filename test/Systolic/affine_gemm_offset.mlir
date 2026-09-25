func.func @affine_gemm_offset(
    %A: memref<5x5xf32>,
    %B: memref<5x5xf32>,
    %C: memref<5x5xf32>) {
  affine.for %x = 0 to 4 {
    affine.for %y = 0 to 4 {
      affine.for %z = 0 to 4 {
        %a = affine.load %A[%x + 1, %z + 1] : memref<5x5xf32>
        %b = affine.load %B[%z + 1, %y + 1] : memref<5x5xf32>
        %c = affine.load %C[%x + 1, %y + 1] : memref<5x5xf32>

        %product = arith.mulf %a, %b : f32
        %sum = arith.addf %c, %product : f32

        affine.store %sum, %C[%x + 1, %y + 1] : memref<5x5xf32>
      }
    }
  }

  return
}
