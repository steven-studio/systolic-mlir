// RUN: systolic-opt %s --convert-matmul-to-systolic="rows=8 cols=8" | FileCheck %s

// 一个刚好 8x8x8、静态形状的 matmul,应该被完整转成 systolic dialect。
func.func @matmul_8x8x8(%A: tensor<8x8xf32>, %B: tensor<8x8xf32>, %C: tensor<8x8xf32>)
    -> tensor<8x8xf32> {
  // CHECK: %[[STREAM:.*]] = systolic.stream %{{.*}} direction(row) skew(1)
  // CHECK: systolic.pe_array<8x8> stationary(weight) %{{.*}}, %[[STREAM]], %{{.*}}
  %result = linalg.matmul ins(%A, %B : tensor<8x8xf32>, tensor<8x8xf32>)
                          outs(%C : tensor<8x8xf32>) -> tensor<8x8xf32>
  return %result : tensor<8x8xf32>
}

// 形状对不上阵列大小的 matmul,现阶段应该保持原样(阶段 4 才处理 tiling)。
func.func @matmul_16x16x16(%A: tensor<16x16xf32>, %B: tensor<16x16xf32>, %C: tensor<16x16xf32>)
    -> tensor<16x16xf32> {
  // CHECK: linalg.matmul
  // CHECK-NOT: systolic.pe_array
  %result = linalg.matmul ins(%A, %B : tensor<16x16xf32>, tensor<16x16xf32>)
                          outs(%C : tensor<16x16xf32>) -> tensor<16x16xf32>
  return %result : tensor<16x16xf32>
}
