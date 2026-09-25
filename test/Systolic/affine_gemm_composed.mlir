func.func @affine_gemm_composed(
    %A: memref<8x4xf32>,
    %B: memref<4x4xf32>,
    %C: memref<8x4xf32>) {
  affine.for %x = 0 to 4 {
    affine.for %y = 0 to 4 {
      affine.for %z = 0 to 4 {
        %a = affine.load %A[%x + %y, %z] : memref<8x4xf32>
        %b = affine.load %B[%z, %y] : memref<4x4xf32>
        %c = affine.load %C[%x + %y, %y] : memref<8x4xf32>

        %product = arith.mulf %a, %b : f32
        %sum = arith.addf %c, %product : f32

        affine.store %sum, %C[%x + %y, %y] : memref<8x4xf32>
      }
    }
  }

  return
}
