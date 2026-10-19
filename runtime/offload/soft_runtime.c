/*
 * soft_runtime.c -- a SOFTWARE MODEL of the FPGA runtime's entry points,
 * for checking what the compiler emits without the board.
 *
 * NOT the FPGA. It implements, on the CPU, the two contracts a compiled
 * kernel can call:
 *
 *   the asynchronous runtime (runtime/fpga_async_runtime.h):
 *     fpga_gemm_begin / fpga_tile_submit / fpga_gemm_poll / fpga_tile_poll /
 *     fpga_gemm_wait_for / fpga_gemm_wait / fpga_gemm_release /
 *     fpga_async_runtime_close
 *   the synchronous entry point (runtime/fpga_matmul_tiled.h):
 *     fpga_matmul_tiled_auto_scheduled, a wrapper over the former (one
 *     GEMM of one tile, submitted and waited for).
 *
 *   C[M][N] += A[M][K] x B[K][N]       row-major fp32, C with row stride ldc
 *
 * on the 1x8x8 + 3x4x4 device table (device 0: 8x8, devices 1..3: 4x4) with
 * the same refusals as the runtime -- -10 for K outside 1..K_MAX of the
 * device, -11 for an unknown device, a tile larger than the array or ldc <
 * n, -12 for a null pointer, -15 for est_cycles 0, -17 for a tile
 * inconsistent with its GEMM (k_index >= k_count, a k_count that differs
 * from an earlier submit of the tile, a k_index submitted twice, a submit
 * after the GEMM's wait) -- so a compiled kernel that passes here has
 * issued only invocations the real runtime would accept, and its tiles,
 * offsets, K-folding and accumulation through C compute the GEMM. It says
 * nothing about the bitstream, the DPTI link or the array's own
 * floating-point summation order.
 *
 * The model of the asynchronous semantics: a submit copies A and B, queues
 * the job and returns without computing it; the queued jobs of the session
 * run in submission order when the caller polls or waits (one job per
 * poll, every job the GEMM needs per wait), each accumulating its tile into
 * C in k_index order of its output tile -- so a kernel that reads C before
 * its wait sees the model's deferred results, as it would the FPGA's.
 * Fault injection: SOFT_RUNTIME_FAULT=fail makes job SOFT_RUNTIME_FAULT_AT
 * (default 0, counted over the process's submits in order) fail with -8 (a
 * transport timeout) as the real runtime's link fails: every job in flight
 * fails, the GEMM's wait reports -8, and every further submit or begin is
 * refused (-14) until fpga_async_runtime_close() -- the synchronous wrapper
 * closes after each call, so there a fault costs that one invocation, as
 * before. =corrupt makes the job complete (0) with one element of its result
 * tile off by one -- a wrong result the runtime does not report.
 *
 * K_MAX_8X8 / K_MAX_4X4 come from SOFT_K_MAX_8X8 / SOFT_K_MAX_4X4 in the
 * environment, else from the macros SOFT_DEFAULT_K_MAX_8X8 /
 * SOFT_DEFAULT_K_MAX_4X4 it was compiled with (systolic-cc passes the
 * fleet's), else 16 / 16 (the archived bitstream's).
 *
 * SOFT_RUNTIME_MOCK_HARDWARE=1 (TESTS ONLY) makes systolic_offload_stats()
 * report is_hardware = 1, with the backend "MOCK FPGA (software model, test
 * only)", so that a test can exercise what a program does when the FPGA
 * succeeds: the GEMMs it offloaded then report "FPGA Executed" although the
 * software model computed them. Never set it outside tests.
 */

#include "systolic_offload.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The asynchronous API, as fpga_async_runtime.h declares it (the header is
   not included: this file is compiled with the offload headers alone). */
typedef struct fpga_gemm fpga_gemm_t;
typedef enum {
  FPGA_TILE_UNKNOWN = 0, FPGA_TILE_QUEUED, FPGA_TILE_STAGED, FPGA_TILE_IN_FLIGHT,
  FPGA_TILE_HELD, FPGA_TILE_DONE, FPGA_TILE_FAILED
} fpga_tile_state_t;
typedef struct { fpga_tile_state_t state; unsigned job_id; int error; } fpga_tile_status_t;
typedef struct {
  unsigned submitted, queued, staged, in_flight, held, completed, failed;
  int first_error, done, link_failed;
} fpga_gemm_status_t;
#define FPGA_ASYNC_WAIT_FOREVER 0xffffffffu

