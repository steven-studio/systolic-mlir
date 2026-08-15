#include "design.h"

#ifndef __SYNTHESIS__
#include <cstdio>
#endif


/*
 * ============================================================
 * Stage 1: produce one K beat at a time.
 * ============================================================
 *
 * There is NO whole-fold packet.
 *
 * Therefore:
 *
 *   fold0/local7
 *          ->
 *   fold1/local0
 *
 * can appear on consecutive stream transfers.
 */
static void load_beats(
    data_t A[R][K_MAX],
    data_t B[K_MAX][C],
    hls::stream<fold_beat_t> &beat_stream,
    int k_len)
{
#pragma HLS INLINE off

load_loop:
    for (int k = 0; k < k_len; ++k) {
#pragma HLS PIPELINE II=1
#pragma HLS LOOP_TRIPCOUNT min=1 max=K_MAX avg=32

        fold_beat_t beat;

        beat.global_k = k;
        beat.fold      = k / FOLD_K;
        beat.local_k   = k % FOLD_K;

    load_a:
        for (int i = 0; i < R; ++i) {
#pragma HLS UNROLL
            beat.a[i] = A[i][k];
        }

    load_b:
        for (int j = 0; j < C; ++j) {
#pragma HLS UNROLL
            beat.b[j] = B[k][j];
        }

        beat_stream.write(beat);
    }
}

template<int BANK>
static void update_acc_bank(
    acc_t acc_bank[ACC_BANKS][R][C],
    int i,
    int j,
    data_t a_in,
    data_t b_in)
{
#pragma HLS INLINE

    data_t product =
        a_in * b_in;

    acc_t next_acc =
        acc_bank[BANK][i][j] + product;

#pragma HLS BIND_OP variable=next_acc op=fadd impl=fulldsp latency=3

    acc_bank[BANK][i][j] =
        next_acc;
}

/*
 * ============================================================
 * One physical systolic beat.
 * ============================================================
 *
 * raw_a[] / raw_b[] contain one global-k slice.
 *
 * a_skew:
 *   row i delayed by i beats
 *
 * b_skew:
 *   column j delayed by j beats
 *
 * PE propagation then contributes the remaining j/i delays.
 *
 * Thus A[i][k] and B[k][j] meet at PE(i,j) at:
 *
 *   t = k + i + j
 */
static void systolic_step(
    data_t raw_a[R],
    data_t raw_b[C],
    acc_t acc_bank[ACC_BANKS][R][C],
    int acc_sel,
    data_t a_reg[R][C],
    data_t b_reg[R][C],
    data_t a_skew[R][R],
    data_t b_skew[C][C])
{
#pragma HLS INLINE

    data_t a_edge[R];
    data_t b_edge[C];

#pragma HLS ARRAY_PARTITION variable=a_edge complete dim=0
#pragma HLS ARRAY_PARTITION variable=b_edge complete dim=0


/*
 * A row skew.
 */
a_skew_loop:
    for (int i = 0; i < R; ++i) {
#pragma HLS UNROLL

        if (i == 0) {
            a_edge[i] = raw_a[i];
        }
        else {
            a_edge[i] = a_skew[i][i - 1];

        a_shift:
            for (int d = R - 1; d > 0; --d) {
#pragma HLS UNROLL
                if (d < i) {
                    a_skew[i][d] =
                        a_skew[i][d - 1];
                }
            }

            a_skew[i][0] = raw_a[i];
        }
    }


/*
 * B column skew.
 */
b_skew_loop:
    for (int j = 0; j < C; ++j) {
#pragma HLS UNROLL

        if (j == 0) {
            b_edge[j] = raw_b[j];
        }
        else {
            b_edge[j] = b_skew[j][j - 1];

        b_shift:
            for (int d = C - 1; d > 0; --d) {
#pragma HLS UNROLL
                if (d < j) {
                    b_skew[j][d] =
                        b_skew[j][d - 1];
                }
            }

            b_skew[j][0] = raw_b[j];
        }
    }


/*
 * PE grid.
 *
 * Walk backwards so each PE observes upstream state from
 * the previous systolic beat.
 */
pe_i:
    for (int i = R - 1; i >= 0; --i) {
#pragma HLS UNROLL

    pe_j:
        for (int j = C - 1; j >= 0; --j) {
#pragma HLS UNROLL

            data_t a_in =
                (j == 0)
                    ? a_edge[i]
                    : a_reg[i][j - 1];

            data_t b_in =
                (i == 0)
                    ? b_edge[j]
                    : b_reg[i - 1][j];

            data_t product =
                a_in * b_in;

            acc_t next_acc =
                acc_bank[acc_sel][i][j] + product;

            #pragma HLS BIND_OP variable=next_acc op=fadd impl=fulldsp latency=3

            acc_bank[acc_sel][i][j] =
                next_acc;

            a_reg[i][j] =
                a_in;

            b_reg[i][j] =
                b_in;
        }
    }
}


/*
 * ============================================================
 * Stage 2: consume K beats continuously.
 * ============================================================
 */
