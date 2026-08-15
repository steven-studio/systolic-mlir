#ifndef DESIGN_H
#define DESIGN_H

#include <hls_stream.h>

#define R       8
#define C       8

#define K_MAX   64
#define FOLD_K  8

#define MAX_FOLDS ((K_MAX + FOLD_K - 1) / FOLD_K)

typedef float data_t;
typedef float acc_t;

/*
 * One REAL reduction fold.
 *
 * A fold contains up to FOLD_K consecutive values along K:
 *
 *   fold 0: k =  0.. 7
 *   fold 1: k =  8..15
 *   ...
 *
 * Unlike the old runtime-K implementation, fold is now an
 * explicit producer/consumer data unit.
 */
struct fold_packet_t {
    data_t a[FOLD_K][R];
    data_t b[FOLD_K][C];

    int valid_k;
};

void matmul_8x8_fold_pipelined(
    data_t A[R][K_MAX],
    data_t B[K_MAX][C],
    data_t Cinout[R][C],
    int K
);

#endif