fpga_gemm_t *fpga_gemm_begin(int M, int N, int K, unsigned tiles_hint);
int fpga_tile_submit(fpga_gemm_t *gemm, unsigned tile_id, unsigned k_index,
                     unsigned k_count, int m, int k, int n, const float *A,
                     const float *B, float *C, int ldc, unsigned device_id,
                     unsigned start_cycle, unsigned est_cycles);
int fpga_gemm_poll(fpga_gemm_t *gemm, fpga_gemm_status_t *out);
int fpga_tile_poll(fpga_gemm_t *gemm, unsigned tile_id, unsigned k_index,
                   fpga_tile_status_t *out);
int fpga_gemm_wait_for(fpga_gemm_t *gemm, unsigned timeout_ms);
int fpga_gemm_wait(fpga_gemm_t *gemm);
int fpga_gemm_release(fpga_gemm_t *gemm);
int fpga_async_runtime_last_error(void);
void fpga_async_runtime_close(void);
int fpga_matmul_tiled_auto_scheduled(int M, int K, int N, const float *A,
                                     const float *B, float *C,
                                     unsigned device_id, unsigned start_cycle,
                                     unsigned est_cycles);

static const int kRows[4] = {8, 4, 4, 4};

#ifndef SOFT_DEFAULT_K_MAX_8X8
#define SOFT_DEFAULT_K_MAX_8X8 16
#endif
#ifndef SOFT_DEFAULT_K_MAX_4X4
#define SOFT_DEFAULT_K_MAX_4X4 16
#endif

/* ---- the model's state --------------------------------------------------- */

typedef struct job {
  struct job *next;              /* the session's queue, submission order */
  fpga_gemm_t *gemm;
  unsigned job_id, tile_id, k_index, k_count, device;
  unsigned long long ordinal;    /* the process's submit number (fault injection) */
  int m, k, n, ldc;
  float *A, *B, *C;              /* A, B: the model's copies */
  fpga_tile_state_t state;
  int error;
} job;

struct fpga_gemm {
  struct fpga_gemm *next;        /* the open GEMMs */
  unsigned id;
  unsigned submitted, outstanding, completed, failed, queued, held;
  int first_error;
  int waited;
  /* Output tiles: k_count and the next fold to accumulate, per tile_id. */
  struct output { unsigned tile_id, k_count, next_k, submitted_mask; int failed; } *outputs;
  size_t noutputs, outputs_cap;
  job **jobs;                    /* every job of the GEMM (owned here), for polling */
  size_t njobs, jobs_cap;
};

static struct {
  fpga_gemm_t *gemms;
  job *queue_head, *queue_tail;
  unsigned long long invocations[4];   /* completed, by device */
  unsigned long long failed_calls;
  int first_error;
  int link_failed, link_error, last_begin_error;
  unsigned epoch, next_seq, next_gemm_id;
  unsigned long long submits;          /* over the process, for SOFT_RUNTIME_FAULT_AT */
  int trace;
} rt = {0, 0, 0, {0, 0, 0, 0}, 0, 0, 0, 0, 0, 0, 0, 1, 0, -1};

static int k_max_of(unsigned device_id) {
  const char *name = device_id == 0 ? "SOFT_K_MAX_8X8" : "SOFT_K_MAX_4X4";
  const char *value = getenv(name);
  if (value)
    return atoi(value);
  return device_id == 0 ? SOFT_DEFAULT_K_MAX_8X8 : SOFT_DEFAULT_K_MAX_4X4;
}

/* "fail", "corrupt" or NULL for submit number `ordinal` (0-based). */
static const char *fault_at(unsigned long long ordinal) {
  const char *fault = getenv("SOFT_RUNTIME_FAULT");
  const char *at = getenv("SOFT_RUNTIME_FAULT_AT");
  if (!fault || ordinal != (unsigned long long)(at ? atoll(at) : 0))
    return NULL;
  return fault;
}

static void note_error(fpga_gemm_t *g, int rc) {
  ++rt.failed_calls;
  if (rt.first_error == 0)
    rt.first_error = rc;
  if (g && g->first_error == 0)
    g->first_error = rc;
}

static fpga_gemm_t *find_gemm(fpga_gemm_t *h) {
  for (fpga_gemm_t *g = rt.gemms; g; g = g->next)
    if (g == h)
      return g;
  return NULL;
}

static struct output *output_of(fpga_gemm_t *g, unsigned tile_id, int create) {
  for (size_t i = 0; i < g->noutputs; ++i)
    if (g->outputs[i].tile_id == tile_id)
      return &g->outputs[i];
  if (!create)
    return NULL;
  if (g->noutputs == g->outputs_cap) {
    g->outputs_cap = g->outputs_cap ? 2 * g->outputs_cap : 8;
    g->outputs = realloc(g->outputs, g->outputs_cap * sizeof *g->outputs);
  }
  struct output *o = &g->outputs[g->noutputs++];
  memset(o, 0, sizeof *o);
  o->tile_id = tile_id;
  return o;
}

