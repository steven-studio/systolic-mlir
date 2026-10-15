// Case ID: mlir_010
// Name: reversed_coordinate_on_a_only
// Set / category: negative / negative_coefficient
// Kind: MLIR-level case (the affine nest as the C front end would emit it, or as
//   the front end cannot express it); stages: recognition, dense partition, lowering.
//   No C program, nothing is executed: the verdict covers those stages only.
// Mathematical class: gemm
//
// Mathematical operation:
//   C[i][j] += A[7-i][k] B[k][j]: C += (A with its rows reversed) B, a valid GEMM
//   of a reversed view of A.  I is i on C and 7 - i on A: opposite directions,
//   which the specification lists as a documented limitation (a negative stride).
//   The recognizer refuses it (the two expressions are not the same coordinate in
//   the same direction); ext_003 is the same limitation with both operands reversed.
//
// Mathematical GEMM: YES
// Expected recognition (specification): OUT_OF_SCOPE
//   basis: two operands run through a shared coordinate in opposite directions (A[7 - i] with C[i]): no strided view of one operand matches the other (a memref.subview has no negative stride); recognizeLogicalMatmul matchSharedCoordinate refuses it. (A coordinate that decreases in every operand is supported: ISSUE-004)
// Current implementation (predicted from the compiler sources; measured by
// scripts/run_showcase.py, never assumed):
//   recognition (recognizeLogicalMatmul):    REJECT
//   dense partition (getDenseLogicalGemm):   not_reached
//   lowering (--affine-matmul-to-systolic):  not_reached
//   rejection at: recognition
//   reason: not recognized: A's row expression 7 - i and C's i are not one coordinate
// Classification: DOCUMENTED LIMITATION
//   A mathematically valid GEMM that the specification excludes explicitly;
//   'refused' must never be read as 'not a GEMM'.
//
// Independent reference: none at this level (no program to run); the checks are the
// recognizer's result, the dense-partition result, the operand views it reports and
// the lowering pass's outcome and remark, compared with the lines above.
//
// How to run:
//   build/bin/systolic-opt --systolic-declare-fpga-fleet --test-affine-matmul-discovery \
//     showcase/negative/mlir_010.mlir -o /dev/null
//   build/bin/systolic-opt --systolic-declare-fpga-fleet --affine-matmul-to-systolic=report=true \
//     showcase/negative/mlir_010.mlir -o /dev/null
//   Stage by stage, results collected: scripts/run_showcase.py --case mlir_010

func.func @reversed_coordinate_on_a_only(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[7 - %i, %k] : memref<8x8xf32>
        %b = affine.load %B[%k, %j] : memref<8x8xf32>
        %c = affine.load %C[%i, %j] : memref<8x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<8x8xf32>
      }
    }
  }
  return
}
