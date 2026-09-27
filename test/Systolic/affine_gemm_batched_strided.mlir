#mapA = affine_map<(d0, d1, d2, d3) -> (d0, d1 * 2 + 7, d3)>
#mapB = affine_map<(d0, d1, d2, d3) -> (d0, d3, d2)>
#mapC = affine_map<(d0, d1, d2, d3) -> (d0, d1 * 2 + 7, d2)>

module {
  func.func @affine_gemm_batched_strided(
      %arg0: memref<2x204x4xf32>,
      %arg1: memref<2x4x4xf32>,
      %arg2: memref<2x204x4xf32>) {

    affine.for %b = 0 to 2 {
      affine.for %i = 0 to 20 {
        affine.for %j = 0 to 4 {
          affine.for %k = 0 to 4 {

            %a = affine.load %arg0[%b, %i * 2 + 7, %k]
                : memref<2x204x4xf32>

            %bb = affine.load %arg1[%b, %k, %j]
                : memref<2x4x4xf32>

            %c = affine.load %arg2[%b, %i * 2 + 7, %j]
                : memref<2x204x4xf32>

            %mul = arith.mulf %a, %bb : f32
            %add = arith.addf %c, %mul : f32

            affine.store %add, %arg2[%b, %i * 2 + 7, %j]
                : memref<2x204x4xf32>
          }
        }
      }
    }

    return
  }
}
