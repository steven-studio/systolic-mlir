#ifndef DESIGN_H
#define DESIGN_H

#include <hls_stream.h>

#define R       8
#define C       8

#define K_MAX   64
#define FOLD_K  8

#define MAX_FOLDS ((K_MAX + FOLD_K - 1) / FOLD_K)

#define TS_MIN  (1 + R + C - 2)
#define TS_MAX  (K_MAX + R + C - 2)
#define TS_AVG  (K_MAX / 2 + R + C - 2)

typedef float data_t;
typedef float acc_t;

/*
 * One systolic-array input beat.
 *
 * a_edge[i] is the value injected into the west side of row i.
 * b_edge[j] is the value injected into the north side of column j.
 *
 * The producer already performs the row/column skew.
 */
struct systolic_beat_t {
    data_t a_edge[R];
    data_t b_edge[C];
};

void matmul_8x8_fold_pipelined(
    data_t A[R][K_MAX],
    data_t B[K_MAX][C],
    data_t Cinout[R][C],
    int K
);

#endif