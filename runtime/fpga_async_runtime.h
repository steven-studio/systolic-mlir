#ifndef FPGA_ASYNC_RUNTIME_H
#define FPGA_ASYNC_RUNTIME_H

#include "fpga_matmul_tiled.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The asynchronous FPGA runtime: many invocations in flight across the
 * 1x8x8 + 3x4x4 bitstream, on one DPTI session, driven by a transfer thread
 * of its own.
 *
 * The scheduled entry point (fpga_matmul_tiled.h) is synchronous: every
 * call stages one operand slab, submits one descriptor and reads the result
 * before it returns, so the four accelerators run one at a time and the
 * link is idle while the host waits. This runtime separates the steps. A
 * GEMM is opened with fpga_gemm_begin; its invocations ("tiles") are handed
 * over with fpga_tile_submit, which packs the operand slab, queues the tile
 * and returns -- it never waits for any tile's computation, only, with the
 * host queue full, for room in it; the runtime's transfer thread stages
 * slabs, submits descriptors and collects completion records on its own,
 * attributing every result by the job_id of its record and accumulating it
 * into C; fpga_gemm_poll and fpga_tile_poll report the state without
 * blocking; fpga_gemm_wait_for waits for the GEMM's results with a timeout;
 * fpga_gemm_wait waits for them all and releases the GEMM; fpga_gemm_release
 * releases a GEMM that was polled or waited for.
 *
 * Ownership and lifetimes (2026-10-19):
 *   A, B      read and packed inside fpga_tile_submit; the caller may reuse
 *             or free them as soon as it returns.
 *   C         the caller's; the transfer thread adds the tile's result into
 *             it when the completion record arrives. It must stay allocated
 *             and must not be read or written by the caller from the submit
 *             until fpga_gemm_poll / fpga_tile_poll reports the tile done
 *             (or failed), fpga_gemm_wait_for returns 0 (or an error), or
 *             fpga_gemm_wait returns -- each of which orders the thread's
 *             writes before the caller's reads.
 *   gemm      valid from fpga_gemm_begin until fpga_gemm_wait returns or
 *             fpga_gemm_release / fpga_async_runtime_close; a handle is
 *             never reused while another GEMM is open.
 *   slabs, descriptors, DDR slots, completion records, job_ids: the
 *             runtime's; a job_id (epoch << 24 | sequence) is unique within
 *             a session and reported by fpga_tile_poll and the trace.
 * The API may be called from several caller threads; every call is
 * serialized on the runtime's lock. The transport is used by the transfer
 * thread only, so no caller's call waits for a slab write or a link read
 * other than through the host queue.
 *
 * It needs a bitstream with the completion record (COMPLETION_HEADER=1 in
 * SYSTOLIC_FPGA_CONFIG, docs/ASYNC.md phase 2): every result is attributed
 * by the job_id the record carries, never by order, and a refused descriptor
 * comes back as a record with a status instead of wedging the link.
 *
 * Per device the runtime keeps up to `depth` descriptors in flight -- the
 * bitstream's PIPELINE_DEPTH (fpga_matmul_tiled.h), the job queue of one
 * accelerator context, so that the context can fill, feed and reduce
 * consecutive tiles back to back (docs/COSTMODEL.md section 8); 1 for a
 * bitstream whose record has no such line -- and one further slab staged
 * in DDR ("credits": desc = depth, stage 1). The slab of the next tile is
 * written while the current ones compute (the transfer/compute overlap,
 * SYSTOLIC_ASYNC_STAGE=0 turns it off for the measurements of docs/ASYNC.md,
 * and with it keeps one descriptor in flight). Each in-flight job owns its
 * own operand slot (depth + 1 per device, one for the staged slab) and
 * result slot (depth per device) in DDR; a job's slots are reused only
 * once its completion record has been parsed, so a staged slab never
 * overwrites a slab an array may still read and a result tile is never
 * overwritten before the host has it.
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
 *        a k_index submitted twice, or a submit after the GEMM's wait; and,
 *        from the wait, an output tile not all of whose k_count folds were
 *        accepted (one refused at submit, or never submitted): it can never
 *        be accumulated in order, so its accepted folds fail with -17
 *   -18  the bitstream refused the descriptor (completion status BAD_K or
 *        BAD_DEV): this tile ran nowhere; the link stays in step
 *   -19  the bitstream has no completion record (COMPLETION_HEADER is not
 *        1 in the configuration): this runtime cannot drive it
 *   -20  fpga_gemm_wait_for: the timeout passed with tiles still outstanding
 *        (the GEMM stays open)
 *   -21  fpga_gemm_release: tiles still outstanding (nothing released)
 * A link failure (-3, -4, -6, -7, -8, -9) fails every tile in flight and
 * everything submitted afterwards (-14) until fpga_async_runtime_close.
 *
 * Environment:
 *   SYSTOLIC_FPGA_CONFIG         the bitstream's fpga_config.txt (required
 *                                in practice: COMPLETION_HEADER=1)
 *   SYSTOLIC_ASYNC_STAGE=0|1     stage the next slab while a device computes
 *                                (default 1)
 *   SYSTOLIC_ASYNC_DEPTH=N       descriptors in flight per device (default:
 *                                the configuration's PIPELINE_DEPTH; never
 *                                above it, at most 8; A/B measurements)
 *   SYSTOLIC_ASYNC_BURST=0|1     1 (default): hold a device's descriptors
 *                                until every one of its operand slots holds
 *                                a slab (depth + 1) or a GEMM is waited on,
 *                                so the context gets them back to back and
 *                                pipelines the tiles; 0: send each descriptor
 *                                as soon as its slab is in DDR
 *   SYSTOLIC_ASYNC_QUEUE_LIMIT   packed tiles the host queue holds before a
 *                                submit waits for room (default 64)
 *   SYSTOLIC_ASYNC_BURST_HOLD_US burst mode: a device's first descriptors,
 *                                held for a full burst, go out anyway once
 *                                no tile has been submitted for this long
 *                                (default 1000), so a polled GEMM completes
 *   SYSTOLIC_ASYNC_READ_POLICY   when the transfer thread reads the link:
 *                                blocked (default: once nothing more can be
 *                                written), always (between writes too),
 *                                wait (only while a caller waits or the
 *                                host queue is full)
 *   SYSTOLIC_ASYNC_POLL_US       the thread's pause after an empty read
 *                                (default 50; the FT2232H itself paces an
 *                                empty read by its latency timer), doubling
 *                                with each further empty read up to
 *   SYSTOLIC_ASYNC_POLL_MAX_US   (default 1000)
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

