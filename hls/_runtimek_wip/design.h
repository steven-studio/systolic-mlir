// design.h -- geometry, operand types, and the top-level prototype for the
// output-stationary systolic array.
//
// This header owns every constant that appears in the kernel's signature.
// That ownership is the point: A is declared [R][K_MAX] and B is declared
// [K_MAX][C], so R, C and K_MAX must all be complete before the prototype
// is parsed. Defining K_MAX in design.cpp *after* including this file (as
// an earlier revision did) leaves it undefined at the declaration and only
// defined at the definition -- which either fails to compile or, worse,
// compiles two different signatures if some other translation unit picks up
// a different default.
//
// Rule of thumb: if it is in the signature, it lives here.

#ifndef DESIGN_H
#define DESIGN_H

// ---------------------------------------------------------------------
// Array geometry
// ---------------------------------------------------------------------
// R x C processing elements. These set the fill/drain term R + C - 2 and
// the number of operand banks after partitioning, so changing them changes
// both the schedule and the interface.
#ifndef R
#define R 8
#endif

#ifndef C
#define C 8
#endif

// Reduction-length bound. K arrives at runtime; K_MAX only sizes the
// operand buffers and therefore the ap_memory address width
// (ceil(log2(K_MAX)) bits). It does NOT bound TIME_STEPS -- that is the
// whole point of the runtime-K rewrite.
#ifndef K_MAX
#define K_MAX 64
#endif

// ---------------------------------------------------------------------
// Trip-count literals for the systolic beat loop
// ---------------------------------------------------------------------
// #pragma HLS LOOP_TRIPCOUNT wants integer literals; constant expressions
// are accepted inconsistently across Vitis versions and are sometimes
// dropped silently, which leaves the latency report as '?' while looking
// like the pragma took. So the values are spelled out per geometry, and
// anything unrecognised is a hard error rather than a wrong number.
//
//   TS_MIN =         1 + R + C - 2   (shallowest useful K)
//   TS_MAX =     K_MAX + R + C - 2
//   TS_AVG = K_MAX / 2 + R + C - 2
#if (R == 8) && (C == 8) && (K_MAX == 64)
  #define TS_MIN 15
  #define TS_MAX 78
  #define TS_AVG 46
#elif (R == 4) && (C == 4) && (K_MAX == 64)
  #define TS_MIN 7
  #define TS_MAX 70
  #define TS_AVG 38
#elif (R == 4) && (C == 4) && (K_MAX == 16)
  #define TS_MIN 7
  #define TS_MAX 22
  #define TS_AVG 14
#else
  #error "No TRIPCOUNT literals for this R/C/K_MAX. Add a case above -- do \
not fall back to an expression, it may be silently ignored."
#endif

// ---------------------------------------------------------------------
// Operand and accumulator types
// ---------------------------------------------------------------------
// data_t is the wire type and must stay float: it fixes the ap_memory port
// width at 32 bits, and matmul_iface.v and the host both assume that.
typedef float data_t;

// acc_t is internal -- it never reaches a port, so it is the one type that
// can change without touching the RTL.
//
// It is float today, which is what makes the results bit-identical to the
// fixed-K_DIM kernel and keeps the 48-configuration bit-exactness result
// valid. The cost is that acc[i][j] += ... is a loop-carried dependency
// through an fadd whose latency exceeds 1, so II=1 on time_loop depends on
// the tool recognising the accumulator. If csynth reports II > 1, swapping
// acc_t for a fixed-point type is the cheapest lever -- but it breaks
// bit-exactness, so re-run the comparison before believing the speedup.
typedef float acc_t;

// ---------------------------------------------------------------------
// Top-level kernel
// ---------------------------------------------------------------------
// Name is stale -- K is a runtime argument and R/C are macros, so nothing
// here is fixed at 4. It is kept because the top-level function name
// becomes the generated Verilog module name, and renaming it would force an
// edit to matmul_iface.v for no functional gain. Rename on the next pass
// that touches the RTL anyway.
//
// Semantics: Cinout += A[0:R][0:K] * B[0:K][0:C], accumulating in place, so
// a K deeper than K_MAX is issued as successive calls that fold into the
// same C tile. K is clamped to [0, K_MAX] inside the kernel; the host is
// not trusted to stay in range.
void matmul_4x4x4(data_t A[R][K_MAX], data_t B[K_MAX][C], data_t Cinout[R][C],
                  int K);

#endif  // DESIGN_H