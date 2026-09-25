func.func @affine_gemm_singular_mapping(
    %A: memref<7x13xf32>,
    %B: memref<13x7xf32>,
    %C: memref<7x7xf32>) {
  affine.for %x = 0 to 4 {
    affine.for %y = 0 to 4 {
      affine.for %z = 0 to 4 {
        %a = affine.load %A[%x + %z, %x + %y + 2 * %z]
            : memref<7x13xf32>
        %b = affine.load %B[%x + %y + 2 * %z, %y + %z]
            : memref<13x7xf32>
        %c = affine.load %C[%x + %z, %y + %z]
            : memref<7x7xf32>

        %product = arith.mulf %a, %b : f32
        %sum = arith.addf %c, %product : f32

        affine.store %sum, %C[%x + %z, %y + %z]
            : memref<7x7xf32>
      }
    }
  }

  return
}
