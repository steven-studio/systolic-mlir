#ifndef FPGA_ASYNC_RUNTIME_H
#define FPGA_ASYNC_RUNTIME_H

#include "fpga_matmul_tiled.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The asynchronous FPGA runtime: many invocations in flight across the
 * 1x8x8 + 3x4x4 bitstream, on one DPTI session, from one host thread.
 *
 * The scheduled entry point (fpga_matmul_tiled.h) is synchronous: every
 * call stages one operand slab, submits one descriptor and reads the result
 * before it returns, so the four accelerators run one at a time and the
 * link is idle while the host waits. This runtime separates the three
 * steps. A GEMM is opened with fpga_gemm_begin; its invocations ("tiles")
 * are handed over with fpga_tile_submit, which packs the operand slab at
 * once and returns; the runtime stages slabs, submits descriptors and
 * collects completion records whenever it is called (there is no thread of
 * its own -- a submit or a wait pumps the link); fpga_gemm_wait returns once
 * every tile of the GEMM has been accumulated into its C.
 *
 * It needs a bitstream with the completion record (COMPLETION_HEADER=1 in
 * SYSTOLIC_FPGA_CONFIG, docs/ASYNC.md phase 2): every result is attributed
 * by the job_id the record carries, never by order, and a refused descriptor
 * comes back as a record with a status instead of wedging the link.
 *
 * Per device the runtime keeps one descriptor in flight and one further
 * slab staged in DDR ("credits": desc 1, stage 1), because the descriptor
 * bridge of the bitstream blocks on a busy device (head-of-line) -- more
 * outstanding descriptors are not an option without a change there. The
 * slab of the next tile is written while the current one computes (the
 * transfer/compute overlap, SYSTOLIC_ASYNC_STAGE=0 turns it off for the
 * measurements of docs/ASYNC.md). Each device owns two operand slots and two
 * result slots in DDR, alternated, so a staged slab never overwrites the
 * slab an array is reading.
 *
 * Semantics of a tile: C[i*ldc + j] += (A x B)[i][j] for the m x n tile,
 * exactly as fpga_matmul_tiled_auto_scheduled accumulates, and the tiles of
 * one output tile (same tile_id) are accumulated in k_index order whatever
 * order their results arrive in, so a GEMM folded over K gives the same
 * bits as the synchronous path. A, B are read during fpga_tile_submit and
 * may be reused afterwards; C must stay valid until fpga_gemm_wait returns.
 *
 * Return codes are those of fpga_matmul_tiled.h where they apply (-3 .. -16,
 * -14 refused after a link failure), plus
 *   -17  the tile is inconsistent with the GEMM: k_index >= k_count, a
 *        k_count that differs from an earlier submit of the same tile_id,
 *        a k_index submitted twice, or a submit after the GEMM's wait
 *   -18  the bitstream refused the descriptor (completion status BAD_K or
 *        BAD_DEV): this tile ran nowhere; the link stays in step
 *   -19  the bitstream has no completion record (COMPLETION_HEADER is not
 *        1 in the configuration): this runtime cannot drive it
 * A link failure (-3, -4, -6, -7, -8, -9) fails every tile in flight and
 * everything submitted afterwards (-14) until fpga_async_runtime_close.
 *
 * Environment:
 *   SYSTOLIC_FPGA_CONFIG         the bitstream's fpga_config.txt (required
 *                                in practice: COMPLETION_HEADER=1)
 *   SYSTOLIC_ASYNC_STAGE=0|1     stage the next slab while a device computes
 *                                (default 1)
 *   SYSTOLIC_ASYNC_QUEUE_LIMIT   packed tiles held before a submit blocks on
 *                                the link (default 64)
 *   SYSTOLIC_ASYNC_READ_POLICY   when a non-blocking pump reads the link:
 *                                blocked (default: only once nothing more
 *                                can be written), always, wait (only in
 *                                fpga_gemm_wait and at the queue limit)
 *   SYSTOLIC_ASYNC_VIOLATE=1     send k = K_MAX + 1 to the bitstream instead
 *                                of refusing it on the host (tests of the
 *                                refusal path only)
 *   SYSTOLIC_DPTI_TIMEOUT_MS     no byte from the link for this long while
 *                                a descriptor is in flight: -8 (default 5000)
 *   SYSTOLIC_RUNTIME_TIMING=1    a "[fpga-async] timing ..." line at close
 *   SYSTOLIC_RUNTIME_TRACE=1     one line per link event
 */