static void finish_job(job *j, int error) {
  fpga_gemm_t *g = j->gemm;
  if (j->state == FPGA_TILE_QUEUED && g->queued > 0)
    --g->queued;
  if (j->state == FPGA_TILE_HELD && g->held > 0)
    --g->held;
  j->state = error ? FPGA_TILE_FAILED : FPGA_TILE_DONE;
  j->error = error;
  if (g->outstanding > 0)
    --g->outstanding;
  if (error) {
    ++g->failed;
    note_error(g, error);
  } else {
    ++g->completed;
    ++rt.invocations[j->device];
  }
  free(j->A);
  free(j->B);
  j->A = j->B = NULL;
}

/* The model's link failure: every job not yet run fails, nothing more is
   accepted until fpga_async_runtime_close(). */
static void fail_link(int error) {
  if (rt.link_failed)
    return;
  rt.link_failed = 1;
  rt.link_error = error;
  for (job *j = rt.queue_head; j; j = j->next)
    if (j->state == FPGA_TILE_QUEUED || j->state == FPGA_TILE_HELD)
      finish_job(j, error);
  rt.queue_head = rt.queue_tail = NULL;
}

static void accumulate(job *j) {
  for (int i = 0; i < j->m; ++i)
    for (int c = 0; c < j->n; ++c) {
      float acc = 0.0f;
      for (int kk = 0; kk < j->k; ++kk)
        acc += j->A[i * j->k + kk] * j->B[kk * j->n + c];
      j->C[(size_t)i * j->ldc + c] += acc;
    }
}

/* Run the oldest runnable queued job of the session: the first queued job
   that is the next fold of its output tile (a fold ahead of its
   predecessors is held until they are in). 0 when nothing could run. */
static int run_one(void) {
  for (job *j = rt.queue_head; j; j = j->next) {
    if (j->state != FPGA_TILE_QUEUED)
      continue;
    fpga_gemm_t *g = j->gemm;
    struct output *o = output_of(g, j->tile_id, 0);
    if (j->k_index != o->next_k) {
      j->state = FPGA_TILE_HELD;
      --g->queued;
      ++g->held;
      continue;
    }
    const char *fault = fault_at(j->ordinal);
    if (rt.trace)
      fprintf(stderr,
              "[soft-runtime] job 0x%08x gemm %u tile %u k %u/%u device %u %dx%dx%d\n",
              j->job_id, g->id, j->tile_id, j->k_index, j->k_count, j->device, j->m,
              j->n, j->k);
    if (fault && fault[0] == 'f') {
      /* A transport timeout: the whole link, as fpga_async_runtime.cpp. */
      finish_job(j, -8);
      fail_link(-8);
      return 1;
    }
    if (o->failed) {
      finish_job(j, -14);
      return 1;
    }
    accumulate(j);
    if (fault && fault[0] == 'c')
      j->C[0] += 1.0f;
    finish_job(j, 0);
    ++o->next_k;
    /* Held folds of the same output tile that are next now. */
    for (int progressed = 1; progressed;) {
      progressed = 0;
      for (job *n = rt.queue_head; n; n = n->next)
        if (n->gemm == g && n->tile_id == j->tile_id && n->state == FPGA_TILE_HELD &&
            n->k_index == o->next_k) {
          accumulate(n);
          finish_job(n, 0);
          ++o->next_k;
          progressed = 1;
        }
    }
    return 1;
  }
  return 0;
}

/* Drop finished jobs from the queue's head. */
static void trim_queue(void) {
  while (rt.queue_head && rt.queue_head->state != FPGA_TILE_QUEUED &&
         rt.queue_head->state != FPGA_TILE_HELD)
    rt.queue_head = rt.queue_head->next;
  if (!rt.queue_head)
    rt.queue_tail = NULL;
}

/* ---- the asynchronous API ----------------------------------------------- */

fpga_gemm_t *fpga_gemm_begin(int M, int N, int K, unsigned tiles_hint) {
  if (rt.trace < 0)
    rt.trace = getenv("SOFT_RUNTIME_TRACE") != NULL;
  if (rt.link_failed) {
    rt.last_begin_error = -14;
    return NULL;
  }
  fpga_gemm_t *g = calloc(1, sizeof *g);
  if (!g) {
    rt.last_begin_error = -3;
    return NULL;
  }
  if (rt.epoch == 0)
    rt.epoch = 1;
  g->id = rt.next_gemm_id++;
  g->next = rt.gemms;
  rt.gemms = g;
  rt.last_begin_error = 0;
  (void)M; (void)N; (void)K; (void)tiles_hint;
  return g;
}

