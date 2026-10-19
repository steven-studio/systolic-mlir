// Case ID: mlir_013
// Name: zero_trip_loop
// Set / category: extended / empty_domain
// Kind: MLIR-level case (the affine nest as the C front end would emit it, or as
//   the front end cannot express it); stages: recognition, dense partition, lowering.
//   No C program, nothing is executed: the verdict covers those stages only.
// Mathematical class: gemm
//
// Mathematical operation:
//   case_129 at the MLIR level (affine.for 0 to 0): open question SQ-2.  Today:
//   recognized, refused by getDenseLogicalGemm as an empty GEMM (its own
//   reason since 2026-10-15; before, the message meant for symbolic bounds).
//
// Mathematical GEMM: YES
// Expected recognition (specification): UNDETERMINED
//   basis: UNDETERMINED -- an empty GEMM (a loop of zero iterations, M, N or K = 0): the nest computes nothing; must the compiler offload it (zero invocations) or refuse it?  Either keeps the result correct; the specification does not say which
// Current implementation (predicted from the compiler sources; measured by
// scripts/run_showcase.py, never assumed):
//   recognition (recognizeLogicalMatmul):    ACCEPT
//   dense partition (getDenseLogicalGemm):   unsupported
//   lowering (--affine-matmul-to-systolic):  not_reached
//   rejection at: dense_partition
//   reason: a loop of the nest has no iteration (an empty GEMM: M, N or K is 0)
// Classification: SPEC OPEN
//   The specification does not decide this case (SQ-2, README.md section 2):
//     an empty GEMM (a loop of zero iterations, M, N or K = 0): the nest
//     computes nothing; must the compiler offload it (zero invocations) or
//     refuse it? Either keeps the result correct; the specification does not say
//     which
//   The compiler's behaviour is recorded, not judged, until the question is settled.
//
// Independent reference: none at this level (no program to run); the checks are the
// recognizer's result, the dense-partition result, the operand views it reports and
// the lowering pass's outcome and remark, compared with the lines above.
//
// How to run:
//   build/bin/systolic-opt --systolic-declare-fpga-fleet --test-affine-matmul-discovery \
//     showcase/extended/mlir_013.mlir -o /dev/null
//   build/bin/systolic-opt --systolic-declare-fpga-fleet --affine-matmul-to-systolic=report=true \
//     showcase/extended/mlir_013.mlir -o /dev/null
//   Stage by stage, results collected: scripts/run_showcase.py --case mlir_013

func.func @zero_trip_loop(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 0 {
    affine.for %j = 0 to 8 {
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
