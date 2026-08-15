#include "design.h"
#ifndef __SYNTHESIS__
#include <cstdio>
#endif

/*
 * ============================================================
 * Stage 1: load reduction folds
 * ============================================================
 *
 * Each invocation of this loop constructs one explicit
 * FOLD_K-sized reduction tile.
 *
 * With K=64 and FOLD_K=8:
 *
 *   packet 0 = k  0.. 7
 *   packet 1 = k  8..15
 *   ...
 *   packet 7 = k 56..63
 *
 * Under HLS DATAFLOW, while compute_folds() consumes packet n,
 * this process can construct packet n+1.
 */
static void load_folds(
    data_t A[R][K_MAX],
    data_t B[K_MAX][C],
    hls::stream<fold_packet_t> &fold_stream,
    int k_len)
{
#pragma HLS INLINE off
#ifndef __SYNTHESIS__
    int load_cycle = 0;
#endif
    const int num_folds =
        (k_len + FOLD_K - 1) / FOLD_K;

load_fold_loop:
    for (int fold = 0; fold < num_folds; ++fold) {

#ifndef __SYNTHESIS__
        std::printf(
            "[LOAD START] fold=%d  global_k=%d..%d\n",
            fold,
            fold * FOLD_K,
            ((fold + 1) * FOLD_K - 1 < k_len)
                ? ((fold + 1) * FOLD_K - 1)
                : (k_len - 1)
        );
#endif

        fold_packet_t packet;

        const int base_k =
            fold * FOLD_K;

        const int remain =
            k_len - base_k;

        packet.valid_k =
            (remain < FOLD_K)
                ? remain
                : FOLD_K;

    load_local_k:
        for (int local_k = 0;
             local_k < FOLD_K;
             ++local_k)
        {
#pragma HLS PIPELINE II=1

            const int global_k =
                base_k + local_k;

#ifndef __SYNTHESIS__
            const int global_cycle =
                fold * FOLD_K + local_k;

            if (local_k < packet.valid_k) {
                std::printf(
                    "[GLOBAL t=%2d] LOAD     fold=%d local_k=%d global_k=%d\n",
                    global_cycle,
                    fold,
                    local_k,
                    global_k
                );
            }
            else {
                std::printf(
                    "[GLOBAL t=%2d] LOAD     fold=%d local_k=%d PAD\n",
                    global_cycle,
                    fold,
                    local_k
                );
            }
#endif

        load_a:
            for (int i = 0; i < R; ++i) {
#pragma HLS UNROLL

                packet.a[local_k][i] =
                    (global_k < k_len)
                        ? A[i][global_k]
                        : (data_t)0;
            }

        load_b:
            for (int j = 0; j < C; ++j) {
#pragma HLS UNROLL

                packet.b[local_k][j] =
                    (global_k < k_len)
                        ? B[global_k][j]
                        : (data_t)0;
            }
        }

        /*
         * This write marks the availability of one complete fold.
         *
         * With FIFO depth=2, one fold can be consumed while
         * the following fold is being assembled.
         */
        fold_stream.write(packet);

#ifndef __SYNTHESIS__
        std::printf(
            "[LOAD DONE ] fold=%d valid_k=%d\n",
            fold,
            packet.valid_k
        );
#endif
    }
}


/*
 * ============================================================
 * One systolic beat
 * ============================================================
 *
 * raw_a[i] and raw_b[j] correspond to one global reduction k.
 *
 * The delay lines create the required systolic skew:
 *
 *   row i    delayed by i cycles
 *   column j delayed by j cycles
 *
 * Those delay lines persist across fold boundaries.
 */