/* Wait until every tile of gemm has been accumulated (or failed), then
 * release gemm. 0, or the first error of any of its tiles: C then holds the
 * tiles that did complete, accumulated in order, and nothing of the ones
 * that did not. Equivalent to fpga_gemm_wait_for(gemm, FPGA_ASYNC_WAIT_FOREVER)
 * followed by fpga_gemm_release(gemm). */
int fpga_gemm_wait(fpga_gemm_t *gemm);

/* ---- observing the work without blocking ------------------------------ */

/* The state of one tile of a GEMM, by the identity the caller gave it
 * (tile_id, k_index). */
typedef enum {
    FPGA_TILE_UNKNOWN = 0,   /* no tile of that identity in this GEMM */
    FPGA_TILE_QUEUED,        /* packed on the host, waiting for an operand slot */
    FPGA_TILE_STAGED,        /* operand slab in DDR, descriptor not yet sent */
    FPGA_TILE_IN_FLIGHT,     /* descriptor sent, completion record pending */
    FPGA_TILE_HELD,          /* result received, waiting for an earlier fold */
    FPGA_TILE_DONE,          /* accumulated into C */
    FPGA_TILE_FAILED         /* refused or failed: error says why */
} fpga_tile_state_t;

typedef struct {
    fpga_tile_state_t state;
    unsigned          job_id;   /* the descriptor's job_id (epoch << 24 | sequence) */
    int               error;    /* FPGA_TILE_FAILED: the code; otherwise 0 */
} fpga_tile_status_t;

typedef struct {
    unsigned submitted;         /* tiles fpga_tile_submit accepted */
    unsigned queued, staged, in_flight, held;
    unsigned completed;         /* accumulated into C */
    unsigned failed;
    int      first_error;       /* of the GEMM's tiles so far, 0 if none */
    int      done;              /* every submitted tile completed or failed */
    int      link_failed;       /* the session's link has failed (nothing more completes) */
} fpga_gemm_status_t;

/* The GEMM's progress, at once, without touching the link (the transfer
 * thread does that). 0, or -17 for a handle that is not an open GEMM. */
int fpga_gemm_poll(fpga_gemm_t *gemm, fpga_gemm_status_t *out);

/* One tile's state, likewise. 0 (state FPGA_TILE_UNKNOWN when no such tile
 * was submitted), or -17 for a handle that is not an open GEMM. */
int fpga_tile_poll(fpga_gemm_t *gemm, unsigned tile_id, unsigned k_index,
                   fpga_tile_status_t *out);

