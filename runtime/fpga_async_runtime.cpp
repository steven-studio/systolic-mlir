// fpga_async_runtime.cpp -- many invocations in flight across the
// 1x8x8 + 3x4x4 bitstream over one DPTI session (see fpga_async_runtime.h).
//
// Two threads. The callers' threads pack operand slabs and queue tiles
// (fpga_tile_submit), observe the state (fpga_gemm_poll, fpga_tile_poll),
// wait (fpga_gemm_wait_for, fpga_gemm_wait) and release GEMMs; the
// runtime's transfer thread owns the DPTI session: it writes what the
// credits allow (slabs, then descriptors), reads what the link has,
// attributes every completion record by its job_id and accumulates results
// into C in k_index order. One mutex guards every structure both sides
// touch; the thread holds it between transport calls, never during one, so
// a submit or a poll never waits for a slab write or a link read. Two
// condition variables: cv_worker wakes the thread (new tiles, a waiter, a
// stop), cv_caller wakes callers (a completion, room in the host queue, a
// failure).
//
// The FT2232H's synchronous FIFO carries one direction at a time and the
// RTL's PHY (dpti_sync245_phy.sv, or dpti_bus_arbiter.sv on the legacy pads
// path) alternates them, so the host may write while a result is on its
// way back; what the host must not do is read in a tight loop, since an
// empty read costs a latency-timer period on the chip -- hence the read
// policy below.
//
// Per device the host keeps up to `depth` descriptors in flight -- the
// bitstream's PIPELINE_DEPTH, the job queue of one accelerator context
// (systolic_accel_context.sv JOB_QUEUE_DEPTH; 1 for a bitstream without the
// record) -- each with its own operand slab and result tile in DDR, plus
// one further slab staged ahead. A job's slots are freed when its completion
// record has been parsed: the context read the slab before it folded the
// tile, and the result tile left DDR before the record's payload reached
// the host. Within the depth the hardware never holds a descriptor back
// (acceptance needs only queue room), so the descriptor bridge's
// head-of-line never blocks another device's descriptor.

#include "fpga_async_runtime.h"
#include "dpti_runtime.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <thread>
#include <unordered_map>
#include <vector>

namespace {

using clock_t_ = std::chrono::steady_clock;

/* The DDR layout. OPERAND_BASE and RESULT_BASE are the regions the board
 * runs validated (fpga_matmul_tiled.cpp); each device owns depth + 1
 * operand slabs (one per descriptor in flight, one staged ahead) from
 * OPERAND_BASE + device * operand_slots * slab_bytes (slab_bytes is a
 * multiple of 512, so every slot keeps the MEM_WRITE's 256-byte alignment)
 * and depth result tiles of RESULT_SLOT_BYTES from RESULT_BASE. At depth 4
 * and K_MAX = 16 that is 0x10000..0x15000 and 0x3000..0x4000. MAX_DEPTH
 * keeps the result region below OPERAND_BASE (0x3000 + 4 * 8 * 256 =
 * 0x5000); the configuration's DDR_BYTES (the co-simulation's model
 * records it) bounds the operand region, see resolve_depth. */
constexpr uint32_t OPERAND_BASE      = 0x00010000u;
constexpr uint32_t RESULT_BASE       = 0x00003000u;
constexpr unsigned DEVICES           = 4;
constexpr unsigned MAX_DEPTH         = 8;
constexpr unsigned RESULT_SLOT_BYTES = 8 * 8 * sizeof(float);

struct Gemm;

struct Tile {
    Gemm        *gemm = nullptr;
    uint32_t     job_id = 0;
    unsigned     tile_id = 0, k_index = 0, k_count = 0;
    int          m = 0, k = 0, n = 0, P = 0;
    float       *C = nullptr;
    int          ldc = 0;
    unsigned     device = 0, start_cycle = 0, est_cycles = 0;
    unsigned     operand_slot = 0, result_slot = 0;
    uint64_t     seq = 0;                     // global submission order
    std::vector<unsigned char> slab;
    std::vector<float>         result;        // P x P once received
};

/* One output tile of a GEMM: the tiles sharing a tile_id, accumulated into
 * the same C in k_index order. */
struct OutputTile {
    unsigned k_count = 0;
    unsigned next_k = 0;
    bool     failed = false;                  // a fold failed: the rest are dropped
    std::set<unsigned> submitted;             // k_index values seen
    std::map<unsigned, std::unique_ptr<Tile>> early;   // received ahead of next_k
};

/* What fpga_tile_poll reports for one submitted tile, kept after the tile
 * itself is gone. */
struct TileRecord {
    fpga_tile_state_t state = FPGA_TILE_UNKNOWN;   // set_state counts the first real state
    uint32_t          job_id = 0;
    int               error = 0;
};

struct Gemm {
    unsigned id = 0;
    int      M = 0, N = 0, K = 0;
    unsigned tiles_hint = 0;
    unsigned submitted = 0;
    unsigned outstanding = 0;                 // submitted, not yet accumulated or failed
    unsigned completed = 0, failed = 0;
    unsigned queued = 0, staged = 0, inflight = 0, held = 0;   // tiles in each state
    int      first_error = 0;
    bool     waited = false;
    std::map<unsigned, OutputTile> outputs;
    std::map<std::pair<unsigned, unsigned>, TileRecord> tiles;   // (tile_id, k_index)
    clock_t_::time_point t_begin;
};

struct Device {
    unsigned P = 0, k_max = 0;
    std::deque<std::unique_ptr<Tile>> queue;    // packed, not yet in DDR
    std::deque<std::unique_ptr<Tile>> staged;   // slab in DDR, descriptor not sent (in order)
    std::deque<std::unique_ptr<Tile>> inflight; // descriptor sent, no completion yet (in order)
    uint32_t operand_free = 0;                  // bit s: operand slot s is free
    uint32_t result_free = 0;                   // bit s: result slot s is free
    unsigned max_inflight = 0;                  // most descriptors in flight at once
    bool     credit_blocked = false;            // a staged descriptor waits for a credit
    uint32_t last_freed_job = 0;                // the completion that last freed a credit
};

enum class ReadPolicy { Blocked, Always, Wait };

struct Settings {
    bool       stage = true;
    bool       burst = true;                  // SYSTOLIC_ASYNC_BURST: stage every slab a device has
                                              // room for before its first descriptors go out
    unsigned   depth_env = 0;                 // SYSTOLIC_ASYNC_DEPTH; 0 = the configuration's
    unsigned   queue_limit = 64;
    unsigned   burst_hold_us = 1000;          // SYSTOLIC_ASYNC_BURST_HOLD_US
    ReadPolicy policy = ReadPolicy::Blocked;
    bool       violate = false;
    uint32_t   timeout_ms = 5000;
    uint32_t   poll_us = 50;
    uint32_t   poll_max_us = 1000;            // SYSTOLIC_ASYNC_POLL_MAX_US
    bool       trace = false;
    bool       timing = false;
};

bool env_flag(const char *name) {
    const char *v = std::getenv(name);
    return v && *v && std::strcmp(v, "0") != 0;
}

long env_long(const char *name, long dflt, long lo, long hi) {
    const char *v = std::getenv(name);
    if (!v || !*v)
        return dflt;
    char *end = nullptr;
    const long t = std::strtol(v, &end, 10);
    if (end && !*end && t >= lo && t <= hi)
        return t;
    std::fprintf(stderr, "[fpga-async] %s='%s' ignored: expected %ld..%ld\n",
                 name, v, lo, hi);
    return dflt;
}

Settings read_settings() {
    Settings s;
    s.stage         = env_long("SYSTOLIC_ASYNC_STAGE", 1, 0, 1) == 1;
    s.depth_env     = static_cast<unsigned>(env_long("SYSTOLIC_ASYNC_DEPTH", 0, 0, MAX_DEPTH));
    s.burst         = env_long("SYSTOLIC_ASYNC_BURST", 1, 0, 1) == 1;
    s.queue_limit   = static_cast<unsigned>(env_long("SYSTOLIC_ASYNC_QUEUE_LIMIT", 64, 1, 1 << 20));
    s.burst_hold_us = static_cast<unsigned>(env_long("SYSTOLIC_ASYNC_BURST_HOLD_US", 1000, 0, 60000000));
    s.timeout_ms    = static_cast<uint32_t>(env_long("SYSTOLIC_DPTI_TIMEOUT_MS", 5000, 1, 1L << 30));
    s.poll_us       = static_cast<uint32_t>(env_long("SYSTOLIC_ASYNC_POLL_US", 50, 0, 1000000));
    s.poll_max_us   = static_cast<uint32_t>(env_long("SYSTOLIC_ASYNC_POLL_MAX_US", 1000, 0, 1000000));
    if (s.poll_max_us < s.poll_us)
        s.poll_max_us = s.poll_us;
    s.violate       = env_flag("SYSTOLIC_ASYNC_VIOLATE");
    s.trace         = env_flag("SYSTOLIC_RUNTIME_TRACE");
    s.timing        = env_flag("SYSTOLIC_RUNTIME_TIMING");
    if (const char *p = std::getenv("SYSTOLIC_ASYNC_READ_POLICY")) {
        if (!std::strcmp(p, "always"))       s.policy = ReadPolicy::Always;
        else if (!std::strcmp(p, "wait"))    s.policy = ReadPolicy::Wait;
        else if (!std::strcmp(p, "blocked")) s.policy = ReadPolicy::Blocked;
        else if (*p)
            std::fprintf(stderr, "[fpga-async] SYSTOLIC_ASYNC_READ_POLICY='%s' "
                                 "ignored: blocked, always or wait\n", p);
    }
    return s;
}

const char *policy_name(ReadPolicy p) {
    switch (p) {
    case ReadPolicy::Always: return "always";
    case ReadPolicy::Wait:   return "wait";
    default:                 return "blocked";
    }
}

unsigned long long ns_between(clock_t_::time_point a, clock_t_::time_point b) {
    return static_cast<unsigned long long>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(b - a).count());
}

