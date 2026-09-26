func.func @affine_gemm_loop_3_5(
    %A: memref<103x4xf32>,
    %B: memref<4x4xf32>,
    %C: memref<103x4xf32>) {
  affine.for %x = 3 to 103 step 5 {
    affine.for %y = 0 to 4 {
      affine.for %z = 0 to 4 {
        %a = affine.load %A[%x, %z] : memref<103x4xf32>
        %b = affine.load %B[%z, %y] : memref<4x4xf32>
        %c = affine.load %C[%x, %y] : memref<103x4xf32>

        %product = arith.mulf %a, %b : f32
        %sum = arith.addf %c, %product : f32

        affine.store %sum, %C[%x, %y] : memref<103x4xf32>
      }
    }
  }

  return
}
