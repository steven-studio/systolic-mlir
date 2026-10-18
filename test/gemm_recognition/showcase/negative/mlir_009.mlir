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
//   pattern; getDenseLogicalGemm refuses the domain -- since 2026-10-18 with a
//   proof (docs section 19.13.4): the accesses tell the iterations apart (the
//   stacked map (i, k, k, j, i, j) has rank 3), A, B and C are three arrays, no
//   loop is stepped, the bounds fit 64 bits and the set is not centrally
//   symmetric, so it is no affine image of a box and no GEMM in any
//   decomposition (PROVEN_INVALID; 'the loop bounds are not static', UNKNOWN,
//   before).  The C front end translates this nest since the same day (a
//   triangle: bounds that do not shift together, exact subscript ranges by
//   enumeration).
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
//   reason: the iteration domain is not centrally symmetric
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
//
// re-decided 2026-10-18 (docs/AFFINE_GEMM_MAPPING.md section 19.13): the compiler's refusal was 'the loop bounds are not static' (UNKNOWN); it is proven invalid since (central symmetry); the specification's REJECT is unchanged

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