int fpga_async_runtime_last_error(void) { return rt.last_begin_error; }

int fpga_tile_submit(fpga_gemm_t *h, unsigned tile_id, unsigned k_index,
                     unsigned k_count, int m, int k, int n, const float *A,
                     const float *B, float *C, int ldc, unsigned device_id,
                     unsigned start_cycle, unsigned est_cycles) {
  (void)start_cycle;
  fpga_gemm_t *g = find_gemm(h);
  if (!g || g->waited)
    return -17;
  if (rt.link_failed)
    return -14;
  if (device_id > 3 || m < 1 || n < 1 || m > kRows[device_id] ||
      n > kRows[device_id])
    return -11;
  if (k < 1 || k > k_max_of(device_id))
    return -10;
  if (!A || !B || !C)
    return -12;
  if (ldc < n)
    return -11;
  if (est_cycles == 0)
    return -15;
  if (k_count == 0 || k_index >= k_count || k_index >= 32)
    return -17;
  struct output *o = output_of(g, tile_id, 1);
  if (o->k_count == 0)
    o->k_count = k_count;
  else if (o->k_count != k_count || (o->submitted_mask & (1u << k_index)))
    return -17;
  o->submitted_mask |= 1u << k_index;

  job *j = calloc(1, sizeof *j);
  j->A = malloc((size_t)m * k * sizeof(float));
  j->B = malloc((size_t)k * n * sizeof(float));
  memcpy(j->A, A, (size_t)m * k * sizeof(float));
  memcpy(j->B, B, (size_t)k * n * sizeof(float));
  j->gemm = g;
  j->job_id = (rt.epoch << 24) | (rt.next_seq++ & 0x00ffffffu);
  j->ordinal = rt.submits;
  j->tile_id = tile_id; j->k_index = k_index; j->k_count = k_count;
  j->device = device_id;
  j->m = m; j->k = k; j->n = n; j->ldc = ldc; j->C = C;
  j->state = FPGA_TILE_QUEUED;
  if (rt.queue_tail)
    rt.queue_tail->next = j;
  else
    rt.queue_head = j;
  rt.queue_tail = j;
  if (g->njobs == g->jobs_cap) {
    g->jobs_cap = g->jobs_cap ? 2 * g->jobs_cap : 16;
    g->jobs = realloc(g->jobs, g->jobs_cap * sizeof *g->jobs);
  }
  g->jobs[g->njobs++] = j;
  ++g->submitted;
  ++g->outstanding;
  ++g->queued;
  ++rt.submits;
  return 0;
}

int fpga_gemm_poll(fpga_gemm_t *h, fpga_gemm_status_t *out) {
  if (out)
    memset(out, 0, sizeof *out);
  fpga_gemm_t *g = find_gemm(h);
  if (!g)
    return -17;
  /* The model's "link" progresses one job per poll. */
  if (!rt.link_failed)
    run_one();
  trim_queue();
  if (!out)
    return 0;
  out->submitted = g->submitted;
  out->queued = g->queued;
  out->held = g->held;
  out->completed = g->completed;
  out->failed = g->failed;
  out->first_error = g->first_error;
  out->done = g->outstanding == 0;
  out->link_failed = rt.link_failed;
  return 0;
}

int fpga_tile_poll(fpga_gemm_t *h, unsigned tile_id, unsigned k_index,
                   fpga_tile_status_t *out) {
  if (out)
    memset(out, 0, sizeof *out);
  fpga_gemm_t *g = find_gemm(h);
  if (!g)
    return -17;
  if (!rt.link_failed)
    run_one();
  trim_queue();
  if (!out)
    return 0;
  for (size_t i = 0; i < g->njobs; ++i) {
    const job *live = g->jobs[i];
    if (live->tile_id == tile_id && live->k_index == k_index) {
      out->state = live->state;
      out->job_id = live->job_id;
      out->error = live->error;
      return 0;
    }
  }
  out->state = FPGA_TILE_UNKNOWN;
  return 0;
}

