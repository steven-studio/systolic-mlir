// Case ID: mlir_011
// Name: symbolic_bound
// Set / category: extended / symbolic_bound
// Kind: MLIR-level case (the affine nest as the C front end would emit it, or as
//   the front end cannot express it); stages: recognition, dense partition, lowering.
//   No C program, nothing is executed: the verdict covers those stages only.
// Mathematical class: gemm_dynamic_extent
//
// Mathematical operation:
//   M = %n, a symbolic extent: a GEMM whose row count is a runtime value.  The C
//   front end cannot produce it (bounds must be constants), and the specification
//   does not say whether the MLIR path must support dynamic extents: open question
//   SQ-3.  Today: recognized (row extent symbolic), no dense form, not lowered.
//
// Mathematical GEMM: YES
// Expected recognition (specification): UNDETERMINED
//   basis: UNDETERMINED -- a loop with a symbolic bound (affine.for 0 to %n: a GEMM extent that is a runtime value): unreachable from the C front end, whose bounds must be constants; is the MLIR path's refusal ('no static normalized extent') a documented limitation, or must dynamic extents be supported?
// Current implementation (predicted from the compiler sources; measured by
// scripts/run_showcase.py, never assumed):
//   recognition (recognizeLogicalMatmul):    ACCEPT
//   dense partition (getDenseLogicalGemm):   unsupported
//   lowering (--affine-matmul-to-systolic):  not_reached
//   rejection at: dense_partition
//   reason: the loop bounds are not static, so the iteration domain has no static normalized extent
// Classification: SPEC OPEN
//   The specification does not decide this case (SQ-3, README.md section 2):
//     a loop with a symbolic bound (affine.for 0 to %n: a GEMM extent that is a
//     runtime value): unreachable from the C front end, whose bounds must be
//     constants; is the MLIR path's refusal ('no static normalized extent') a
//     documented limitation, or must dynamic extents be supported?
//   The compiler's behaviour is recorded, not judged, until the question is settled.
//
// Independent reference: none at this level (no program to run); the checks are the
// recognizer's result, the dense-partition result, the operand views it reports and
// the lowering pass's outcome and remark, compared with the lines above.
//
// How to run:
//   build/bin/systolic-opt --systolic-declare-fpga-fleet --test-affine-matmul-discovery \
//     showcase/extended/mlir_011.mlir -o /dev/null
//   build/bin/systolic-opt --systolic-declare-fpga-fleet --affine-matmul-to-systolic=report=true \
//     showcase/extended/mlir_011.mlir -o /dev/null
//   Stage by stage, results collected: scripts/run_showcase.py --case mlir_011

func.func @symbolic_bound(%A: memref<?x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<?x8xf32> {llvm.noalias}, %n: index) {
  affine.for %i = 0 to %n {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i, %k] : memref<?x8xf32>
        %b = affine.load %B[%k, %j] : memref<8x8xf32>
        %c = affine.load %C[%i, %j] : memref<?x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<?x8xf32>
      }
    }
  }
  return
}
