func.func @affine_gemm_loop_3_5_access_7_2(
    %A: memref<204x4xf32>,
    %B: memref<4x4xf32>,
    %C: memref<204x4xf32>) {
  affine.for %x = 3 to 103 step 5 {
    affine.for %y = 0 to 4 {
      affine.for %z = 0 to 4 {
        %a = affine.load %A[7 + 2 * %x, %z] : memref<204x4xf32>
        %b = affine.load %B[%z, %y] : memref<4x4xf32>
        %c = affine.load %C[7 + 2 * %x, %y] : memref<204x4xf32>

        %product = arith.mulf %a, %b : f32
        %sum = arith.addf %c, %product : f32

        affine.store %sum, %C[7 + 2 * %x, %y] : memref<204x4xf32>
      }
    }
  }

  return
}
