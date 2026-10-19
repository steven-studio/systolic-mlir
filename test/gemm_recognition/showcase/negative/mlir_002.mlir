// Case ID: mlir_002
// Name: oob_static_dim1_operand_stride
// Set / category: negative / out_of_bounds
// Kind: MLIR-level case (the affine nest as the C front end would emit it, or as
//   the front end cannot express it); stages: recognition, dense partition, lowering.
//   No C program, nothing is executed: the verdict covers those stages only.
// Mathematical class: undefined_out_of_bounds
//
// Mathematical operation:
//   C[i][j] += A[i][k] B[k][3j]: B's column view has stride 3 (a per-operand
//   stride, fine in itself) but 3 * 7 = 21 exceeds B's 8 columns.  Undefined;
//   the lowering must refuse B's view.
//
// Mathematical GEMM: NO
// Expected recognition (specification): REJECT
//   basis: the subscripts leave a static array dimension: the nest has no defined meaning as a program, so there is no GEMM to lower and offloading it would be wrong code; the lowering must refuse it and say so (AffineMatmulToSystolic.cpp getLogicalOperand: 'accessed out of bounds along dimension'; AffineMatmulPartitioning.cpp getDenseLogicalGemm when the coordinate's values overflow 64-bit arithmetic -- ISSUE-002)
// Current implementation (predicted from the compiler sources; measured by
// scripts/run_showcase.py, never assumed):
//   recognition (recognizeLogicalMatmul):    ACCEPT
//   dense partition (getDenseLogicalGemm):   supported
//   lowering (--affine-matmul-to-systolic):  unsupported
//   rejection at: lowering
//   reason: B is accessed out of bounds along dimension 1
// Classification: TRUE NEGATIVE
//   Not a dense GEMM the compiler may lower; it must keep refusing it, for the
//   reason above (the lowering pass's remark is compared with it).
//
// Independent reference: none at this level (no program to run); the checks are the
// recognizer's result, the dense-partition result, the operand views it reports and
// the lowering pass's outcome and remark, compared with the lines above.
//
// How to run:
//   build/bin/systolic-opt --systolic-declare-fpga-fleet --test-affine-matmul-discovery \
//     showcase/negative/mlir_002.mlir -o /dev/null
//   build/bin/systolic-opt --systolic-declare-fpga-fleet --affine-matmul-to-systolic=report=true \
//     showcase/negative/mlir_002.mlir -o /dev/null
//   Stage by stage, results collected: scripts/run_showcase.py --case mlir_002

func.func @oob_static_dim1_operand_stride(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i, %k] : memref<8x8xf32>
        %b = affine.load %B[%k, %j * 3] : memref<8x8xf32>
        %c = affine.load %C[%i, %j] : memref<8x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<8x8xf32>
      }
    }
  }
  return
}
