// design.cpp -- 4x4 float32 output-stationary SYSTOLIC array.
//
// Drop-in replacement for the previous matmul_4x4x4: same signature,
// same float32 datapath, same accumulate-into-C semantics, so the UART
// protocol, runtime/fpga_matmul4x4.c and the MLIR lowering are all
// unaffected.
//
// WHAT WAS WRONG BEFORE
// The previous body was:
//     #pragma HLS ARRAY_PARTITION arg0/arg1/arg2 complete
//     for i UNROLL, for j UNROLL, for k PIPELINE: acc += arg0[i][k]*arg1[k][j]
// Partitioning the operand arrays completely and unrolling i and j
// instantiates 16 independent MAC chains, each reading its operands
// straight out of registers. That is a BROADCAST array: no data moves
// between PEs, there is no skew, and there is no pipeline fill/drain.
// It computes the right product, but the quantity rows+cols-2 has no
// physical referent in it, so it cannot be used to check a systolic
// cost model.
//
// WHAT THIS DOES INSTEAD
//   - each PE owns one accumulator (output-stationary)
//   - A moves left -> right one PE per cycle; B moves top -> bottom
//   - a PE reads ONLY its own registers and its upstream neighbours';
//     only the edge PEs ever touch A[][] or B[][]
//   - edges are fed with a per-lane skew so A[i][k] and B[k][j] land on
//     PE(i,j) on the same cycle
// Consequently PE(i,j) performs its k-th MAC on cycle i+j+k, and the
// array is busy for K + rows + cols - 2 time steps. trace_check.cpp
// asserts exactly this.
//
// ON II AND float
// acc[i][j] += ... is carried across the time loop, so the achievable
// II is bounded by the float adder's latency (2 in the full_dsp
// binding this project already uses). Expect II=2 and a total latency
// near 2*TIME_STEPS rather than TIME_STEPS. That is not a defect: it
// means measured_cycles = II * (K + rows + cols - 2) + depth, and the
// geometry term is still exactly what the cost model says it is. Read
// the achieved II off the cosim report rather than assuming it.

#include "design.h"

void matmul_4x4x4(float A[R][K_DIM], float B[K_DIM][C], float Cinout[R][C]) {
#pragma HLS ARRAY_PARTITION variable=A       complete dim=0
#pragma HLS ARRAY_PARTITION variable=B       complete dim=0
#pragma HLS ARRAY_PARTITION variable=Cinout  complete dim=0

  // Per-PE state: one accumulator plus the two inter-PE pipeline
  // registers that make this systolic rather than broadcast.
  acc_t  acc[R][C];
  data_t a_reg[R][C];
  data_t b_reg[R][C];
#pragma HLS ARRAY_PARTITION variable=acc   complete dim=0
#pragma HLS ARRAY_PARTITION variable=a_reg complete dim=0
#pragma HLS ARRAY_PARTITION variable=b_reg complete dim=0

init_i:
  for (int i = 0; i < R; i++) {
#pragma HLS UNROLL
  init_j:
    for (int j = 0; j < C; j++) {
#pragma HLS UNROLL
      acc[i][j]   = Cinout[i][j];   // accumulate into C, as before
      a_reg[i][j] = 0.0f;
      b_reg[i][j] = 0.0f;
    }
  }

  // One iteration == one systolic beat.
time_loop:
  for (int t = 0; t < TIME_STEPS; t++) {

    // Walk the grid backwards so a PE reads its upstream neighbour's
    // value from the PREVIOUS beat. Forwards would let the whole row
    // collapse into one combinational path -- i.e. back to broadcast.
  pe_i:
    for (int i = R - 1; i >= 0; i--) {
#pragma HLS UNROLL
    pe_j:
      for (int j = C - 1; j >= 0; j--) {
#pragma HLS UNROLL

        data_t a_in;
        if (j == 0) {                       // west edge: skewed by row
          int k = t - i;
          a_in = (k >= 0 && k < K_DIM) ? A[i][k] : 0.0f;
        } else {
          a_in = a_reg[i][j - 1];           // from the PE to the left
        }

        data_t b_in;
        if (i == 0) {                       // north edge: skewed by col
          int k = t - j;
          b_in = (k >= 0 && k < K_DIM) ? B[k][j] : 0.0f;
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
#pragma HLS UNROLL
  drain_j:
    for (int j = 0; j < C; j++) {
#pragma HLS UNROLL
      Cinout[i][j] = acc[i][j];
    }
  }
}