static void compute_beats(
    hls::stream<fold_beat_t> &beat_stream,
    data_t Cinout[R][C],
    int k_len)
{
#pragma HLS INLINE off

acc_t acc_bank[ACC_BANKS][R][C];

#pragma HLS ARRAY_PARTITION variable=acc_bank complete dim=0

    data_t a_reg[R][C];
    data_t b_reg[R][C];

    data_t a_skew[R][R];
    data_t b_skew[C][C];

#pragma HLS ARRAY_PARTITION variable=a_reg  complete dim=0
#pragma HLS ARRAY_PARTITION variable=b_reg  complete dim=0
#pragma HLS ARRAY_PARTITION variable=a_skew complete dim=0
#pragma HLS ARRAY_PARTITION variable=b_skew complete dim=0
#pragma HLS ARRAY_PARTITION variable=Cinout complete dim=0


/*
 * Initialize ONCE for the entire reduction.
 */
init_i:
for (int i = 0; i < R; ++i) {
#pragma HLS UNROLL

init_j:
    for (int j = 0; j < C; ++j) {
#pragma HLS UNROLL

        init_bank:
        for (int b = 0; b < ACC_BANKS; ++b) {
        #pragma HLS UNROLL
            acc_bank[b][i][j] =
                (b == 0)
                    ? Cinout[i][j]
                    : (acc_t)0;
        }

        a_reg[i][j] = (data_t)0;
        b_reg[i][j] = (data_t)0;
        a_skew[i][j] = (data_t)0;
        b_skew[i][j] = (data_t)0;
    }
}


/*
 * ------------------------------------------------------------
 * Effective K beats.
 * ------------------------------------------------------------
 *
 * IMPORTANT:
 *
 * systolic_t is the logical systolic-array beat number.
 *
 * There is NO reset, flush or idle at:
 *
 *   k=7 -> k=8
 *   k=15 -> k=16
 *   ...
 */
const int total_steps =
    k_len + R + C - 2;

systolic_loop:
for (int systolic_t = 0;
     systolic_t < total_steps;
     ++systolic_t)
{
#pragma HLS PIPELINE II=1
#pragma HLS LOOP_TRIPCOUNT min=14 max=78 avg=46

    data_t raw_a[R];
    data_t raw_b[C];

#pragma HLS ARRAY_PARTITION variable=raw_a complete dim=0
#pragma HLS ARRAY_PARTITION variable=raw_b complete dim=0

    if (systolic_t < k_len) {

        fold_beat_t beat =
            beat_stream.read();

    copy_a:
        for (int i = 0; i < R; ++i) {
#pragma HLS UNROLL
            raw_a[i] =
                beat.a[i];
        }

    copy_b:
        for (int j = 0; j < C; ++j) {
#pragma HLS UNROLL
            raw_b[j] =
                beat.b[j];
        }

#ifndef __SYNTHESIS__
        std::printf(
            "[BEAT t=%2d] COMPUTE "
            "fold=%d local_k=%d global_k=%d\n",
            systolic_t,
            beat.fold,
            beat.local_k,
            beat.global_k
        );
#endif

    }
    else {

    zero_a:
        for (int i = 0; i < R; ++i) {
#pragma HLS UNROLL
            raw_a[i] =
                (data_t)0;
        }

    zero_b:
        for (int j = 0; j < C; ++j) {
#pragma HLS UNROLL
            raw_b[j] =
                (data_t)0;
        }

#ifndef __SYNTHESIS__
        std::printf(
            "[BEAT t=%2d] FINAL FLUSH %d/%d\n",
            systolic_t,
            systolic_t - k_len,
            R + C - 3
        );
#endif
    }

    const int acc_sel =
        systolic_t & (ACC_BANKS - 1);

    systolic_step(
        raw_a,
        raw_b,
        acc_bank,
        acc_sel,
        a_reg,
        b_reg,
        a_skew,
        b_skew
    );
}


/*
 * Write result ONCE.
 */
drain_i:
for (int i = 0; i < R; ++i) {
#pragma HLS UNROLL

drain_j:
    for (int j = 0; j < C; ++j) {
#pragma HLS UNROLL

        acc_t sum =
            acc_bank[0][i][j];

    reduce_bank:
        for (int b = 1; b < ACC_BANKS; ++b) {
#pragma HLS UNROLL
            sum += acc_bank[b][i][j];
        }

        Cinout[i][j] =
            sum;
    }
}
}


/*
 * ============================================================
 * Top.
 * ============================================================
 */
void matmul_8x8_fold_pipelined(
    data_t A[R][K_MAX],
    data_t B[K_MAX][C],
    data_t Cinout[R][C],
    int K)
{
#pragma HLS ARRAY_PARTITION variable=A      complete dim=1
#pragma HLS ARRAY_PARTITION variable=B      complete dim=2
#pragma HLS ARRAY_PARTITION variable=Cinout complete dim=0

    const int k_len =
        (K < 0)
            ? 0
            : ((K > K_MAX)
                ? K_MAX
                : K);

    hls::stream<fold_beat_t> beat_stream;

#pragma HLS STREAM variable=beat_stream depth=2

/*
 * Producer and consumer are separate DATAFLOW processes.
 *
 * Producer:
 *   k0 k1 ... k7 k8 ...
 *
 * Consumer:
 *      k0 k1 ... k7 k8 ...
 *
 * No whole-fold packet has to be completed before the next
 * stage can begin.
 */
#pragma HLS DATAFLOW

    load_beats(
        A,
        B,
        beat_stream,
        k_len
    );

    compute_beats(
        beat_stream,
        Cinout,
        k_len
    );
}
