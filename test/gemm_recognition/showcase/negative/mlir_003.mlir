// Case ID: mlir_003
// Name: oob_offset_plus_extent_by_one
// Set / category: negative / out_of_bounds
// Kind: MLIR-level case (the affine nest as the C front end would emit it, or as
//   the front end cannot express it); stages: recognition, dense partition, lowering.
//   No C program, nothing is executed: the verdict covers those stages only.
// Mathematical class: undefined_out_of_bounds
//
// Mathematical operation:
//   Offsets 2 with extents 7 on 8x8 arrays: the last row/column touched is 2 + 6 =
//   8, one past the end on every operand but B's rows.  The boundary of the bounds
//   check: offset + scale * (extent - 1) = 8 is not < 8.
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
//   reason: A is accessed out of bounds along dimension 0
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
//     showcase/negative/mlir_003.mlir -o /dev/null
//   build/bin/systolic-opt --systolic-declare-fpga-fleet --affine-matmul-to-systolic=report=true \
//     showcase/negative/mlir_003.mlir -o /dev/null
//   Stage by stage, results collected: scripts/run_showcase.py --case mlir_003

func.func @oob_offset_plus_extent_by_one(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 7 {
    affine.for %j = 0 to 7 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i + 2, %k] : memref<8x8xf32>
        %b = affine.load %B[%k, %j + 2] : memref<8x8xf32>
        %c = affine.load %C[%i + 2, %j + 2] : memref<8x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i + 2, %j + 2] : memref<8x8xf32>
      }
    }
  }
  return
}