/* Wait until every tile of gemm has been accumulated or failed, at most
 * timeout_ms (FPGA_ASYNC_WAIT_FOREVER: no limit). 0 when they all
 * completed; the first error of the GEMM's tiles (or the link's) when some
 * failed; -20 when the timeout passed first. The GEMM stays open in every
 * case: release it with fpga_gemm_release (or wait with fpga_gemm_wait). No
 * tile may be submitted to the GEMM after its first wait: the first wait
 * (or wait_for, whatever its timeout) seals the GEMM, and every output tile
 * whose k_count folds were not all accepted fails then with -17 -- its folds
 * already held at once, those still on their way when they arrive -- so the
 * GEMM completes (polls see it done) instead of waiting for a fold that
 * never comes, and a refused fold never leaves 0 over a partial K-sum. */
#define FPGA_ASYNC_WAIT_FOREVER 0xffffffffu
int fpga_gemm_wait_for(fpga_gemm_t *gemm, unsigned timeout_ms);

/* Release a GEMM whose tiles are all over (completed or failed) or whose
 * session's link has failed: its handle becomes invalid. -21 while a tile
 * is still outstanding (nothing is released: C would still be written),
 * -17 for a handle that is not an open GEMM. */
int fpga_gemm_release(fpga_gemm_t *gemm);

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
     * flight, passes that saw a slab staged behind a running job, and
     * passes that saw a device with >= 2 descriptors in flight (the
     * pipelined queue in use). */
    unsigned long long passes;
    unsigned long long passes_2_inflight;
    unsigned long long passes_4_inflight;
    unsigned long long passes_staged_behind;
    unsigned long long passes_depth2;
    /* The session's descriptor credit per device and the most descriptors
     * each device had in flight at once. */
    unsigned           depth;
    unsigned long long max_inflight_per_device[4];
    /* Wall-clock, nanoseconds from the first submit: when the last
     * descriptor of the session went out, and when the first completion
     * record came back -- the batch evidence, "submissions precede
     * collections", is last_descriptor < first_completion for a GEMM that
     * fits the credits, and per credit-sized batch otherwise. */
    unsigned long long ns_last_descriptor;
    unsigned long long ns_first_completion;
    /* The submission semantics, counted (docs/ASYNC.md section 10):
     *   descriptors_before_first_completion  descriptors on the link before
     *       the session's first completion record came back: more than one
     *       means the host queued work in the hardware without waiting for
     *       a result (at most depth per device, by the credits);
     *   descriptors_behind_inflight  descriptors sent while an older
     *       descriptor of the same device was still in flight -- the tile
     *       went out without waiting for its predecessor's completion;
     *   credit_waits  descriptors that could only go once a completion had
     *       freed a credit of their device (the hardware queue was full:
     *       they waited for capacity, released by the OLDEST job's
     *       completion, with the previous job still in flight);
     *   submits_blocked / ns_submit_blocked  fpga_tile_submit calls that
     *       waited for room in the host queue (SYSTOLIC_ASYNC_QUEUE_LIMIT)
     *       and the time they spent waiting. Never for a particular job's
     *       completion: for the next completion of any device. */
    unsigned long long descriptors_before_first_completion;
    unsigned long long descriptors_behind_inflight;
    unsigned long long credit_waits;
    unsigned long long submits_blocked;
    unsigned long long ns_submit_blocked;
    int first_error;
    /* Where the time went, nanoseconds (2026-10-19, the transfer thread):
     *   ns_submit         inside fpga_tile_submit, packing and queueing,
     *                     without the time spent waiting for host-queue
     *                     room (that is ns_submit_blocked);
     *   ns_link_write     the transfer thread inside the transport's slab
     *                     writes and descriptor submits;
     *   ns_link_read      the transfer thread inside the transport's reads;
     *   ns_collect        the transfer thread accumulating results into C;
     *   ns_wait           callers inside fpga_gemm_wait / _wait_for.
     * polls counts fpga_gemm_poll + fpga_tile_poll calls, waits_timed_out
     * the fpga_gemm_wait_for calls that returned -20. */
    unsigned long long ns_link_write;
    unsigned long long ns_link_read;
    unsigned long long ns_collect;
    unsigned long long polls;
    unsigned long long waits_timed_out;
} fpga_async_stats_t;

void fpga_async_runtime_get_stats(fpga_async_stats_t *out);
void fpga_async_runtime_reset_stats(void);

/* Stop the transfer thread and close the session; every GEMM still open is
 * dropped (its tiles are not accumulated; nothing is written to any C after
 * this returns) and the link is left as it is. The next begin opens afresh
 * with a new job_id epoch, so a late completion of the old session is
 * recognised and discarded. */
void fpga_async_runtime_close(void);

#ifdef __cplusplus
}
#endif

#endif
