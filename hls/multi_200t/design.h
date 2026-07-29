#ifndef SYSTOLIC_DESIGN_H
#define SYSTOLIC_DESIGN_H

// 4x4 float32 systolic array.
//
// Interface is deliberately identical to the old matmul_4x4x4: same
// float[4][4] operands, same accumulate-into-C semantics. The existing
// UART protocol, runtime/fpga_matmul4x4.c, and the MLIR lowering that
// emits calls to it all keep working unchanged. Only the internal
// structure changes -- from a broadcast MAC array to a real systolic
// one.

#ifndef R
#define R 4
#endif
#ifndef C
#define C 4
#endif
#ifndef K_DIM
#define K_DIM 4
#endif

typedef float data_t;
typedef float acc_t;

// Same name and signature as the design this replaces.
void matmul_4x4x4(float A[R][K_DIM], float B[K_DIM][C], float Cinout[R][C]);

// Iterations of the time loop. The achieved latency is
//     II * TIME_STEPS + (pipeline depth)
// With a float accumulator II is set by the adder latency, not by 1;
// see README. The point of cosim is to measure what II actually came
// out as, not to assume it.
#define TIME_STEPS (K_DIM + R + C - 2)

#endif