// design.cpp -- output-stationary systolic array with a RUNTIME reduction
// length.
//
// WHAT CHANGES FROM THE FIXED-K_DIM VERSION
// K_DIM was a compile-time constant, so TIME_STEPS was fixed at synthesis
// and a tile deeper than K_DIM had to be issued as several calls. Each call
// re-runs the whole schedule, so the fill/drain term rows+cols-2 -- and the
// init/drain loops around it -- were paid once per K-chunk rather than once
// per fold. On an 8x8 array with K_DIM=8, a K=64 tile therefore cost
// 8 * 28 = 224 cycles against an ideal 78: 29% array utilisation.
//
// Here K arrives as an argument. The array is fed for K + R + C - 2 beats
// in one invocation, so the geometry term is paid once, and the cost model
// becomes
//
//     cycles = II * (K + R + C - 2) + depth
//
// with a single fill/drain regardless of how deep the reduction is. K_MAX
// only bounds the operand buffers, not the schedule.
//
// WHAT IS UNCHANGED
// The datapath, the skew, and the accumulate-into-C semantics are exactly
// as before: PE(i,j) still performs its k-th MAC on beat i+j+k, still reads
// only its own registers and its upstream neighbours', and fill/drain beats
// still multiply by zero, which is exact. Results stay bit-identical to the
// fixed-K_DIM kernel for the same k order -- the 48-configuration
// bit-exactness result carries over unchanged.
//
// WHAT THIS DOES CHANGE ABOVE THE KERNEL   [corrected]
// An earlier draft of this comment claimed the RTL and wire format were
// untouched. That was wrong, and worth stating plainly because the host
// will not work otherwise:
//
//   1. K is a new scalar argument, so the IP gains a control register that
//      the host must write before raising ap_start. matmul_iface.v has to
//      forward it.
//   2. The operand buffers went from [R][K_DIM] to [R][K_MAX], so the
//      ap_memory ports are deeper (3-bit address -> 6-bit at K_MAX=64).
//      Same protocol, wider address bus.
//
// Both are small, additive changes -- no new handshake, no streaming. What
// this file still avoids is the free-running rewrite (ap_ctrl_none with
// hls::stream operands) that would be needed to delete the last per-call
// overhead. That one genuinely reshapes the interface.
//
// WHAT THIS STILL DOES NOT REMOVE
// The init and drain loops, and the ap_start/ap_done handshake, remain
// per-invocation. Measured at K=64 on 8x8 (csynth, 2026-08-06):
//   time_loop  Trip 78 = K + R + C - 2, II 1, Depth 7
//   function   latency 88, Interval 89
// So the real cost is Trip + Depth + init/drain = 88, against an ideal 78
// -- 13% overhead, not the 7.7% an earlier revision of this comment
// claimed. See above for why closing that gap is out of scope here.

// R, C, K_MAX, TS_MIN/MAX/AVG, data_t and acc_t all live in design.h --
// they appear in the signature below, so they must be complete before the
// prototype is parsed, not defined here after the include.
#include "design.h"

// The header picks TRIPCOUNT literals per geometry; these re-derive them so
// a mismatched case fails at compile time rather than mis-reporting latency.
static_assert(TS_MIN == 1 + R + C - 2, "TS_MIN stale: R or C changed");
static_assert(TS_MAX == K_MAX + R + C - 2, "TS_MAX stale: K_MAX/R/C changed");
static_assert(TS_AVG == K_MAX / 2 + R + C - 2, "TS_AVG stale");

