// Case ID: mlir_008
// Name: per_operand_view_offset_overflow
// Set / category: negative / coefficient_overflow
// Kind: MLIR-level case (the affine nest as the C front end would emit it, or as
//   the front end cannot express it); stages: recognition, dense partition, lowering.
//   No C program, nothing is executed: the verdict covers those stages only.
// Mathematical class: undefined_out_of_bounds
//
// Mathematical operation:
//   A's rows are 2^62 i + 2^62, C's 2^62 i: the same coordinate i (scale 1,
//   extent 4: nothing overflows in the dense form), A and C through views of
//   stride 2^62 -- and A's offset plus its span, 2^62 + 2^62 * 3 = 2^64, exceeds
//   int64.  Undefined; the lowering must refuse A's view for the overflow rather
//   than compute a wrapped-around last index (ISSUE-002).
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
//   reason: A's subscripts along dimension 0 overflow 64-bit arithmetic
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
//     showcase/negative/mlir_008.mlir -o /dev/null
//   build/bin/systolic-opt --systolic-declare-fpga-fleet --affine-matmul-to-systolic=report=true \
//     showcase/negative/mlir_008.mlir -o /dev/null
//   Stage by stage, results collected: scripts/run_showcase.py --case mlir_008

func.func @per_operand_view_offset_overflow(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i * 4611686018427387904 + 4611686018427387904, %k] : memref<8x8xf32>
        %b = affine.load %B[%k, %j] : memref<8x8xf32>
        %c = affine.load %C[%i * 4611686018427387904, %j] : memref<8x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i * 4611686018427387904, %j] : memref<8x8xf32>
      }
    }
  }
  return
}
