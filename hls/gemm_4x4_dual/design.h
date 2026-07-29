#ifndef SYSTOLIC_DESIGN_H
#define SYSTOLIC_DESIGN_H

// 4x4 systolic array. FIXED-POINT datapath behind a FLOAT interface.
//
// Interface is deliberately identical to the old matmul_4x4x4: same
// float[4][4] operands, same accumulate-into-C semantics. The existing
// UART protocol, runtime/fpga_matmul4x4.c, and the MLIR lowering that
// emits calls to it all keep working unchanged. float<->fixed
// conversion happens once at the boundary (init/drain loops); the
// time_loop datapath is pure fixed-point.
//
// WORD LENGTHS -- derived, not guessed (Monte Carlo vs the actual
// tb_accum.cpp distribution and tolerance, 2026-07-29):
//   operands: uniform on [-10, 10] step 0.01  ->  5 integer bits
//     (incl. sign) cover +/-16; 16 fractional bits give 0 tolerance
//     failures in 3.2M comparisons under tb_accum's 1e-3 relative
//     test (14 frac bits: 12 failures; 12: 0.68%; 8: 16.5%).
//   accumulator: worst case |Cinit| + 4*|a*b| = 10 + 4*100 = 410
//     (observed max 328)  ->  10 integer bits cover +/-512. Overflow
//     is therefore impossible for in-range inputs; no saturation
//     logic needed inside the loop.
// The float32 fadd C_LATENCY=0 phenomenon (see the signoff worklog,
// 2026-07-26) is specific to the floating-point datapath and will NOT
// reproduce here: a 26-bit fixed add is a single carry chain.

#include <ap_fixed.h>

// NOTE: this include MUST come before the R/C/K_DIM defines. Vitis's
// own hls_half_x_utils.h has a template parameter named R; with
// `#define R 4` in scope first, csim dies on
// `template <std::float_round_style 4, ...>`. csynth does not catch
// this (the __SYNTHESIS__ path skips that header) -- only csim does.

#ifndef R
#define R 4
#endif
#ifndef C
#define C 4
#endif
#ifndef K_DIM
#define K_DIM 4
#endif

// Internal datapath types. Plain quantization modes (no AP_SAT/AP_RND)
// so that same-type register-to-register moves in the array synthesize
// to bare wires.
typedef ap_fixed<21, 5>  data_t;   // operands: 5 int (incl sign) + 16 frac
typedef ap_fixed<26, 10> acc_t;    // accumulator: 10 int (incl sign) + 16 frac

// Boundary-conversion types: round-to-nearest + saturate, used ONLY
// where a float enters the fixed domain (init loop). This matches the
// quantization model the word lengths were derived under, and keeps
// the rounding/saturation logic off the time_loop recurrence path.
typedef ap_fixed<21, 5,  AP_RND, AP_SAT> data_in_t;
typedef ap_fixed<26, 10, AP_RND, AP_SAT> acc_in_t;

// NOTE: there is deliberately no TARGET_II macro here any more.
// Driving the pragma from a macro does not work -- Vitis reads pragma
// text without macro expansion, so `#pragma HLS PIPELINE II=TARGET_II`
// is parsed as an unrecognised II and silently falls back to II=1. The
// II is set from the .cfg instead, via syn.directive.pipeline.

// Same name and signature as the design this replaces.
void matmul_4x4x4(float A[R][K_DIM], float B[K_DIM][C], float Cinout[R][C]);

// Iterations of the time loop -- one systolic beat each. This is the
// quantity the cost model predicts.
#define TIME_STEPS (K_DIM + R + C - 2)

#endif
