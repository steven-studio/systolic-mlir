// Case ID: mlir_009
// Name: triangular_bounds
// Set / category: negative / coordinate_dependency
// Kind: MLIR-level case (the affine nest as the C front end would emit it, or as
//   the front end cannot express it); stages: recognition, dense partition, lowering.
//   No C program, nothing is executed: the verdict covers those stages only.
// Mathematical class: gemm_values_triangular_region
//
// Mathematical operation:
//   j runs from i: only the upper triangle of C (j >= i) accumulates A B.  The
//   iteration domain is not a box, so the computed elements are not one
//   rectangular GEMM of strided views (the specification's definition needs a
//   box).  Not a dense GEMM; must be refused.  The recognizer accepts the access
//   pattern; getDenseLogicalGemm refuses the non-static (counter-dependent)
//   bound.  The C front end cannot express this nest (bounds must be constants).
//
// Mathematical GEMM: NO
// Expected recognition (specification): REJECT
//   basis: the nest is not a dense GEMM of its operands (analyse(): see why_not_a_dense_gemm)
// Current implementation (predicted from the compiler sources; measured by
// scripts/run_showcase.py, never assumed):
//   recognition (recognizeLogicalMatmul):    ACCEPT
//   dense partition (getDenseLogicalGemm):   unsupported
//   lowering (--affine-matmul-to-systolic):  not_reached
//   rejection at: dense_partition
//   reason: the loop bounds are not static, so the iteration domain has no static normalized extent
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
//     showcase/negative/mlir_009.mlir -o /dev/null
//   build/bin/systolic-opt --systolic-declare-fpga-fleet --affine-matmul-to-systolic=report=true \
//     showcase/negative/mlir_009.mlir -o /dev/null
//   Stage by stage, results collected: scripts/run_showcase.py --case mlir_009

func.func @triangular_bounds(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = affine_map<(d0) -> (d0)>(%i) to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i, %k] : memref<8x8xf32>
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