int fpga_gemm_wait_for(fpga_gemm_t *h, unsigned timeout_ms) {
  (void)timeout_ms;   /* the model's jobs always complete within the call */
  fpga_gemm_t *g = find_gemm(h);
  if (!g)
    return -17;
  g->waited = 1;
  while (g->outstanding > 0 && !rt.link_failed && run_one())
    ;
  trim_queue();
  if (g->outstanding > 0) {
    /* A fold held for a predecessor that was never submitted, or the link
       failed: as the real runtime, the GEMM's wait reports the error. */
    int err = rt.link_failed ? rt.link_error : -17;
    for (size_t i = 0; i < g->njobs; ++i) {
      job *live = g->jobs[i];
      if (live->state == FPGA_TILE_QUEUED || live->state == FPGA_TILE_HELD)
        finish_job(live, err);
    }
    trim_queue();
  }
  return g->first_error;
}

static void free_gemm(fpga_gemm_t *g) {
  fpga_gemm_t **p = &rt.gemms;
  while (*p && *p != g)
    p = &(*p)->next;
  if (*p)
    *p = g->next;
  for (size_t i = 0; i < g->njobs; ++i) {
    job *live = g->jobs[i];
    /* Still queued: unlink it from the session's queue (a released GEMM
       whose link failed, or close). */
    for (job **p = &rt.queue_head; *p; p = &(*p)->next)
      if (*p == live) {
        *p = live->next;
        break;
      }
    rt.queue_tail = NULL;
    for (job *q = rt.queue_head; q; q = q->next)
      rt.queue_tail = q;
    free(live->A);
    free(live->B);
    free(live);
  }
  free(g->jobs);
  free(g->outputs);
  free(g);
}

int fpga_gemm_wait(fpga_gemm_t *h) {
  int rc = fpga_gemm_wait_for(h, FPGA_ASYNC_WAIT_FOREVER);
  if (rc == -17 && !find_gemm(h))
    return -17;
  free_gemm(find_gemm(h));
  return rc;
}

int fpga_gemm_release(fpga_gemm_t *h) {
  fpga_gemm_t *g = find_gemm(h);
  if (!g)
    return -17;
  if (g->outstanding > 0 && !rt.link_failed)
    return -21;
  free_gemm(g);
  return 0;
}

void fpga_async_runtime_close(void) {
  while (rt.gemms)
    free_gemm(rt.gemms);
  rt.queue_head = rt.queue_tail = NULL;
  rt.link_failed = 0;
  rt.link_error = 0;
  rt.epoch = (rt.epoch % 255u) + 1u;
  rt.next_seq = 0;
}

/* ---- the synchronous entry point: one GEMM of one tile ------------------ */

int fpga_matmul_tiled_auto_scheduled(int M, int K, int N, const float *A,
                                     const float *B, float *C,
                                     unsigned device_id, unsigned start_cycle,
                                     unsigned est_cycles) {
  if (rt.trace < 0)
    rt.trace = getenv("SOFT_RUNTIME_TRACE") != NULL;
  if (rt.trace)
    fprintf(stderr,
            "[soft-runtime] device %u tile %dx%d k=%d start_cycle=%u "
            "est_cycles=%u\n",
            device_id, M, N, K, start_cycle, est_cycles);
  fpga_gemm_t *g = fpga_gemm_begin(M, N, K, 1);
  if (!g)
    return rt.last_begin_error;
  int rc = fpga_tile_submit(g, 0, 0, 1, M, K, N, A, B, C, N, device_id,
                            start_cycle, est_cycles);
  if (rc != 0) {
    note_error(g, rc);
    free_gemm(g);
    return rc;
  }
  rc = fpga_gemm_wait(g);
  /* The synchronous contract: each call stands alone, a fault costs that
     one invocation. The link the model failed is reopened for the next. */
  if (rt.link_failed) {
    rt.link_failed = 0;
    rt.link_error = 0;
    rt.epoch = (rt.epoch % 255u) + 1u;
    rt.next_seq = 0;
  }
  return rc;
}

/* ---- systolic_offload.h --------------------------------------------------- */

void systolic_offload_stats(systolic_offload_stats_t *out) {
  const char *mock = getenv("SOFT_RUNTIME_MOCK_HARDWARE");
  out->is_hardware = mock && mock[0] == '1';
  out->backend = out->is_hardware
                     ? "MOCK FPGA (software model, test only)"
                     : "software runtime model (soft_runtime.c), not the FPGA";
  for (int d = 0; d < 4; ++d)
    out->per_device[d] = rt.invocations[d];
  out->failed = rt.failed_calls;
  out->first_error = rt.first_error;
  out->bytes_to_fpga = 0;
  out->bytes_from_fpga = 0;
}

void systolic_offload_close(void) { fpga_async_runtime_close(); }