struct Runtime {
    std::mutex              mu;
    std::condition_variable cv_worker;        // the transfer thread: work, a waiter, stop
    std::condition_variable cv_caller;        // callers: a completion, queue room, a failure
    std::thread             worker;
    bool                    worker_running = false;
    bool                    stop = false;

    Settings              set;
    bool                  settings_read = false;
    fpga_runtime_config_t cfg{};
    int                   cfg_rc = 0;
    bool                  cfg_loaded = false;

    DptiFtdiSession      *session = nullptr;
    bool                  link_failed = false;
    int                   link_error = 0;
    int                   last_begin_error = 0;
    bool                  refusal_reported = false;

    unsigned              epoch = 0;          // job_id bits 31..24, per session
    uint32_t              next_seq = 0;       // job_id bits 23..0
    uint64_t              submit_seq = 0;
    unsigned              next_gemm_id = 1;

    Device                dev[DEVICES];
    std::unordered_map<uint32_t, Tile *> pjt; // job_id -> the in-flight tile
    unsigned              queued_total = 0;
    unsigned              inflight_total = 0;
    unsigned              depth = 1;          // descriptors in flight per device
    unsigned              operand_slots = 2;  // depth + 1
    const char           *depth_source = "config";
    uint32_t              poll_backoff_us = 0; // the current pause after an empty read
    unsigned              waiting = 0;        // callers inside a wait: burst mode releases everything
    bool                  hold_expired = false; // burst mode: no submit for burst_hold_us
    clock_t_::time_point  t_last_submit;

    // The evidence of the batch semantics (fpga_async_runtime.h): when the
    // last descriptor went out and when the first completion record came in,
    // both relative to the first submit.
    clock_t_::time_point  t_last_descriptor, t_first_completion;
    bool                  seen_completion = false;
    uint32_t              last_completed_job = 0; // the most recent completion (any device)

    // Receive side (the transfer thread's): bytes off the link not yet
    // consumed, and the record being assembled.
    std::vector<unsigned char> rx;
    size_t                rx_len = 0;
    bool                  rx_in_payload = false;
    uint32_t              rx_payload_job = 0;
    unsigned              rx_payload_bytes = 0;
    clock_t_::time_point  last_activity;      // last byte read or descriptor sent

    fpga_async_stats_t    stats{};
    bool                  span_started = false;
    clock_t_::time_point  t_first_submit, t_last_wait;
    unsigned long long    bytes_written_base = 0, bytes_read_base = 0;

    std::vector<std::unique_ptr<Gemm>> gemms;  // open GEMMs

    ~Runtime();
};

void stop_worker_locked(Runtime &r, std::unique_lock<std::mutex> &lk);

Runtime &rt() {
    static Runtime r;
    if (!r.settings_read) {
        r.set = read_settings();
        r.settings_read = true;
        r.rx.resize(1u << 16);
    }
    return r;
}

/* Every trace line ends in " @<us>", microseconds since the runtime's first
 * use (std::chrono::steady_clock), so the host-side events of a run --
 * submit, stage, descriptor, completion, accumulate, wait -- can be ordered
 * against each other and against the RTL's own cycle stamps. One fputs per
 * line, so the two threads' lines never interleave. */
clock_t_::time_point trace_epoch() {
    static const clock_t_::time_point t0 = clock_t_::now();
    return t0;
}

