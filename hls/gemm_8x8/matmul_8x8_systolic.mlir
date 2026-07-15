module {
  func.func @matmul_8x8x8(%arg0: tensor<8x8xf32>, %arg1: tensor<8x8xf32>, %arg2: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %c8 = arith.constant 8 : index
    %c1 = arith.constant 1 : index
    %c0 = arith.constant 0 : index
    %0 = systolic.stream %arg0 direction(row) skew(1) : (tensor<8x8xf32>) -> tensor<8x8xf32>
    %1 = scf.for %arg3 = %c0 to %c8 step %c1 iter_args(%arg4 = %arg2) -> (tensor<8x8xf32>) {
      %2 = scf.for %arg5 = %c0 to %c8 step %c1 iter_args(%arg6 = %arg4) -> (tensor<8x8xf32>) {
        %extracted = tensor.extract %arg6[%arg3, %arg5] : tensor<8x8xf32>
        %3 = scf.for %arg7 = %c0 to %c8 step %c1 iter_args(%arg8 = %extracted) -> (f32) {
          %extracted_0 = tensor.extract %0[%arg3, %arg7] : tensor<8x8xf32>
          %extracted_1 = tensor.extract %arg1[%arg7, %arg5] : tensor<8x8xf32>
          %acc_out, %a_out, %b_out = systolic.mac %extracted_0, %extracted_1, %arg8 : (f32, f32, f32) -> (f32, f32, f32)
          scf.yield %acc_out : f32
        }
        %inserted = tensor.insert %3 into %arg6[%arg3, %arg5] : tensor<8x8xf32>
        scf.yield %inserted : tensor<8x8xf32>
      }
      scf.yield %2 : tensor<8x8xf32>
    }
    return %1 : tensor<8x8xf32>
  }
}

