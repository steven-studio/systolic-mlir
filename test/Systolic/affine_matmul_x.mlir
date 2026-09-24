// affine_matmul_x.mlir

func.func @matmul_x() {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %x = 0 to 4 {
      }
    }
  }
  return
}