void trace(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void trace(const char *fmt, ...) {
    if (!rt().set.trace)
        return;
    char line[640];
    int n = std::snprintf(line, sizeof line, "[fpga-async] ");
    va_list ap;
    va_start(ap, fmt);
    n += std::vsnprintf(line + n, sizeof line - static_cast<size_t>(n), fmt, ap);
    va_end(ap);
    if (n < 0 || static_cast<size_t>(n) >= sizeof line - 32)
        n = static_cast<int>(sizeof line) - 33;
    std::snprintf(line + n, sizeof line - static_cast<size_t>(n), " @%llu\n",
                  static_cast<unsigned long long>(
                      std::chrono::duration_cast<std::chrono::microseconds>(
                          clock_t_::now() - trace_epoch()).count()));
    std::fputs(line, stderr);
}

/* How many descriptors to keep in flight per device: SYSTOLIC_ASYNC_DEPTH if
 * set, else the configuration's PIPELINE_DEPTH (1 when not recorded: the
 * archived bitstream takes a context's next job only after the previous
 * result is written back), never above that depth -- a deeper host would
 * park a descriptor in the bridge until the context's queue has room, and
 * every other device's descriptors behind it -- and never above MAX_DEPTH.
 * When the configuration records the DDR size (DDR_BYTES, the
 * co-simulation's model), the depth is lowered until the operand slots fit;
 * 0, or -16 when even one descriptor per device does not. */
int resolve_depth(Runtime &r) {
    const unsigned hw = r.cfg.pipeline_depth < 1 ? 1u : r.cfg.pipeline_depth;
    const unsigned cap = hw < MAX_DEPTH ? hw : MAX_DEPTH;
    unsigned want = r.set.depth_env ? r.set.depth_env : hw;
    r.depth_source = r.set.depth_env ? "SYSTOLIC_ASYNC_DEPTH" : "PIPELINE_DEPTH";
    if (want > cap) {
        std::fprintf(stderr,
                     "[fpga-async] SYSTOLIC_ASYNC_DEPTH=%u clamped to %u: the "
                     "bitstream's PIPELINE_DEPTH is %u (at most %u)\n",
                     want, cap, hw, MAX_DEPTH);
        want = cap;
    }
    if (want < 1)
        want = 1;
    r.depth = want;
    r.operand_slots = r.depth + 1;
    if (r.cfg.ddr_bytes) {
        auto operand_end = [&](unsigned depth) {
            return static_cast<unsigned long long>(OPERAND_BASE) +
                   static_cast<unsigned long long>(DEVICES) * (depth + 1) * r.cfg.slab_bytes;
        };
        while (r.depth > 1 && operand_end(r.depth) > r.cfg.ddr_bytes)
            --r.depth;
        if (operand_end(r.depth) > r.cfg.ddr_bytes) {
            std::fprintf(stderr,
                         "[fpga-async] %s: DDR_BYTES=%u holds no room for one %u-byte "
                         "operand slab per device above 0x%x\n",
                         r.cfg.source, r.cfg.ddr_bytes, r.cfg.slab_bytes, OPERAND_BASE);
            return -16;
        }
        if (r.depth != want) {
            std::fprintf(stderr,
                         "[fpga-async] depth %u lowered to %u: DDR_BYTES=%u holds %u "
                         "operand slabs of %u bytes per device\n",
                         want, r.depth, r.cfg.ddr_bytes, r.depth + 1, r.cfg.slab_bytes);
            r.depth_source = "DDR_BYTES";
        }
        r.operand_slots = r.depth + 1;
    }
    return 0;
}

/* The configuration, loaded once (the same file the scheduled entry point
 * reads). 0, -16 invalid, -19 no completion record. */
int ensure_config(Runtime &r) {
    if (!r.cfg_loaded) {
        r.cfg_rc = fpga_runtime_get_config(&r.cfg);
        r.cfg_loaded = true;
        if (r.cfg_rc == 0 && !r.cfg.completion_header)
            r.cfg_rc = -19;
        if (r.cfg_rc == -16)
            std::fprintf(stderr, "[fpga-async] %s\n", r.cfg.error);
        else if (r.cfg_rc == -19)
            std::fprintf(stderr,
                         "[fpga-async] %s: COMPLETION_HEADER is not 1; the "
                         "asynchronous runtime attributes results by the "
                         "completion record and cannot drive this bitstream\n",
                         r.cfg.source);
        for (unsigned d = 0; d < DEVICES && r.cfg_rc == 0; ++d) {
            r.dev[d].P = fpga_runtime_device_geometry(d);
            r.dev[d].k_max = fpga_runtime_device_k_max(&r.cfg, d);
        }
        if (r.cfg_rc == 0)
            r.cfg_rc = resolve_depth(r);
    }
    return r.cfg_rc;
}

uint32_t operand_addr(const Runtime &r, unsigned device, unsigned slot) {
    return OPERAND_BASE + (device * r.operand_slots + slot) * r.cfg.slab_bytes;
}

uint32_t result_addr(const Runtime &r, unsigned device, unsigned slot) {
    return RESULT_BASE + (device * r.depth + slot) * RESULT_SLOT_BYTES;
}

/* The lowest set bit of a free mask: the slot to take. */
unsigned lowest_free(uint32_t mask) {
    unsigned s = 0;
    while (!(mask & (1u << s)))
        ++s;
    return s;
}

/* The transfer thread's: the session's byte counters into the statistics
 * (called after each transport call, under the lock). */
void account_bytes(Runtime &r) {
    if (!r.session)
        return;
    const unsigned long long w = dpti_ftdi_bytes_written(r.session);
    const unsigned long long rd = dpti_ftdi_bytes_read(r.session);
    r.stats.bytes_to_fpga   += w - r.bytes_written_base;
    r.stats.bytes_from_fpga += rd - r.bytes_read_base;
    r.bytes_written_base = w;
    r.bytes_read_base = rd;
}

/* ---- per-tile state, as fpga_gemm_poll / fpga_tile_poll see it ---------- */

unsigned *state_counter(Gemm &g, fpga_tile_state_t s) {
    switch (s) {
    case FPGA_TILE_QUEUED:    return &g.queued;
    case FPGA_TILE_STAGED:    return &g.staged;
    case FPGA_TILE_IN_FLIGHT: return &g.inflight;
    case FPGA_TILE_HELD:      return &g.held;
    case FPGA_TILE_DONE:      return &g.completed;
    case FPGA_TILE_FAILED:    return &g.failed;
    default:                  return nullptr;
    }
}

void set_state(Tile &t, fpga_tile_state_t s, int error = 0) {
    Gemm &g = *t.gemm;
    TileRecord &rec = g.tiles[{t.tile_id, t.k_index}];
    if (unsigned *old = state_counter(g, rec.state))
        if (*old > 0)
            --*old;
    rec.state = s;
    rec.job_id = t.job_id;
    rec.error = error;
    if (unsigned *now = state_counter(g, s))
        ++*now;
}

/* A tile is over: accumulated, or failed with `error`. */
void retire(Runtime &r, Gemm &g, int error) {
    if (g.outstanding > 0)
        --g.outstanding;
    if (error) {
        ++r.stats.tiles_failed;
        if (g.first_error == 0)
            g.first_error = error;
        if (r.stats.first_error == 0)
            r.stats.first_error = error;
    }
}

void accumulate(Runtime &r, Tile &t) {
    const clock_t_::time_point t0 = clock_t_::now();
    const int P = t.P;
    for (int i = 0; i < t.m; ++i)
        for (int j = 0; j < t.n; ++j)
            t.C[static_cast<size_t>(i) * t.ldc + j] += t.result[static_cast<size_t>(i) * P + j];
    r.stats.ns_collect += ns_between(t0, clock_t_::now());
    ++r.stats.tiles_completed;
    ++r.stats.tiles_per_device[t.device];
    set_state(t, FPGA_TILE_DONE);
    trace("accumulate job=0x%08x gemm=%u tile=%u k=%u/%u device=%u", t.job_id,
          t.gemm->id, t.tile_id, t.k_index, t.k_count, t.device);
    retire(r, *t.gemm, 0);
}

/* A result arrived: add it now if it is the next fold of its output tile,
 * else keep it until the folds before it are in. */
void deliver(Runtime &r, std::unique_ptr<Tile> tile) {
    Gemm &g = *tile->gemm;
    OutputTile &ot = g.outputs[tile->tile_id];
    if (ot.failed) {
        trace("drop job=0x%08x: output tile %u of gemm %u already failed",
              tile->job_id, tile->tile_id, g.id);
        set_state(*tile, FPGA_TILE_FAILED, -14);
        retire(r, g, -14);
        return;
    }
    if (tile->k_index != ot.next_k) {
        trace("hold job=0x%08x: fold %u of output tile %u, waiting for fold %u",
              tile->job_id, tile->k_index, tile->tile_id, ot.next_k);
        set_state(*tile, FPGA_TILE_HELD);
        ot.early[tile->k_index] = std::move(tile);
        return;
    }
    accumulate(r, *tile);
    ++ot.next_k;
    tile.reset();
    for (auto it = ot.early.find(ot.next_k); it != ot.early.end();
         it = ot.early.find(ot.next_k)) {
        std::unique_ptr<Tile> next = std::move(it->second);
        ot.early.erase(it);
        accumulate(r, *next);
        ++ot.next_k;
    }
}

void fail_tile(Runtime &r, std::unique_ptr<Tile> tile, int error) {
    Gemm &g = *tile->gemm;
    OutputTile &ot = g.outputs[tile->tile_id];
    ot.failed = true;
    // Folds already received but held behind this one can no longer be
    // accumulated in order: they fail with it.
    for (auto &kv : ot.early) {
        set_state(*kv.second, FPGA_TILE_FAILED, error);
        retire(r, g, error);
    }
    ot.early.clear();
    set_state(*tile, FPGA_TILE_FAILED, error);
    retire(r, g, error);
}

/* The link is unusable: every tile anywhere fails with `error`, nothing
 * more is sent until fpga_async_runtime_close(). Under the lock, on the
 * transfer thread (or at close). */
void fail_link(Runtime &r, int error) {
    if (r.link_failed)
        return;
    r.link_failed = true;
    r.link_error = error;
    std::fprintf(stderr,
                 "[fpga-async] the DPTI link failed (rc=%d); every tile in "
                 "flight fails and nothing more is sent until "
                 "fpga_async_runtime_close()\n",
                 error);
    for (unsigned d = 0; d < DEVICES; ++d) {
        Device &dv = r.dev[d];
        while (!dv.queue.empty()) {
            std::unique_ptr<Tile> t = std::move(dv.queue.front());
            dv.queue.pop_front();
            fail_tile(r, std::move(t), error);
        }
        while (!dv.staged.empty()) {
            std::unique_ptr<Tile> t = std::move(dv.staged.front());
            dv.staged.pop_front();
            fail_tile(r, std::move(t), error);
        }
        while (!dv.inflight.empty()) {
            std::unique_ptr<Tile> t = std::move(dv.inflight.front());
            dv.inflight.pop_front();
            fail_tile(r, std::move(t), error);
        }
        dv.credit_blocked = false;
    }
    // Held folds whose output tile has not failed through a tile above
    // (every held tile belongs to a tile that was in flight, so none is
    // left; kept as a guard).
    for (auto &g : r.gemms)
        for (auto &kv : g->outputs) {
            for (auto &held : kv.second.early) {
                set_state(*held.second, FPGA_TILE_FAILED, error);
                retire(r, *g, error);
            }
            kv.second.early.clear();
        }
    r.pjt.clear();
    r.queued_total = 0;
    r.inflight_total = 0;
    r.rx_len = 0;
    r.rx_in_payload = false;
    r.cv_caller.notify_all();
}

/* ---- the write side (the transfer thread) ------------------------------- */

bool can_send(const Runtime &r, const Device &dv) {
    return !dv.staged.empty() && dv.inflight.size() < r.depth && dv.result_free != 0;
}

/* Slabs are staged ahead of their descriptors: one per device, or, in
 * burst mode, as many as the device has operand slots for (depth + 1).
 * With SYSTOLIC_ASYNC_STAGE=0 only while nothing of that device is in
 * flight, which also keeps at most one descriptor in flight there. */
bool can_stage(const Runtime &r, const Device &dv) {
    const size_t ahead = (r.set.burst && r.set.stage) ? r.operand_slots : 1;
    if (dv.queue.empty() || dv.staged.size() >= ahead || dv.operand_free == 0)
        return false;
    return r.set.stage || dv.inflight.empty();
}

/* Burst mode (the default, with staging on): a device's first descriptors
 * are held back until every operand slot of the device holds a slab
 * (depth + 1 of them), until a caller waits on a GEMM, or until no tile has
 * been submitted for burst_hold_us; from then on each staged descriptor
 * goes as soon as a credit is free. So the context receives the first ones
 * back to back and runs the tiles through its pipeline (the fill of T+1 at
 * the fold of T, the fold of T+1 while T is still being written back and
 * read back, and while T reduces where the fill is short enough:
 * docs/COSTMODEL.md section 8), instead of one descriptor per slab transfer
 * with the array idle in between (a slab crosses the DPTI link in several
 * times a tile's compute). The link carries the same bytes either way; a
 * device's first result comes later by the slabs staged ahead of it.
 * SYSTOLIC_ASYNC_BURST=0 sends every descriptor as soon as its slab is in
 * DDR. (The in-flight jobs hold their operand slots until they complete, so
 * after the first burst the device can never again have every slot staged:
 * holding until then would leave one job in flight at a time.) */
bool may_release(const Runtime &r, const Device &dv) {
    return !r.set.burst || !r.set.stage || r.waiting > 0 || r.hold_expired ||
           !dv.inflight.empty() || dv.staged.size() >= r.operand_slots;
}

/* Whether some device holds a staged descriptor only because the burst is
 * not full yet (what the hold timer is for). */
bool burst_held(const Runtime &r) {
    for (unsigned d = 0; d < DEVICES; ++d)
        if (!r.dev[d].staged.empty() && !may_release(r, r.dev[d]))
            return true;
    return false;
}

void note_credit_backpressure(Runtime &r, Device &dv, unsigned d) {
    // A released descriptor that cannot go: the device's hardware queue is
    // full (every credit in flight) or every result slot is taken. It waits
    // for capacity -- the next completion of this device -- not for any
    // particular job.
    if (!dv.staged.empty() && may_release(r, dv) && !dv.credit_blocked &&
        (dv.inflight.size() >= r.depth || dv.result_free == 0)) {
        dv.credit_blocked = true;
        trace("backpressure device=%u: job=0x%08x staged, %zu/%u descriptor(s) in "
              "flight, result slots free=0x%x: waiting for a credit",
              d, dv.staged.front()->job_id, dv.inflight.size(), r.depth,
              static_cast<unsigned>(dv.result_free));
    }
}

/* Send the front staged descriptor of device d. The transport call runs
 * without the lock; true when it went out. */
bool send_descriptor(Runtime &r, std::unique_lock<std::mutex> &lk, unsigned d) {
    Device &dv = r.dev[d];
    std::unique_ptr<Tile> t = std::move(dv.staged.front());
    dv.staged.pop_front();
    dpti_job_t job = {};
    job.job_id      = t->job_id;
    job.device_id   = d;
    job.m           = static_cast<uint32_t>(t->P);
    job.n           = static_cast<uint32_t>(t->P);
    job.k           = static_cast<uint32_t>(t->k);
    job.start_cycle = t->start_cycle;
    job.est_cycles  = t->est_cycles;
    job.a_base      = operand_addr(r, d, t->operand_slot);
    job.b_base      = 0;
    t->result_slot  = lowest_free(dv.result_free);
    dv.result_free &= ~(1u << t->result_slot);
    job.c_base      = result_addr(r, d, t->result_slot);
    DptiFtdiSession *session = r.session;

    lk.unlock();
    const clock_t_::time_point t0 = clock_t_::now();
    const int rc = dpti_ftdi_submit_session(session, &job);
    const clock_t_::time_point t1 = clock_t_::now();
    lk.lock();

    r.stats.ns_link_write += ns_between(t0, t1);
    ++r.stats.descriptors;
    account_bytes(r);
    if (rc != 0) {
        dv.result_free |= 1u << t->result_slot;
        dv.staged.push_front(std::move(t));
        fail_link(r, rc == -6 ? -6 : -5);
        return false;
    }
    r.pjt[t->job_id] = t.get();
    if (!r.seen_completion)
        ++r.stats.descriptors_before_first_completion;
    const size_t older = dv.inflight.size();
    if (older > 0)
        ++r.stats.descriptors_behind_inflight;
    if (dv.credit_blocked) {
        // This descriptor waited for a credit: the oldest job's completion
        // freed it while the device's previous descriptor(s) stayed in
        // flight.
        ++r.stats.credit_waits;
        dv.credit_blocked = false;
        trace("credit device=%u job=0x%08x sent on the credit freed by job=0x%08x "
              "(%zu older descriptor(s) still in flight)",
              d, t->job_id, dv.last_freed_job, older);
    }
    set_state(*t, FPGA_TILE_IN_FLIGHT);
    dv.inflight.push_back(std::move(t));
    ++r.inflight_total;
    if (dv.inflight.size() > dv.max_inflight)
        dv.max_inflight = static_cast<unsigned>(dv.inflight.size());
    r.last_activity = t1;
    r.t_last_descriptor = t1;
    r.poll_backoff_us = r.set.poll_us;
    trace("descriptor job=0x%08x device=%u k=%d a=0x%05x c=0x%05x inflight=%zu/%u",
          dv.inflight.back()->job_id, d, dv.inflight.back()->k,
          static_cast<unsigned>(job.a_base), static_cast<unsigned>(job.c_base),
          dv.inflight.size(), r.depth);
    return true;
}

/* Stage the front queued slab of device d into DDR, without the lock
 * during the transfer; true when it is there. */
bool stage_slab(Runtime &r, std::unique_lock<std::mutex> &lk, unsigned d) {
    Device &dv = r.dev[d];
    std::unique_ptr<Tile> t = std::move(dv.queue.front());
    dv.queue.pop_front();
    --r.queued_total;
    t->operand_slot = lowest_free(dv.operand_free);
    dv.operand_free &= ~(1u << t->operand_slot);
    const uint32_t addr = operand_addr(r, d, t->operand_slot);
    DptiFtdiSession *session = r.session;
    const bool behind = !dv.inflight.empty();

    lk.unlock();
    const clock_t_::time_point t0 = clock_t_::now();
    const int rc = dpti_ftdi_mem_write_session(
        session, addr, t->slab.data(), static_cast<uint32_t>(t->slab.size()));
    const clock_t_::time_point t1 = clock_t_::now();
    lk.lock();

    r.stats.ns_link_write += ns_between(t0, t1);
    ++r.stats.mem_writes;
    account_bytes(r);
    if (rc != 0) {
        dv.operand_free |= 1u << t->operand_slot;
        dv.queue.push_front(std::move(t));
        ++r.queued_total;
        fail_link(r, -4);
        return false;
    }
    trace("stage job=0x%08x device=%u slot=%u addr=0x%05x%s", t->job_id, d,
          t->operand_slot, addr, behind ? " (behind a running job)" : "");
    set_state(*t, FPGA_TILE_STAGED);
    dv.staged.push_back(std::move(t));
    // Room in the host queue: a blocked submit may go on.
    r.cv_caller.notify_all();
    return true;
}

/* The oldest queued slab that may be staged, or -1. */
int stageable_device(const Runtime &r) {
    int best = -1;
    for (unsigned d = 0; d < DEVICES; ++d) {
        const Device &dv = r.dev[d];
        if (can_stage(r, dv) &&
            (best < 0 || dv.queue.front()->seq < r.dev[best].queue.front()->seq))
            best = static_cast<int>(d);
    }
    return best;
}

/* The first device with a descriptor that may go, or -1. */
int sendable_device(Runtime &r) {
    for (unsigned d = 0; d < DEVICES; ++d) {
        Device &dv = r.dev[d];
        if (may_release(r, dv) && can_send(r, dv))
            return static_cast<int>(d);
        note_credit_backpressure(r, dv, d);
    }
    return -1;
}

/* One write action -- a descriptor that may go, or the oldest queued slab
 * that may be staged -- or none. While nobody waits, a freed credit is
 * used first (the context's queue stays topped up) and slabs are staged
 * after; once a caller waits (or the burst hold has expired), every slab
 * that fits in DDR is staged before the descriptors go, so the burst a
 * device receives is as full as its slots allow -- the "slabs first" order
 * the synchronous submits of the single-threaded runtime had. */
bool write_step(Runtime &r, std::unique_lock<std::mutex> &lk) {
    if (r.link_failed || r.stop)
        return false;
    const bool slabs_first = r.waiting > 0 || r.hold_expired;
    if (slabs_first) {
        const int d = stageable_device(r);
        if (d >= 0)
            return stage_slab(r, lk, static_cast<unsigned>(d));
    }
    const int s = sendable_device(r);
    if (s >= 0)
        return send_descriptor(r, lk, static_cast<unsigned>(s));
    if (!slabs_first) {
        const int d = stageable_device(r);
        if (d >= 0)
            return stage_slab(r, lk, static_cast<unsigned>(d));
    }
    return false;
}

void note_pass(Runtime &r) {
    ++r.stats.passes;
    unsigned inflight = 0, behind = 0, deep = 0;
    for (unsigned d = 0; d < DEVICES; ++d) {
        if (!r.dev[d].inflight.empty()) ++inflight;
        if (!r.dev[d].inflight.empty() && !r.dev[d].staged.empty()) ++behind;
        if (r.dev[d].inflight.size() >= 2) ++deep;
    }
    if (inflight >= 2) ++r.stats.passes_2_inflight;
    if (inflight == DEVICES) ++r.stats.passes_4_inflight;
    if (behind) ++r.stats.passes_staged_behind;
    if (deep) ++r.stats.passes_depth2;
}

/* ---- the read side (the transfer thread) -------------------------------- */

uint32_t le32(const unsigned char *p) {
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

/* The in-flight tile `t` (a PJT hit) leaves the link: out of its device's
 * in-flight list, its DDR slots free again (the context read the slab
 * before folding the tile; the result tile left DDR before the record's
 * payload reached the host, or never existed for a refusal). */
std::unique_ptr<Tile> take_inflight(Runtime &r, Tile *t) {
    Device &dv = r.dev[t->device];
    std::unique_ptr<Tile> owned;
    for (auto it = dv.inflight.begin(); it != dv.inflight.end(); ++it) {
        if (it->get() == t) {
            owned = std::move(*it);
            dv.inflight.erase(it);
            break;
        }
    }
    if (!owned)
        return owned;                             // cannot happen: the PJT holds only in-flight tiles
    --r.inflight_total;
    dv.operand_free |= 1u << owned->operand_slot;
    dv.result_free  |= 1u << owned->result_slot;
    dv.last_freed_job = owned->job_id;
    r.last_completed_job = owned->job_id;
    if (!r.seen_completion) {
        r.seen_completion = true;
        r.t_first_completion = clock_t_::now();
    }
    return owned;
}

void complete(Runtime &r, uint32_t job_id, const unsigned char *payload) {
    auto it = r.pjt.find(job_id);
    if (it == r.pjt.end())
        return;
    Tile *t = it->second;
    r.pjt.erase(it);
    std::unique_ptr<Tile> owned = take_inflight(r, t);
    if (!owned)
        return;
    owned->result.resize(static_cast<size_t>(owned->P) * owned->P);
    for (size_t i = 0; i < owned->result.size(); ++i) {
        const uint32_t bits = le32(payload + i * 4);
        std::memcpy(&owned->result[i], &bits, sizeof(float));
    }
    deliver(r, std::move(owned));
}

/* Parse complete records out of the receive buffer. */
void parse_rx(Runtime &r) {
    size_t off = 0;
    for (;;) {
        if (r.rx_in_payload) {
            if (r.rx_len - off < r.rx_payload_bytes)
                break;
            complete(r, r.rx_payload_job, r.rx.data() + off);
            off += r.rx_payload_bytes;
            r.rx_in_payload = false;
            continue;
        }
        if (r.rx_len - off < FPGA_COMPLETION_BYTES)
            break;
        const unsigned char *h = r.rx.data() + off;
        const uint32_t magic = le32(h);
        if (magic != FPGA_COMPLETION_MAGIC) {
            std::fprintf(stderr,
                         "[fpga-async] completion record magic 0x%08x (expected "
                         "0x%08x): the host is out of step with the link\n",
                         magic, FPGA_COMPLETION_MAGIC);
            fail_link(r, -9);
            return;
        }
        const uint32_t job_id  = le32(h + 4);
        const unsigned device  = h[8];
        const unsigned status  = h[9];
        const unsigned payload = static_cast<unsigned>(h[10]) | (static_cast<unsigned>(h[11]) << 8);
        const uint32_t seq     = le32(h + 12);
        off += FPGA_COMPLETION_BYTES;

        auto it = r.pjt.find(job_id);
        Tile *t = it == r.pjt.end() ? nullptr : it->second;
        trace("completion job=0x%08x device=%u status=%u payload=%u seq=%u%s", job_id,
              device, status, payload, seq, t ? "" : " (not in flight: discarded)");

        if (status != FPGA_COMPLETION_OK) {
            // A refusal carries no payload; the tile ran nowhere.
            if (t) {
                std::fprintf(stderr,
                             "[fpga-async] job 0x%08x on device %u refused by the "
                             "bitstream (status %u: %s)\n",
                             job_id, device, status,
                             status == FPGA_COMPLETION_BAD_K ? "k outside 1..K_MAX" :
                             status == FPGA_COMPLETION_BAD_DEV ? "no such device" : "?");
                r.pjt.erase(it);
                std::unique_ptr<Tile> owned = take_inflight(r, t);
                if (owned)
                    fail_tile(r, std::move(owned), -18);
            }
            if (payload != 0) {
                // Not something this RTL sends; stay in step by skipping it.
                r.rx_in_payload = true;
                r.rx_payload_job = 0xffffffffu;
                r.rx_payload_bytes = payload;
            }
            continue;
        }
        if (t) {
            const unsigned want = static_cast<unsigned>(t->P * t->P * sizeof(float));
            if (device != t->device || payload != want) {
                std::fprintf(stderr,
                             "[fpga-async] completion job=0x%08x names device %u, "
                             "%u payload bytes; the tile was submitted to device %u "
                             "(%u bytes expected): the host is out of step with the link\n",
                             job_id, device, payload, t->device, want);
                fail_link(r, -9);
                return;
            }
        }
        r.rx_in_payload = true;
        r.rx_payload_job = job_id;
        r.rx_payload_bytes = payload;
    }
    if (off > 0) {
        std::memmove(r.rx.data(), r.rx.data() + off, r.rx_len - off);
        r.rx_len -= off;
    }
}

/* One read off the link, without the lock during the call; true if any
 * byte came. */
bool read_step(Runtime &r, std::unique_lock<std::mutex> &lk) {
    if (r.link_failed || !r.session)
        return false;
    if (r.rx_len == r.rx.size())
        r.rx.resize(r.rx.size() * 2);
    DptiFtdiSession *session = r.session;
    unsigned char *at = r.rx.data() + r.rx_len;
    const uint32_t room = static_cast<uint32_t>(r.rx.size() - r.rx_len);

    lk.unlock();
    const clock_t_::time_point t0 = clock_t_::now();
    const int n = dpti_ftdi_read_some_session(session, at, room);
    const clock_t_::time_point t1 = clock_t_::now();
    lk.lock();

    r.stats.ns_link_read += ns_between(t0, t1);
    ++r.stats.reads;
    account_bytes(r);
    if (r.stop || r.link_failed)
        return false;
    if (n < 0) {
        fail_link(r, -7);
        return false;
    }
    if (n == 0) {
        ++r.stats.empty_reads;
        return false;
    }
    r.rx_len += static_cast<size_t>(n);
    r.last_activity = t1;
    r.poll_backoff_us = r.set.poll_us;
    parse_rx(r);
    r.cv_caller.notify_all();
    return true;
}

bool read_allowed(const Runtime &r) {
    switch (r.set.policy) {
    case ReadPolicy::Wait:
        return r.waiting > 0 || r.queued_total >= r.set.queue_limit;
    default:
        return true;
    }
}

/* The transfer thread. */
void worker_main(Runtime *rp) {
    Runtime &r = *rp;
    std::unique_lock<std::mutex> lk(r.mu);
    while (!r.stop) {
        if (r.link_failed) {
            r.cv_worker.wait(lk, [&] { return r.stop; });
            continue;
        }
        // Burst hold: no submit for burst_hold_us releases held descriptors.
        if (!r.hold_expired && r.set.burst && r.set.stage && burst_held(r) &&
            ns_between(r.t_last_submit, clock_t_::now()) >=
                static_cast<unsigned long long>(r.set.burst_hold_us) * 1000ull) {
            r.hold_expired = true;
            trace("burst hold expired: releasing staged descriptors");
        }
        if (write_step(r, lk)) {
            if (r.set.policy == ReadPolicy::Always && r.inflight_total > 0 && !r.stop)
                read_step(r, lk);
            continue;
        }
        if (r.stop)
            break;
        note_pass(r);
        if (r.inflight_total > 0 && read_allowed(r)) {
            if (read_step(r, lk))
                continue;
            if (r.stop || r.link_failed)
                continue;
            if (ns_between(r.last_activity, clock_t_::now()) >
                static_cast<unsigned long long>(r.set.timeout_ms) * 1000000ull) {
                std::fprintf(stderr,
                             "[fpga-async] no byte from the link for %u ms with %u "
                             "descriptor(s) in flight\n",
                             r.set.timeout_ms, r.inflight_total);
                fail_link(r, -8);
                continue;
            }
            // Nothing pending: pause (the FT2232H paces an empty read by its
            // latency timer; the co-simulation answers at once, and every
            // request costs it simulation time, so the pause doubles with
            // each empty read up to poll_max_us), waking early for a
            // submit, a waiter or a stop.
            r.cv_worker.wait_for(lk, std::chrono::microseconds(r.poll_backoff_us ? r.poll_backoff_us : 1));
            if (r.poll_backoff_us < r.set.poll_max_us)
                r.poll_backoff_us = std::min(r.set.poll_max_us,
                                             r.poll_backoff_us ? 2 * r.poll_backoff_us : 1u);
            continue;
        }
        // Nothing to write and nothing to read: sleep until there is, or
        // until the burst hold expires.
        if (r.set.burst && r.set.stage && !r.hold_expired && burst_held(r)) {
            const auto deadline = r.t_last_submit +
                                  std::chrono::microseconds(r.set.burst_hold_us);
            r.cv_worker.wait_until(lk, deadline);
        } else {
            r.cv_worker.wait(lk);
        }
    }
}

/* Start the transfer thread (under the lock, after the session opened). */
void start_worker(Runtime &r) {
    if (r.worker_running)
        return;
    r.stop = false;
    r.worker = std::thread(worker_main, &r);
    r.worker_running = true;
}

/* Stop and join the transfer thread; lk is held on entry and on return. */
void stop_worker_locked(Runtime &r, std::unique_lock<std::mutex> &lk) {
    if (!r.worker_running)
        return;
    r.stop = true;
    r.cv_worker.notify_all();
    lk.unlock();
    r.worker.join();
    lk.lock();
    r.worker_running = false;
    r.stop = false;
}

bool open_session(Runtime &r) {
    if (r.session)
        return true;
    r.session = dpti_ftdi_open();
    if (!r.session) {
        r.link_failed = true;
        r.link_error = -3;
        return false;
    }
    r.epoch = (r.epoch % 255u) + 1u;
    r.next_seq = 0;
    r.bytes_written_base = dpti_ftdi_bytes_written(r.session);
    r.bytes_read_base = dpti_ftdi_bytes_read(r.session);
    r.last_activity = clock_t_::now();
    r.t_last_submit = r.last_activity;
    r.seen_completion = false;
    r.hold_expired = false;
    r.waiting = 0;
    for (unsigned d = 0; d < DEVICES; ++d) {
        r.dev[d].operand_free = (1u << r.operand_slots) - 1u;
        r.dev[d].result_free  = (1u << r.depth) - 1u;
        r.dev[d].max_inflight = 0;
        r.dev[d].credit_blocked = false;
        r.dev[d].last_freed_job = 0;
    }
    trace("session opened: epoch %u, depth=%u (%s), stage=%d, burst=%d, queue_limit=%u, read_policy=%s, transfer thread",
          r.epoch, r.depth, r.depth_source, r.set.stage ? 1 : 0, r.set.burst ? 1 : 0,
          r.set.queue_limit, policy_name(r.set.policy));
    start_worker(r);
    return true;
}

Gemm *find_gemm(Runtime &r, fpga_gemm_t *handle) {
    for (auto &g : r.gemms)
        if (reinterpret_cast<fpga_gemm_t *>(g.get()) == handle)
            return g.get();
    return nullptr;
}

void drop_gemm(Runtime &r, Gemm *g) {
    r.gemms.erase(std::remove_if(r.gemms.begin(), r.gemms.end(),
                                 [g](const std::unique_ptr<Gemm> &p) { return p.get() == g; }),
                  r.gemms.end());
}

/* The per-session figures kept outside the counters. */
void refresh_stats(Runtime &r) {
    r.stats.depth = r.depth;
    for (unsigned d = 0; d < DEVICES; ++d)
        if (r.dev[d].max_inflight > r.stats.max_inflight_per_device[d])
            r.stats.max_inflight_per_device[d] = r.dev[d].max_inflight;
    if (r.span_started && r.stats.descriptors > 0)
        r.stats.ns_last_descriptor = ns_between(r.t_first_submit, r.t_last_descriptor);
    if (r.span_started && r.seen_completion)
        r.stats.ns_first_completion = ns_between(r.t_first_submit, r.t_first_completion);
}

void report_timing(Runtime &r) {
    if (!r.set.timing || r.stats.tiles_submitted == 0)
        return;
    refresh_stats(r);
    const fpga_async_stats_t &s = r.stats;
    std::fprintf(stderr,
                 "[fpga-async] timing tiles=%llu completed=%llu failed=%llu gemms=%llu "
                 "span_us=%llu submit_us=%llu wait_us=%llu reads=%llu empty_reads=%llu "
                 "mem_writes=%llu descriptors=%llu bytes_to_fpga=%llu bytes_from_fpga=%llu "
                 "passes=%llu overlap2=%llu overlap4=%llu staged_behind=%llu max_queued=%llu "
                 "stage=%d read_policy=%s queue_limit=%u per_device=%llu/%llu/%llu/%llu "
                 "depth=%u burst=%d depth2_passes=%llu max_inflight=%llu/%llu/%llu/%llu "
                 "last_descriptor_us=%llu first_completion_us=%llu "
                 "before_first_completion=%llu behind_inflight=%llu credit_waits=%llu "
                 "submits_blocked=%llu blocked_us=%llu "
                 "link_write_us=%llu link_read_us=%llu collect_us=%llu polls=%llu "
                 "waits_timed_out=%llu\n",
                 s.tiles_submitted, s.tiles_completed, s.tiles_failed, s.gemms_waited,
                 s.ns_span / 1000, s.ns_submit / 1000, s.ns_wait / 1000, s.reads,
                 s.empty_reads, s.mem_writes, s.descriptors, s.bytes_to_fpga,
                 s.bytes_from_fpga, s.passes, s.passes_2_inflight, s.passes_4_inflight,
                 s.passes_staged_behind, s.max_queued, r.set.stage ? 1 : 0,
                 policy_name(r.set.policy), r.set.queue_limit,
                 s.tiles_per_device[0], s.tiles_per_device[1], s.tiles_per_device[2],
                 s.tiles_per_device[3], s.depth, r.set.burst ? 1 : 0, s.passes_depth2,
                 s.max_inflight_per_device[0], s.max_inflight_per_device[1],
                 s.max_inflight_per_device[2], s.max_inflight_per_device[3],
                 s.ns_last_descriptor / 1000, s.ns_first_completion / 1000,
                 s.descriptors_before_first_completion, s.descriptors_behind_inflight,
                 s.credit_waits, s.submits_blocked, s.ns_submit_blocked / 1000,
                 s.ns_link_write / 1000, s.ns_link_read / 1000, s.ns_collect / 1000,
                 s.polls, s.waits_timed_out);
}

/* Everything of the session, after the transfer thread has stopped. */
void clear_session(Runtime &r) {
    for (unsigned d = 0; d < DEVICES; ++d) {
        r.dev[d].queue.clear();
        r.dev[d].staged.clear();
        r.dev[d].inflight.clear();
        r.dev[d].operand_free = 0;
        r.dev[d].result_free = 0;
        r.dev[d].credit_blocked = false;
    }
    r.pjt.clear();
    r.gemms.clear();
    r.queued_total = 0;
    r.inflight_total = 0;
    r.rx_len = 0;
    r.rx_in_payload = false;
    r.waiting = 0;
    r.hold_expired = false;
    if (r.session) {
        dpti_ftdi_close(r.session);
        r.session = nullptr;
    }
    r.link_failed = false;
    r.link_error = 0;
    r.refusal_reported = false;
    r.span_started = false;
}

Runtime::~Runtime() {
    // A program that never called fpga_async_runtime_close(): stop the
    // thread before the object goes (a running std::thread would terminate
    // the process), and close the link.
    std::unique_lock<std::mutex> lk(mu);
    stop_worker_locked(*this, lk);
    if (session) {
        dpti_ftdi_close(session);
        session = nullptr;
    }
}

/* fpga_gemm_wait_for, with the lock held. */
int wait_locked(Runtime &r, std::unique_lock<std::mutex> &lk, Gemm *g,
                unsigned timeout_ms) {
    const clock_t_::time_point t0 = clock_t_::now();
    g->waited = true;
    // Burst mode releases every staged descriptor while someone waits: the
    // caller wants results, not a fuller burst.
    ++r.waiting;
    r.cv_worker.notify_all();
    auto done = [&] { return g->outstanding == 0 || r.link_failed; };
    if (timeout_ms == FPGA_ASYNC_WAIT_FOREVER)
        r.cv_caller.wait(lk, done);
    else
        r.cv_caller.wait_for(lk, std::chrono::milliseconds(timeout_ms), done);
    --r.waiting;
    int rc;
    if (g->outstanding == 0) {
        rc = g->first_error;
    } else if (r.link_failed) {
        if (g->first_error == 0)
            g->first_error = r.link_error ? r.link_error : -8;
        rc = g->first_error;
    } else {
        rc = -20;
        ++r.stats.waits_timed_out;
    }
    const clock_t_::time_point t1 = clock_t_::now();
    r.stats.ns_wait += ns_between(t0, t1);
    if (rc != -20) {
        ++r.stats.gemms_waited;
        r.t_last_wait = t1;
        if (r.span_started)
            r.stats.ns_span = ns_between(r.t_first_submit, r.t_last_wait);
    }
    trace("gemm %u wait: %u tile(s), %u outstanding, rc=%d, %llu us since begin", g->id,
          g->submitted, g->outstanding, rc, ns_between(g->t_begin, t1) / 1000);
    return rc;
}

} // namespace

/* ---- the API --------------------------------------------------------------- */

fpga_gemm_t *fpga_gemm_begin(int M, int N, int K, unsigned tiles_hint) {
    Runtime &r = rt();
    std::unique_lock<std::mutex> lk(r.mu);
    const int crc = ensure_config(r);
    if (crc != 0) {
        r.last_begin_error = crc;
        return nullptr;
    }
    if (r.link_failed) {
        r.last_begin_error = -14;
        if (!r.refusal_reported) {
            r.refusal_reported = true;
            std::fprintf(stderr,
                         "[fpga-async] the DPTI link failed earlier in this session "
                         "(rc=%d); this and every further GEMM is refused (rc=-14) "
                         "until fpga_async_runtime_close()\n",
                         r.link_error);
        }
        return nullptr;
    }
    if (!open_session(r)) {
        r.last_begin_error = -3;
        return nullptr;
    }
    auto g = std::make_unique<Gemm>();
    g->id = r.next_gemm_id++;
    g->M = M; g->N = N; g->K = K;
    g->tiles_hint = tiles_hint;
    g->t_begin = clock_t_::now();
    ++r.stats.gemms_begun;
    trace("gemm %u begin: %dx%dx%d, %u tile(s) expected", g->id, M, N, K, tiles_hint);
    Gemm *raw = g.get();
    r.gemms.push_back(std::move(g));
    r.last_begin_error = 0;
    return reinterpret_cast<fpga_gemm_t *>(raw);
}

int fpga_async_runtime_last_error(void) {
    Runtime &r = rt();
    std::lock_guard<std::mutex> lk(r.mu);
    return r.last_begin_error;
}

int fpga_tile_submit(fpga_gemm_t *handle,
                     unsigned tile_id, unsigned k_index, unsigned k_count,
                     int m, int k, int n,
                     const float *A, const float *B, float *C, int ldc,
                     unsigned device_id,
                     unsigned start_cycle,
                     unsigned est_cycles) {
    Runtime &r = rt();
    const clock_t_::time_point t0 = clock_t_::now();
    std::unique_lock<std::mutex> lk(r.mu);
    Gemm *g = find_gemm(r, handle);
    if (!g || g->waited)
        return -17;
    if (r.link_failed)
        return -14;
    const unsigned P = device_id < DEVICES ? r.dev[device_id].P : 0;
    if (P == 0)
        return -11;
    const unsigned k_max = r.dev[device_id].k_max;
    if (k < 1 || (static_cast<unsigned>(k) > k_max && !r.set.violate))
        return -10;
    if (m < 1 || n < 1 || static_cast<unsigned>(m) > P || static_cast<unsigned>(n) > P)
        return -11;
    if (!A || !B || !C)
        return -12;
    if (ldc < n)
        return -11;
    if (est_cycles == 0)
        return -15;
    if (k_count == 0 || k_index >= k_count)
        return -17;
    {
        auto it = g->outputs.find(tile_id);
        if (it != g->outputs.end() &&
            (it->second.k_count != k_count || it->second.submitted.count(k_index)))
            return -17;
    }
    const unsigned slab_bytes = r.cfg.slab_bytes;
    const bool violate = r.set.violate;
    lk.unlock();

    // Packing: CPU work on the caller's thread, without the lock.
    auto t = std::make_unique<Tile>();
    t->slab.assign(slab_bytes, 0);
    // SYSTOLIC_ASYNC_VIOLATE: k beyond the device's capacity is packed as
    // far as the slab goes and sent, so the bitstream's refusal is seen.
    const int pack_k = static_cast<unsigned>(k) > k_max ? static_cast<int>(k_max) : k;
    const int prc = fpga_runtime_pack_operand_slab(
        m, pack_k, n, static_cast<int>(P), static_cast<int>(k_max), A, B,
        t->slab.data(), static_cast<unsigned>(t->slab.size()));
    (void)violate;
    if (prc != 0)
        return -13;

    lk.lock();
    // The link may have failed while the slab was packed; the GEMM may have
    // been released by another caller thread.
    g = find_gemm(r, handle);
    if (!g || g->waited)
        return -17;
    if (r.link_failed)
        return -14;
    OutputTile &ot = g->outputs[tile_id];
    if (ot.k_count == 0)
        ot.k_count = k_count;
    else if (ot.k_count != k_count || ot.submitted.count(k_index))
        return -17;
    ot.submitted.insert(k_index);
    t->gemm = g;
    t->job_id = (static_cast<uint32_t>(r.epoch) << 24) | (r.next_seq++ & 0x00ffffffu);
    t->tile_id = tile_id; t->k_index = k_index; t->k_count = k_count;
    t->m = m; t->k = k; t->n = n; t->P = static_cast<int>(P);
    t->C = C; t->ldc = ldc;
    t->device = device_id; t->start_cycle = start_cycle; t->est_cycles = est_cycles;
    t->seq = r.submit_seq++;
    const uint32_t job_id = t->job_id;
    trace("submit job=0x%08x gemm=%u tile=%u k=%u/%u %dx%dx%d device=%u", job_id,
          g->id, tile_id, k_index, k_count, m, k, n, device_id);

    set_state(*t, FPGA_TILE_QUEUED);
    r.dev[device_id].queue.push_back(std::move(t));
    ++r.queued_total;
    ++g->submitted;
    ++g->outstanding;
    ++r.stats.tiles_submitted;
    if (r.queued_total > r.stats.max_queued)
        r.stats.max_queued = r.queued_total;
    if (!r.span_started) {
        r.span_started = true;
        r.t_first_submit = t0;
    }
    r.t_last_submit = clock_t_::now();
    r.hold_expired = false;
    r.cv_worker.notify_all();

    const clock_t_::time_point t_enq = clock_t_::now();
    r.stats.ns_submit += ns_between(t0, t_enq);
    if (r.queued_total >= r.set.queue_limit && !r.link_failed) {
        // The host queue is full: every operand slot of the devices with
        // queued tiles holds a slab and every credit is in use. Wait for
        // room -- the next completion of any device frees an operand slot
        // and the transfer thread stages a queued slab -- never for this or
        // any particular job to finish.
        ++r.stats.submits_blocked;
        trace("backpressure job=0x%08x: host queue full (%u >= limit %u), %u "
              "descriptor(s) in flight: waiting for room",
              job_id, r.queued_total, r.set.queue_limit, r.inflight_total);
        r.cv_caller.wait(lk, [&] {
            return r.queued_total < r.set.queue_limit || r.link_failed || r.stop;
        });
        r.stats.ns_submit_blocked += ns_between(t_enq, clock_t_::now());
        trace("resume job=0x%08x: room after the completion of job=0x%08x, %u "
              "descriptor(s) in flight",
              job_id, r.last_completed_job, r.inflight_total);
    }
    return 0;
}

int fpga_gemm_wait_for(fpga_gemm_t *handle, unsigned timeout_ms) {
    Runtime &r = rt();
    std::unique_lock<std::mutex> lk(r.mu);
    Gemm *g = find_gemm(r, handle);
    if (!g)
        return -17;
    return wait_locked(r, lk, g, timeout_ms);
}

int fpga_gemm_wait(fpga_gemm_t *handle) {
    Runtime &r = rt();
    std::unique_lock<std::mutex> lk(r.mu);
    Gemm *g = find_gemm(r, handle);
    if (!g)
        return -17;
    const int rc = wait_locked(r, lk, g, FPGA_ASYNC_WAIT_FOREVER);
    drop_gemm(r, g);
    return rc;
}

int fpga_gemm_release(fpga_gemm_t *handle) {
    Runtime &r = rt();
    std::lock_guard<std::mutex> lk(r.mu);
    Gemm *g = find_gemm(r, handle);
    if (!g)
        return -17;
    if (g->outstanding > 0 && !r.link_failed)
        return -21;
    trace("gemm %u release: %u tile(s), %u completed, %u failed", g->id, g->submitted,
          g->completed, g->failed);
    drop_gemm(r, g);
    return 0;
}

int fpga_gemm_poll(fpga_gemm_t *handle, fpga_gemm_status_t *out) {
    Runtime &r = rt();
    std::lock_guard<std::mutex> lk(r.mu);
    if (out)
        std::memset(out, 0, sizeof *out);
    ++r.stats.polls;
    Gemm *g = find_gemm(r, handle);
    if (!g)
        return -17;
    if (!out)
        return 0;
    out->submitted = g->submitted;
    out->queued = g->queued;
    out->staged = g->staged;
    out->in_flight = g->inflight;
    out->held = g->held;
    out->completed = g->completed;
    out->failed = g->failed;
    out->first_error = g->first_error;
    out->done = g->outstanding == 0;
    out->link_failed = r.link_failed;
    return 0;
}

int fpga_tile_poll(fpga_gemm_t *handle, unsigned tile_id, unsigned k_index,
                   fpga_tile_status_t *out) {
    Runtime &r = rt();
    std::lock_guard<std::mutex> lk(r.mu);
    if (out)
        std::memset(out, 0, sizeof *out);
    ++r.stats.polls;
    Gemm *g = find_gemm(r, handle);
    if (!g)
        return -17;
    if (!out)
        return 0;
    auto it = g->tiles.find({tile_id, k_index});
    if (it == g->tiles.end()) {
        out->state = FPGA_TILE_UNKNOWN;
        return 0;
    }
    out->state = it->second.state;
    out->job_id = it->second.job_id;
    out->error = it->second.error;
    return 0;
}

void fpga_async_runtime_get_stats(fpga_async_stats_t *out) {
    if (!out)
        return;
    Runtime &r = rt();
    std::lock_guard<std::mutex> lk(r.mu);
    refresh_stats(r);
    *out = r.stats;
}

void fpga_async_runtime_reset_stats(void) {
    Runtime &r = rt();
    std::lock_guard<std::mutex> lk(r.mu);
    r.stats = fpga_async_stats_t{};
    r.span_started = false;
}

void fpga_async_runtime_close(void) {
    Runtime &r = rt();
    std::unique_lock<std::mutex> lk(r.mu);
    stop_worker_locked(r, lk);
    account_bytes(r);
    report_timing(r);
    clear_session(r);
    // Settings (and with them the depth, which the configuration bounds) are
    // read again on the next use, so a test can change them between sessions.
    r.settings_read = false;
    r.cfg_loaded = false;
}
