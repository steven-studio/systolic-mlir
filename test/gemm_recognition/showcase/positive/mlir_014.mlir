// Case ID: mlir_014
// Name: per_operand_views_all_coordinates
// Set / category: positive / operand_view
// Kind: MLIR-level case (the affine nest as the C front end would emit it, or as
//   the front end cannot express it); stages: recognition, dense partition, lowering.
//   No C program, nothing is executed: the verdict covers those stages only.
// Mathematical class: gemm
//
// Mathematical operation:
//   case_114 at the MLIR level with static 8x8 shapes: C[0::2, 2:6] += A[1:5, 0::2]
//   B[3:7, 0:4], every shared coordinate spelled differently by its two operands.
//   A dense GEMM of the operands' own views: must be recognized with the views
//   A = [I + 1, 2K], B = [K + 3, J], C = [2I, J + 2] and lowered.
//
// Mathematical GEMM: YES
// Expected recognition (specification): ACCEPT
//   basis: a dense GEMM with affine operand views: include/Systolic/AffineMatmulPartitioning.h ('loops in any order, steps, nonzero origins, access offsets and strides, and coordinates split over several loops ... are all of this form'); include/Systolic/AffineMatmulAnalysis.h (F_A = phiA o (P,I,K), F_B = phiB o (P,K,J), F_C = phiC o (P,I,J): one factor map per operand); the task statement's index family c_i*i + c_j*j + c_k*k + c_0 per operand
// Current implementation (predicted from the compiler sources; measured by
// scripts/run_showcase.py, never assumed):
//   recognition (recognizeLogicalMatmul):    ACCEPT
//   dense partition (getDenseLogicalGemm):   supported
//   lowering (--affine-matmul-to-systolic):  supported
// Classification: TRUE POSITIVE -- see failures/issues/ISSUE-001
//
// Independent reference: none at this level (no program to run); the checks are the
// recognizer's result, the dense-partition result, the operand views it reports and
// the lowering pass's outcome and remark, compared with the lines above.
//
// How to run:
//   build/bin/systolic-opt --systolic-declare-fpga-fleet --test-affine-matmul-discovery \
//     showcase/positive/mlir_014.mlir -o /dev/null
//   build/bin/systolic-opt --systolic-declare-fpga-fleet --affine-matmul-to-systolic=report=true \
//     showcase/positive/mlir_014.mlir -o /dev/null
//   Stage by stage, results collected: scripts/run_showcase.py --case mlir_014

func.func @per_operand_views_all_coordinates(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i + 1, %k * 2] : memref<8x8xf32>
        %b = affine.load %B[%k + 3, %j] : memref<8x8xf32>
        %c = affine.load %C[%i * 2, %j + 2] : memref<8x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i * 2, %j + 2] : memref<8x8xf32>
      }
    }
  }
  return
}