static void systolic_beat(
    data_t raw_a[R],
    data_t raw_b[C],
    acc_t acc[R][C],
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
 * ------------------------------------------------------------
 * A row skew
 * ------------------------------------------------------------
 */
a_skew_rows:
    for (int i = 0; i < R; ++i) {
#pragma HLS UNROLL

        if (i == 0) {
            a_edge[i] = raw_a[i];
        }
        else {
            /*
             * Value leaving the end of row i's delay line.
             */
            a_edge[i] =
                a_skew[i][i - 1];

        a_delay_shift:
            for (int d = R - 1; d > 0; --d) {
#pragma HLS UNROLL

                if (d < i) {
                    a_skew[i][d] =
                        a_skew[i][d - 1];
                }
            }

            a_skew[i][0] =
                raw_a[i];
        }
    }


/*
 * ------------------------------------------------------------
 * B column skew
 * ------------------------------------------------------------
 */
b_skew_cols:
    for (int j = 0; j < C; ++j) {
#pragma HLS UNROLL

        if (j == 0) {
            b_edge[j] = raw_b[j];
        }
        else {
            b_edge[j] =
                b_skew[j][j - 1];

        b_delay_shift:
            for (int d = C - 1; d > 0; --d) {
#pragma HLS UNROLL

                if (d < j) {
                    b_skew[j][d] =
                        b_skew[j][d - 1];
                }
            }

            b_skew[j][0] =
                raw_b[j];
        }
    }


/*
 * ------------------------------------------------------------
 * Actual systolic PE array
 * ------------------------------------------------------------
 *
 * Backward traversal is essential.  Every PE must observe
 * upstream values from the PREVIOUS beat.
 */
pe_i:
    for (int i = R - 1; i >= 0; --i) {
#pragma HLS UNROLL

    pe_j:
        for (int j = C - 1; j >= 0; --j) {
#pragma HLS UNROLL

            data_t a_in;

            if (j == 0) {
                a_in = a_edge[i];
            }
            else {
                a_in =
                    a_reg[i][j - 1];
            }


            data_t b_in;

            if (i == 0) {
                b_in = b_edge[j];
            }
            else {
                b_in =
                    b_reg[i - 1][j];
            }


            acc[i][j] +=
                a_in * b_in;

            a_reg[i][j] =
                a_in;

            b_reg[i][j] =
                b_in;
        }
    }
}


/*
 * ============================================================
 * Stage 2: consume folds
 * ============================================================
 *
 * Important:
 *
 *   - acc[] is initialized ONCE
 *   - a_skew/b_skew are initialized ONCE
 *   - a_reg/b_reg are initialized ONCE
 *
 * None of them are cleared between fold n and fold n+1.
 *
 * Therefore:
 *
 *   fold0 -> fold1 -> fold2
 *
 * is one continuous systolic execution.
 */
