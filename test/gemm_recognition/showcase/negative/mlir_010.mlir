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
//   of a reversed view of A.  I is i on C and 7 - i on A: opposite directions, a
//   documented limitation until 2026-10-15; lowered since ISSUE-005 (P0.3): A's
//   view runs backwards from row 7 (a negative stride), C's forwards.
//
// Mathematical GEMM: YES
// Expected recognition (specification): ACCEPT
//   basis: (no longer a limitation since 2026-10-15, P0.3) two operands that run through a shared coordinate in opposite directions (A[7 - i] with C[i]): one operand's view has a negative stride (memref.subview with a negative static stride, its offset the largest index), docs/AFFINE_GEMM_MAPPING.md section 5
// Current implementation (predicted from the compiler sources; measured by
// scripts/run_showcase.py, never assumed):
//   recognition (recognizeLogicalMatmul):    ACCEPT
//   dense partition (getDenseLogicalGemm):   supported
//   lowering (--affine-matmul-to-systolic):  supported
// Classification: TRUE POSITIVE -- see failures/issues/ISSUE-005
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