typedef struct fpga_gemm fpga_gemm_t;

/* Open a GEMM of M x N x K (recorded for the trace and the timing line;
 * the tiles carry their own shapes). tiles_hint is the number of tiles the
 * caller will submit, 0 if unknown. NULL when the runtime cannot run at all
 * (no configuration, no completion record, the link failed earlier): the
 * reason is on stderr, and fpga_async_runtime_last_error() has its code. */
fpga_gemm_t *fpga_gemm_begin(int M, int N, int K, unsigned tiles_hint);

/* Submit one invocation of gemm. tile_id names the output tile (any value;
 * tiles sharing it accumulate into the same C and are ordered by k_index),
 * k_index in 0..k_count-1 its position in that accumulation. m, k, n, A, B,
 * device_id, start_cycle and est_cycles are fpga_matmul_tiled_auto_scheduled's;
 * C points at the tile's top-left element with row stride ldc (elements).
 * 0 when the tile was packed and queued; the tile's own outcome is reported
 * by fpga_gemm_wait. */
int fpga_tile_submit(fpga_gemm_t *gemm,
                     unsigned tile_id, unsigned k_index, unsigned k_count,
                     int m, int k, int n,
                     const float *A, const float *B, float *C, int ldc,
                     unsigned device_id,
                     unsigned start_cycle,
                     unsigned est_cycles);

/* Pump the link until every tile of gemm has been accumulated (or failed),
 * then release gemm. 0, or the first error of any of its tiles: C then
 * holds the tiles that did complete, accumulated in order, and nothing of
 * the ones that did not. */
int fpga_gemm_wait(fpga_gemm_t *gemm);

/* The last error fpga_gemm_begin reported, 0 if none. */
int fpga_async_runtime_last_error(void);

typedef struct {
    unsigned long long tiles_submitted;
    unsigned long long tiles_completed;             /* results accumulated */
    unsigned long long tiles_failed;
    unsigned long long tiles_per_device[4];         /* completed, by device */
    unsigned long long gemms_begun;
    unsigned long long gemms_waited;
    unsigned long long bytes_to_fpga;
    unsigned long long bytes_from_fpga;
    unsigned long long reads;                       /* read_some calls */
    unsigned long long empty_reads;                 /* ... that returned 0 */
    unsigned long long mem_writes;
    unsigned long long descriptors;
    unsigned long long max_queued;                  /* packed, not yet staged */
    /* Wall-clock, nanoseconds: inside fpga_tile_submit (packing included),
     * inside fpga_gemm_wait, and the span from the first submit to the last
     * wait of the session. */
    unsigned long long ns_submit;
    unsigned long long ns_wait;
    unsigned long long ns_span;
    /* Pump passes that saw >= 2 / all 4 devices with a descriptor in
     * flight, and passes that saw a slab staged behind a running job. */
    unsigned long long passes;
    unsigned long long passes_2_inflight;
    unsigned long long passes_4_inflight;
    unsigned long long passes_staged_behind;
    int first_error;
} fpga_async_stats_t;

void fpga_async_runtime_get_stats(fpga_async_stats_t *out);
void fpga_async_runtime_reset_stats(void);

/* Close the session; every GEMM still open is dropped (its tiles are not
 * accumulated) and the link is left as it is. The next begin opens afresh
 * with a new job_id epoch, so a late completion of the old session is
 * recognised and discarded. */
void fpga_async_runtime_close(void);

#ifdef __cplusplus
}
#endif

#endif