static void compute_folds(
    hls::stream<fold_packet_t> &fold_stream,
    data_t Cinout[R][C],
    int k_len)
{
#pragma HLS INLINE off
#ifndef __SYNTHESIS__
    int compute_cycle = 0;
#endif
    acc_t acc[R][C];

    data_t a_reg[R][C];
    data_t b_reg[R][C];

    /*
     * Explicit edge-skew delay lines.
     */
    data_t a_skew[R][R];
    data_t b_skew[C][C];

#pragma HLS ARRAY_PARTITION variable=acc    complete dim=0
#pragma HLS ARRAY_PARTITION variable=a_reg  complete dim=0
#pragma HLS ARRAY_PARTITION variable=b_reg  complete dim=0
#pragma HLS ARRAY_PARTITION variable=a_skew complete dim=0
#pragma HLS ARRAY_PARTITION variable=b_skew complete dim=0
#pragma HLS ARRAY_PARTITION variable=Cinout complete dim=0


/*
 * Initialize the PE array exactly once.
 */
init_i:
    for (int i = 0; i < R; ++i) {
#pragma HLS UNROLL

    init_j:
        for (int j = 0; j < C; ++j) {
#pragma HLS UNROLL

            acc[i][j] =
                Cinout[i][j];

            a_reg[i][j] =
                (data_t)0;

            b_reg[i][j] =
                (data_t)0;

            a_skew[i][j] =
                (data_t)0;

            b_skew[i][j] =
                (data_t)0;
        }
    }


    const int num_folds =
        (k_len + FOLD_K - 1) / FOLD_K;


/*
 * ============================================================
 * FOLD PIPELINE CONSUMER
 * ============================================================
 */
compute_fold_loop:
    for (int fold = 0;
         fold < num_folds;
         ++fold)
    {
        /*
         * Wait for the next complete reduction fold.
         */
        fold_packet_t packet =
            fold_stream.read();

#ifndef __SYNTHESIS__
        std::printf(
            "[COMPUTE START] fold=%d valid_k=%d\n",
            fold,
            packet.valid_k
        );
#endif

    compute_local_k:
        for (int local_k = 0;
             local_k < FOLD_K;
             ++local_k)
        {
#pragma HLS PIPELINE II=1

#ifndef __SYNTHESIS__
            const int global_cycle =
                (fold + 1) * FOLD_K + local_k;

            if (local_k < packet.valid_k) {
                const int global_k =
                    fold * FOLD_K + local_k;

                std::printf(
                    "[GLOBAL t=%2d] COMPUTE  fold=%d local_k=%d global_k=%d\n",
                    global_cycle,
                    fold,
                    local_k,
                    global_k
                );
            }
            else {
                std::printf(
                    "[GLOBAL t=%2d] COMPUTE  fold=%d local_k=%d IDLE\n",
                    global_cycle,
                    fold,
                    local_k
                );
            }
#endif
            if (local_k < packet.valid_k) {

                data_t raw_a[R];
                data_t raw_b[C];

#pragma HLS ARRAY_PARTITION variable=raw_a complete dim=0
#pragma HLS ARRAY_PARTITION variable=raw_b complete dim=0

            raw_a_copy:
                for (int i = 0; i < R; ++i) {
#pragma HLS UNROLL

                    raw_a[i] =
                        packet.a[local_k][i];
                }

            raw_b_copy:
                for (int j = 0; j < C; ++j) {
#pragma HLS UNROLL

                    raw_b[j] =
                        packet.b[local_k][j];
                }


                /*
                 * Notice:
                 *
                 * NO reset here when local_k goes:
                 *
                 *      7 -> 0
                 *
                 * across a fold boundary.
                 */
                systolic_beat(
                    raw_a,
                    raw_b,
                    acc,
                    a_reg,
                    b_reg,
                    a_skew,
                    b_skew
                );
            }
        }

#ifndef __SYNTHESIS__
        std::printf(
            "[COMPUTE DONE ] fold=%d\n",
            fold
        );
#endif
    }

/*
 * ============================================================
 * Final systolic flush
 * ============================================================
 *
 * Only ONE flush is paid after ALL folds.
 *
 * We need R+C-2 additional beats to propagate the last
 * reduction values to PE(R-1,C-1).
 */
#ifndef __SYNTHESIS__
    std::printf("[FINAL FLUSH START]\n");
#endif

flush_loop:
    for (int f = 0;
         f < R + C - 2;
         ++f)
    {
#pragma HLS PIPELINE II=1

#ifndef __SYNTHESIS__
        const int global_cycle =
            (num_folds + 1) * FOLD_K + f;

        std::printf(
            "[GLOBAL t=%2d] FLUSH    beat=%d/%d\n",
            global_cycle,
            f,
            R + C - 3
        );
#endif

        data_t zeros_a[R];
        data_t zeros_b[C];

#pragma HLS ARRAY_PARTITION variable=zeros_a complete dim=0
#pragma HLS ARRAY_PARTITION variable=zeros_b complete dim=0

    zero_a:
        for (int i = 0; i < R; ++i) {
#pragma HLS UNROLL
            zeros_a[i] =
                (data_t)0;
        }

    zero_b:
        for (int j = 0; j < C; ++j) {
#pragma HLS UNROLL
            zeros_b[j] =
                (data_t)0;
        }


        systolic_beat(
            zeros_a,
            zeros_b,
            acc,
            a_reg,
            b_reg,
            a_skew,
            b_skew
        );
    }
#ifndef __SYNTHESIS__
    std::printf("[FINAL FLUSH DONE]\n");
#endif

/*
 * Write C exactly once after all folds.
 */
drain_i:
    for (int i = 0; i < R; ++i) {
#pragma HLS UNROLL

    drain_j:
        for (int j = 0; j < C; ++j) {
#pragma HLS UNROLL

            Cinout[i][j] =
                acc[i][j];
        }
    }
}


/*
 * ============================================================
 * Top level
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


    /*
     * Two-fold queue.
     *
     * Conceptually:
     *
     *   slot 0 = current fold
     *   slot 1 = next fold
     *
     * This is the buffering that allows:
     *
     *   compute(fold n)
     *
     *        overlap
     *
     *   load(fold n+1)
     */
    hls::stream<fold_packet_t> fold_stream;

#pragma HLS STREAM variable=fold_stream depth=2


/*
 * load_folds() and compute_folds() become independent
 * concurrent processes after HLS synthesis.
 */
#pragma HLS DATAFLOW

    load_folds(
        A,
        B,
        fold_stream,
        k_len
    );

    compute_folds(
        fold_stream,
        Cinout,
        k_len
    );
}