// A loop step of zero or below is not an affine.for at all: the IR verifier
// rejects it before any pass runs. The recognizer (normalizeLogicalCoordinates,
// getDenseLogicalGemm) therefore only ever sees steps > 0 -- for which a loop
// with upper <= lower is exactly a loop that runs zero times -- and a C loop
// that counts down reaches it as an ascending ordinal loop (see
// affine_gemm_loop_steps.mlir and test/CFront/downward.c).
//
// RUN: systolic-opt -split-input-file -verify-diagnostics %s

func.func @zero_step(%A: memref<8x8xf32>) {
  // expected-error @+1 {{expected step to be representable as a positive signed integer}}
  affine.for %i = 0 to 8 step 0 {
    %a = affine.load %A[%i, %i] : memref<8x8xf32>
  }
  return
}

// -----

func.func @negative_step(%A: memref<8x8xf32>) {
  // expected-error @+1 {{expected step to be representable as a positive signed integer}}
  affine.for %i = 5 to 0 step -2 {
    %a = affine.load %A[%i, %i] : memref<8x8xf32>
  }
  return
}
