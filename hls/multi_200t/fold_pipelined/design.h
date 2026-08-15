#ifndef DESIGN_H
#define DESIGN_H

#include <hls_stream.h>

#define R       8
#define C       8
#define ACC_BANKS 4
#define K_MAX   64
#define FOLD_K  8

typedef float data_t;
typedef float acc_t;

/*
 * One reduction beat = one global k.
 *
 * Example:
 *   k=0  -> fold 0, local_k 0
 *   ...
 *   k=7  -> fold 0, local_k 7
 *   k=8  -> fold 1, local_k 0
 */
struct fold_beat_t {
    data_t a[R];
    data_t b[C];

    int global_k;
    int fold;
    int local_k;
};

void matmul_8x8_fold_pipelined(
    data_t A[R][K_MAX],
    data_t B[K_MAX][C],
    data_t Cinout[R][C],
    int K
);

#endif
