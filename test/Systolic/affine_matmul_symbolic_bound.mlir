module {
  func.func @symbolic_bound(
      %p: index,
      %A: memref<100x100xf32>,
      %B: memref<100x100xf32>,
      %C: memref<100x100xf32>) {

    affine.for %p2 = 0 to 4 {
      affine.for %i = 0 to 100 {
        affine.for %p1 = 0 to %p {
          affine.for %j = 0 to 100 {
            affine.for %k = 0 to 100 {

              %a = affine.load %A[%i, %k]
                : memref<100x100xf32>

              %b = affine.load %B[%k, %j]
                : memref<100x100xf32>

              %c = affine.load %C[%i, %j]
                : memref<100x100xf32>

              %mul = arith.mulf %a, %b : f32
              %sum = arith.addf %c, %mul : f32

              affine.store %sum, %C[%i, %j]
                : memref<100x100xf32>
            }
          }
        }
      }
    }

    return
  }
}
