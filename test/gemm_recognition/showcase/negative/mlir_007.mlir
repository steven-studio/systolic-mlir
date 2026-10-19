// Case ID: mlir_007
// Name: large_stride_representable_oob
// Set / category: negative / coefficient_overflow
// Kind: MLIR-level case (the affine nest as the C front end would emit it, or as
//   the front end cannot express it); stages: recognition, dense partition, lowering.
//   No C program, nothing is executed: the verdict covers those stages only.
// Mathematical class: undefined_out_of_bounds
//
// Mathematical operation:
//   Stride 2^61 with 4 iterations: the last row, 3 * 2^61, is representable and out
//   of bounds, but the coordinate's exclusive end 4 * 2^61 = 2^63 -- which the
//   partition's half-open regions are computed with -- is not; getDenseLogicalGemm
//   refuses the coordinate for that before the bounds check is reached (ISSUE-002).
//   Undefined either way; refused with the overflow reason.
//
// Mathematical GEMM: NO
// Expected recognition (specification): REJECT
//   basis: the subscripts leave a static array dimension: the nest has no defined meaning as a program, so there is no GEMM to lower and offloading it would be wrong code; the lowering must refuse it and say so (AffineMatmulToSystolic.cpp getLogicalOperand: 'accessed out of bounds along dimension'; AffineMatmulPartitioning.cpp getDenseLogicalGemm when the coordinate's values overflow 64-bit arithmetic -- ISSUE-002)
// Current implementation (predicted from the compiler sources; measured by
// scripts/run_showcase.py, never assumed):
//   recognition (recognizeLogicalMatmul):    ACCEPT
//   dense partition (getDenseLogicalGemm):   unsupported
//   lowering (--affine-matmul-to-systolic):  not_reached
//   rejection at: dense_partition
//   reason: logical coordinate I's scale or extent overflows 64-bit arithmetic
// Classification: TRUE NEGATIVE -- see failures/issues/ISSUE-002
//   Not a dense GEMM the compiler may lower; it must keep refusing it, for the
//   reason above (the lowering pass's remark is compared with it).
//
// Independent reference: none at this level (no program to run); the checks are the
// recognizer's result, the dense-partition result, the operand views it reports and
// the lowering pass's outcome and remark, compared with the lines above.
//
// How to run:
//   build/bin/systolic-opt --systolic-declare-fpga-fleet --test-affine-matmul-discovery \
//     showcase/negative/mlir_007.mlir -o /dev/null
//   build/bin/systolic-opt --systolic-declare-fpga-fleet --affine-matmul-to-systolic=report=true \
//     showcase/negative/mlir_007.mlir -o /dev/null
//   Stage by stage, results collected: scripts/run_showcase.py --case mlir_007

func.func @large_stride_representable_oob(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i * 2305843009213693952, %k] : memref<8x8xf32>
        %b = affine.load %B[%k, %j] : memref<8x8xf32>
        %c = affine.load %C[%i * 2305843009213693952, %j] : memref<8x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i * 2305843009213693952, %j] : memref<8x8xf32>
      }
    }
  }
  return
}
