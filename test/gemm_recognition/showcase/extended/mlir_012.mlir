// Case ID: mlir_012
// Name: split_radix_differs_per_operand
// Set / category: extended / split_coordinate
// Kind: MLIR-level case (the affine nest as the C front end would emit it, or as
//   the front end cannot express it); stages: recognition, dense partition, lowering.
//   No C program, nothing is executed: the verdict covers those stages only.
// Mathematical class: gemm_per_operand_radix
//
// Mathematical operation:
//   case_128 at the MLIR level: I = 4 i0 + i1 on A, 8 i0 + i1 on C.  Open question
//   SQ-1; the recognizer refuses the two expressions as one coordinate.
//
// Mathematical GEMM: YES
// Expected recognition (specification): UNDETERMINED
//   basis: UNDETERMINED -- a coordinate split over several loops with a different mixed radix per operand (A[4*i0 + i1][k] with C[8*i0 + i1][j]): the definition's 'injective affine function of the coordinate's loops' admits it, the 'inside' list does not name it, and the operand views are not strided sub-matrices (a rank-2 memref.subview cannot express them) -- inside the specification (with a copy or a rank-3 view) or a documented limitation?
// Current implementation (predicted from the compiler sources; measured by
// scripts/run_showcase.py, never assumed):
//   recognition (recognizeLogicalMatmul):    REJECT
//   dense partition (getDenseLogicalGemm):   not_reached
//   lowering (--affine-matmul-to-systolic):  not_reached
//   rejection at: recognition
//   reason: not recognized: 4 i0 + i1 and 8 i0 + i1 are not multiples of one linear form
// Classification: SPEC OPEN
//   The specification does not decide this case (SQ-1, README.md section 2):
//     a coordinate split over several loops with a different mixed radix per
//     operand (A[4*i0 + i1][k] with C[8*i0 + i1][j]): the definition's
//     'injective affine function of the coordinate's loops' admits it, the
//     'inside' list does not name it, and the operand views are not strided
//     sub-matrices (a rank-2 memref.subview cannot express them) -- inside the
//     specification (with a copy or a rank-3 view) or a documented limitation?
//   The compiler's behaviour is recorded, not judged, until the question is settled.
//
// Independent reference: none at this level (no program to run); the checks are the
// recognizer's result, the dense-partition result, the operand views it reports and
// the lowering pass's outcome and remark, compared with the lines above.
//
// How to run:
//   build/bin/systolic-opt --systolic-declare-fpga-fleet --test-affine-matmul-discovery \
//     showcase/extended/mlir_012.mlir -o /dev/null
//   build/bin/systolic-opt --systolic-declare-fpga-fleet --affine-matmul-to-systolic=report=true \
//     showcase/extended/mlir_012.mlir -o /dev/null
//   Stage by stage, results collected: scripts/run_showcase.py --case mlir_012

func.func @split_radix_differs_per_operand(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<12x8xf32> {llvm.noalias}) {
  affine.for %i0 = 0 to 2 {
    affine.for %i1 = 0 to 4 {
      affine.for %j = 0 to 8 {
        affine.for %k = 0 to 8 {
          %a = affine.load %A[%i0 * 4 + %i1, %k] : memref<8x8xf32>
          %b = affine.load %B[%k, %j] : memref<8x8xf32>
          %c = affine.load %C[%i0 * 8 + %i1, %j] : memref<12x8xf32>
          %p = arith.mulf %a, %b : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%i0 * 8 + %i1, %j] : memref<12x8xf32>
        }
      }
    }
  }
  return
}
