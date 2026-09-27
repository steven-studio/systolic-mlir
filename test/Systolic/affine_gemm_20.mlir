func.func @affine_gemm(
    %A: memref<20x20xf32>,
    %B: memref<20x20xf32>,
    %C: memref<20x20xf32>) {
  affine.for %i = 0 to 20 {
    affine.for %j = 0 to 20 {
      affine.for %k = 0 to 20 {
        %a = affine.load %A[%i, %k] : memref<20x20xf32>
        %b = affine.load %B[%k, %j] : memref<20x20xf32>
        %c = affine.load %C[%i, %j] : memref<20x20xf32>

        %product = arith.mulf %a, %b : f32
        %sum = arith.addf %c, %product : f32

        affine.store %sum, %C[%i, %j] : memref<20x20xf32>
      }
    }
  }

  return
}
