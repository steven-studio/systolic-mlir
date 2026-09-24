// affine_matmul_k.mlir

func.func @matmul_k() {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
      }
    }
  }
  return
}
