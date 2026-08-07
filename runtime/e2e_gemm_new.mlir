// e2e_gemm_new.mlir -- end-to-end test shape for the 8x8xK offload path.
//
// 17 x 100 x 9 is chosen so that a single run exercises every boundary the
// pass has to get right:
//
//   M = 17  ->  ceil(17/8) = 3 row tiles, last one 1 row valid of 8
//   N =  9  ->  ceil(9/8)  = 2 col tiles, last one 1 col valid of 8
//   K = 100 ->  ceil(100/64) = 2 chunks, Kc = 64 then Kc = 36
//
// The K tail is the important one: at Kc = 36 the A tile's row stride is 36,
// not 64, so a packer that assumed a fixed stride is wrong here and correct
// for any K that happens to be a multiple of 64. Testing at K = 128 would
// miss it entirely.
//
// llvm.emit_c_interface makes the lowered function callable from C as
// _mlir_ciface_gemm, taking pointers to memref descriptors, instead of the
// exploded scalar-per-descriptor-field ABI that plain LLVM lowering
// produces and that is miserable to call by hand.

func.func @gemm(%a: tensor<17x100xf32>,
                %b: tensor<100x9xf32>,
                %c: tensor<17x9xf32>) -> tensor<17x9xf32>
    attributes {llvm.emit_c_interface} {
  %0 = linalg.matmul
         ins(%a, %b : tensor<17x100xf32>, tensor<100x9xf32>)
         outs(%c : tensor<17x9xf32>) -> tensor<17x9xf32>
  return %0 : tensor<17x9xf32>
}