// Renamed 2026-08-06 from matmul_4x4x4; see design.h for what a further
// rename would cost. The trailing 8 is wrong -- K is runtime.
void matmul_8x8x8(data_t A[R][K_MAX], data_t B[K_MAX][C],
                  data_t Cinout[R][C], int K) {
  // PARTITIONING -- this is the part that changed, and it matters.
  //
  // With K_DIM=8 a `complete dim=0` partition on A and B was free: 64
  // registers each, and the runtime index k selected among 8. At K_MAX=64
  // the same pragma asks for ~512 registers per operand AND turns every
  // edge feed into a 64:1 float mux -- 8 of them on the A side, 8 on the B
  // side, all on the critical path. That is how you win 2.7x on paper and
  // give it back in II or in a timing failure.
  //
  // The access pattern does not need it. On beat t the A reads are
  // A[i][t-i] for i = 0..R-1: one element per ROW per beat, each from a
  // different row. So partition A on dim=1 only -- R independent memories,
  // one read port each, one read each per beat. The runtime k becomes an
  // address, not a mux. B is the mirror image: reads are B[t-j][j], one per
  // COLUMN, so partition dim=2.
  //
  // Cinout stays fully partitioned; it is R*C and touched by every PE.
#pragma HLS ARRAY_PARTITION variable=A       complete dim=1
#pragma HLS ARRAY_PARTITION variable=B       complete dim=2
#pragma HLS ARRAY_PARTITION variable=Cinout  complete dim=0

  // Per-PE state: one accumulator plus the two inter-PE pipeline registers
  // that make this systolic rather than broadcast.
  acc_t  acc[R][C];
  data_t a_reg[R][C];
  data_t b_reg[R][C];
#pragma HLS ARRAY_PARTITION variable=acc   complete dim=0
#pragma HLS ARRAY_PARTITION variable=a_reg complete dim=0
#pragma HLS ARRAY_PARTITION variable=b_reg complete dim=0

  // K is host-supplied, so it is not trusted. Above K_MAX it would index
  // past the operand buffers; below zero it would make time_steps smaller
  // than the fill/drain geometry. Clamping (rather than asserting) keeps
  // the behaviour defined in hardware, where an assert does nothing.
  const int k_len = (K < 0) ? 0 : ((K > K_MAX) ? K_MAX : K);

init_i:
  for (int i = 0; i < R; i++) {
#pragma HLS UNROLL
  init_j:
    for (int j = 0; j < C; j++) {
#pragma HLS UNROLL
      acc[i][j]   = Cinout[i][j];
      a_reg[i][j] = 0.0f;
      b_reg[i][j] = 0.0f;
    }
  }

  const int time_steps = k_len + R + C - 2;

  // One iteration == one systolic beat. The bound is now a variable, so
  // csynth cannot infer the trip count on its own; without the pragma the
  // latency report comes back as '?' and the cost model would be fitted
  // against nothing.
  //
  // PIPELINE is now stated explicitly rather than left to the auto-pipeline
  // heuristic, which keys off a trip count this loop no longer has at
  // compile time. Check the achieved II in the report -- if it comes back
  // above 1, the cause is almost certainly the loop-carried dependency on
  // acc[i][j] through a float add, whose latency exceeds 1. The levers, in
  // order of preference: a fixed-point acc_t; BIND_OP with a lower-latency
  // fadd; or rotating accumulator banks. Do not just delete the pragma --
  // an unstated II is not the same as II=1.
time_loop:
  for (int t = 0; t < time_steps; t++) {
#pragma HLS LOOP_TRIPCOUNT min=TS_MIN max=TS_MAX avg=TS_AVG
#pragma HLS PIPELINE II=1

    // Walk the grid backwards so a PE reads its upstream neighbour's value
    // from the PREVIOUS beat. Forwards would collapse the row into one
    // combinational path -- i.e. back to broadcast.
  pe_i:
    for (int i = R - 1; i >= 0; i--) {
#pragma HLS UNROLL
    pe_j:
      for (int j = C - 1; j >= 0; j--) {
#pragma HLS UNROLL

        data_t a_in;
        if (j == 0) {                       // west edge: skewed by row
          int k = t - i;
          a_in = (k >= 0 && k < k_len) ? A[i][k] : 0.0f;
        } else {
          a_in = a_reg[i][j - 1];           // from the PE to the left
        }

        data_t b_in;
        if (i == 0) {                       // north edge: skewed by col
          int k = t - j;
          b_in = (k >= 0 && k < k_len) ? B[k][j] : 0.0f;
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