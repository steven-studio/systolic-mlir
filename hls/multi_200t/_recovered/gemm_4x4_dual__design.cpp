// design.cpp -- 4x4 output-stationary SYSTOLIC array.
// FIXED-POINT datapath (ap_fixed<21,5> operands, ap_fixed<26,10>
// accumulators -- see design.h for the derivation) behind the same
// float[4][4] interface.
//
// Drop-in replacement for the previous matmul_4x4x4: same signature,
// same accumulate-into-C semantics, so the UART protocol,
// runtime/fpga_matmul4x4.c and the MLIR lowering are all unaffected.
// float<->fixed conversion happens once per call, in the init and
// drain loops; nothing in time_loop touches a float.
//
// ON II -- HISTORY (float32 version, measured 2026-07-26)
// In the float32 predecessor, Vitis honoured II=1 by flattening the
// fadd into combinational logic (C_LATENCY=0, 21.353 ns), capping
// post-route Fmax at ~37.6 MHz. That phenomenon is a property of the
// FLOAT datapath and is expected to vanish here: the fixed-point
// recurrence is a 26-bit add, a single carry chain.
//
// The II design point is selected from the .cfg with
//     syn.directive.pipeline=matmul_4x4x4/time_loop II=<n>
// NOT with a macro in the pragma (Vitis does not macro-expand pragmas).

#include "design.h"

void matmul_4x4x4(float A[R][K_DIM], float B[K_DIM][C], float Cinout[R][C]) {
#pragma HLS ARRAY_PARTITION variable=A       complete dim=0
#pragma HLS ARRAY_PARTITION variable=B       complete dim=0
#pragma HLS ARRAY_PARTITION variable=Cinout  complete dim=0

  acc_t  acc[R][C];
  data_t a_reg[R][C];
  data_t b_reg[R][C];
#pragma HLS ARRAY_PARTITION variable=acc   complete dim=0
#pragma HLS ARRAY_PARTITION variable=a_reg complete dim=0
#pragma HLS ARRAY_PARTITION variable=b_reg complete dim=0

  // Fixed-point copies of the operands. NOTE: the conversion loops
  // below are deliberately NOT unrolled. HLS converts float->fixed
  // via double (fpext 32->64 + 54-bit shifter); unrolled x48 this
  // cost 86k LUT vs the 35T's 20.8k. Rolled, converters are shared;
  // +~100 cycles per call, invisible behind the 31 ms UART.
  // Converted ONCE here so the
  // float->fixed converters sit outside time_loop; the edge PEs then
  // read pure fixed-point values every beat.
  data_t Afix[R][K_DIM];
  data_t Bfix[K_DIM][C];
#pragma HLS ARRAY_PARTITION variable=Afix complete dim=0
#pragma HLS ARRAY_PARTITION variable=Bfix complete dim=0

init_i:
  for (int i = 0; i < R; i++) {
  init_j:
    for (int j = 0; j < C; j++) {
      acc[i][j]   = (acc_in_t)Cinout[i][j];   // accumulate into C, as before
      a_reg[i][j] = (data_t)0;
      b_reg[i][j] = (data_t)0;
    }
  }

init_conv_a_i:
  for (int i = 0; i < R; i++) {
  init_conv_a_k:
    for (int k = 0; k < K_DIM; k++) {
      Afix[i][k] = (data_in_t)A[i][k];        // round-to-nearest, saturate
    }
  }
init_conv_b_k:
  for (int k = 0; k < K_DIM; k++) {
  init_conv_b_j:
    for (int j = 0; j < C; j++) {
      Bfix[k][j] = (data_in_t)B[k][j];
    }
  }

  // One iteration == one systolic beat.
time_loop:
  for (int t = 0; t < TIME_STEPS; t++) {
    // NO pipeline pragma here on purpose; II comes from the .cfg.
    // Walk the grid backwards so a PE reads its upstream neighbour's
    // value from the PREVIOUS beat. Forwards would collapse the row
    // into one combinational path -- i.e. back to broadcast.
  pe_i:
    for (int i = R - 1; i >= 0; i--) {
#pragma HLS UNROLL
    pe_j:
      for (int j = C - 1; j >= 0; j--) {
#pragma HLS UNROLL

        data_t a_in;
        if (j == 0) {                       // west edge: skewed by row
          int k = t - i;
          a_in = (k >= 0 && k < K_DIM) ? Afix[i][k] : (data_t)0;
        } else {
          a_in = a_reg[i][j - 1];           // from the PE to the left
        }

        data_t b_in;
        if (i == 0) {                       // north edge: skewed by col
          int k = t - j;
          b_in = (k >= 0 && k < K_DIM) ? Bfix[k][j] : (data_t)0;
        } else {
          b_in = b_reg[i - 1][j];           // from the PE above
        }

        acc[i][j] += a_in * b_in;           // zeros during fill/drain

        a_reg[i][j] = a_in;                 // hand on for the next beat
        b_reg[i][j] = b_in;
      }
    }
  }

drain_i:
  for (int i = 0; i < R; i++) {
  drain_j:
    for (int j = 0; j < C; j++) {
      Cinout[i][j] = (float)acc[i][j];      // fixed -> float, exact to
    }                                       // ~2^-16, far inside 1e-3
  }
}
