// fpga_async_runtime.cpp -- many invocations in flight across the
// 1x8x8 + 3x4x4 bitstream over one DPTI session (see fpga_async_runtime.h).
//
// Everything happens on the caller's thread, inside fpga_tile_submit and
// fpga_gemm_wait, in a "pump": write what the credits allow (slabs, then
// descriptors), read what the link has, attribute every completion record
// by its job_id, accumulate results in k_index order. The FT2232H's
// synchronous FIFO carries one direction at a time and the RTL's bus
// arbiter (dpti_bus_arbiter.sv) alternates them, so the host may write
// while a result is on its way back; what the host must not do is read
// in a tight loop, since an empty read costs a latency-timer period on the
// chip -- hence the read policy below.

#include "fpga_async_runtime.h"
#include "dpti_runtime.h"

#include <algorithm>
#include <chrono>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <map>
#include <memory>
#include <set>
#include <thread>
#include <unordered_map>
#include <vector>

namespace {

using clock_t_ = std::chrono::steady_clock;

/* The DDR layout. OPERAND_BASE and RESULT_BASE are the regions the board
 * runs validated (fpga_matmul_tiled.cpp); each device owns SLOTS operand
 * slabs from OPERAND_BASE + device * SLOTS * slab_bytes (slab_bytes is a
 * multiple of 512, so every slot keeps the MEM_WRITE's 256-byte alignment)
 * and SLOTS result tiles of RESULT_SLOT_BYTES from RESULT_BASE. At
 * K_MAX = 16 that is 0x10000..0x12000 and 0x3000..0x3800; the RTL
 * co-simulation's DDR model holds 128 KiB, so K_MAX up to 128 fits there. */
constexpr uint32_t OPERAND_BASE      = 0x00010000u;
constexpr uint32_t RESULT_BASE       = 0x00003000u;
constexpr unsigned DEVICES           = 4;
constexpr unsigned SLOTS             = 2;
constexpr unsigned RESULT_SLOT_BYTES = 8 * 8 * sizeof(float);

enum class TileState { Queued, Staged, InFlight, Done };

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
    TileState    state = TileState::Queued;
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

struct Gemm {
    unsigned id = 0;
    int      M = 0, N = 0, K = 0;
    unsigned tiles_hint = 0;
    unsigned submitted = 0;
    unsigned outstanding = 0;                 // submitted, not yet accumulated or failed
    int      first_error = 0;
    bool     waited = false;
    std::map<unsigned, OutputTile> outputs;
    clock_t_::time_point t_begin;
};

struct Device {
    unsigned P = 0, k_max = 0;
    std::deque<std::unique_ptr<Tile>> queue;  // packed, not yet in DDR
    std::unique_ptr<Tile> staged;             // slab in DDR, descriptor not sent
    std::unique_ptr<Tile> inflight;           // descriptor sent, no completion yet
    unsigned next_operand_slot = 0;
    unsigned next_result_slot = 0;
};

enum class ReadPolicy { Blocked, Always, Wait };

struct Settings {
    bool       stage = true;
    unsigned   queue_limit = 64;
    ReadPolicy policy = ReadPolicy::Blocked;
    bool       violate = false;
    uint32_t   timeout_ms = 5000;
    uint32_t   poll_us = 50;
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
    s.stage       = env_long("SYSTOLIC_ASYNC_STAGE", 1, 0, 1) == 1;
    s.queue_limit = static_cast<unsigned>(env_long("SYSTOLIC_ASYNC_QUEUE_LIMIT", 64, 1, 1 << 20));
    s.timeout_ms  = static_cast<uint32_t>(env_long("SYSTOLIC_DPTI_TIMEOUT_MS", 5000, 1, 1L << 30));
    s.poll_us     = static_cast<uint32_t>(env_long("SYSTOLIC_ASYNC_POLL_US", 50, 0, 1000000));
    s.violate     = env_flag("SYSTOLIC_ASYNC_VIOLATE");
    s.trace       = env_flag("SYSTOLIC_RUNTIME_TRACE");
    s.timing      = env_flag("SYSTOLIC_RUNTIME_TIMING");
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

    // Receive side: bytes off the link not yet consumed, and the record
    // being assembled.
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
};

Runtime &rt() {
    static Runtime r;
    if (!r.settings_read) {
        r.set = read_settings();
        r.settings_read = true;
        r.rx.resize(1u << 16);
    }
    return r;
}

void trace(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void trace(const char *fmt, ...) {
    if (!rt().set.trace)
        return;
    va_list ap;
    va_start(ap, fmt);
    std::fputs("[fpga-async] ", stderr);
    std::vfprintf(stderr, fmt, ap);
    std::fputc('\n', stderr);
    va_end(ap);
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
    }
    return r.cfg_rc;
}

uint32_t operand_addr(const Runtime &r, unsigned device, unsigned slot) {
    return OPERAND_BASE + (device * SLOTS + slot) * r.cfg.slab_bytes;
}

uint32_t result_addr(unsigned device, unsigned slot) {
    return RESULT_BASE + (device * SLOTS + slot) * RESULT_SLOT_BYTES;
}

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
    const int P = t.P;
    for (int i = 0; i < t.m; ++i)
        for (int j = 0; j < t.n; ++j)
            t.C[static_cast<size_t>(i) * t.ldc + j] += t.result[static_cast<size_t>(i) * P + j];
    ++r.stats.tiles_completed;
    ++r.stats.tiles_per_device[t.device];
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
        retire(r, g, -14);
        return;
    }
    if (tile->k_index != ot.next_k) {
        trace("hold job=0x%08x: fold %u of output tile %u, waiting for fold %u",
              tile->job_id, tile->k_index, tile->tile_id, ot.next_k);
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
    const size_t held = ot.early.size();
    ot.early.clear();
    for (size_t i = 0; i < held; ++i)
        retire(r, g, error);
    retire(r, g, error);
}

/* The link is unusable: every tile anywhere fails with `error`, nothing
 * more is sent until fpga_async_runtime_close(). */
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
        if (dv.staged)
            fail_tile(r, std::move(dv.staged), error);
        if (dv.inflight)
            fail_tile(r, std::move(dv.inflight), error);
    }
    r.pjt.clear();
    r.queued_total = 0;
    r.inflight_total = 0;
    r.rx_len = 0;
    r.rx_in_payload = false;
}

/* ---- the write side ------------------------------------------------------ */

bool send_descriptor(Runtime &r, Device &dv, unsigned d) {
    std::unique_ptr<Tile> &t = dv.staged;
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
    t->result_slot  = dv.next_result_slot;
    dv.next_result_slot = (dv.next_result_slot + 1) % SLOTS;
    job.c_base      = result_addr(d, t->result_slot);

    const int rc = dpti_ftdi_submit_session(r.session, &job);
    ++r.stats.descriptors;
    if (rc != 0) {
        fail_link(r, rc == -6 ? -6 : -5);
        return false;
    }
    trace("descriptor job=0x%08x device=%u k=%d a=0x%05x c=0x%05x", t->job_id, d,
          t->k, static_cast<unsigned>(job.a_base), static_cast<unsigned>(job.c_base));
    t->state = TileState::InFlight;
    r.pjt[t->job_id] = t.get();
    dv.inflight = std::move(dv.staged);
    ++r.inflight_total;
    r.last_activity = clock_t_::now();
    return true;
}

bool stage_slab(Runtime &r, Device &dv, unsigned d) {
    std::unique_ptr<Tile> t = std::move(dv.queue.front());
    dv.queue.pop_front();
    --r.queued_total;
    t->operand_slot = dv.next_operand_slot;
    dv.next_operand_slot = (dv.next_operand_slot + 1) % SLOTS;
    const uint32_t addr = operand_addr(r, d, t->operand_slot);
    const int rc = dpti_ftdi_mem_write_session(
        r.session, addr, t->slab.data(), static_cast<uint32_t>(t->slab.size()));
    ++r.stats.mem_writes;
    if (rc != 0) {
        fail_link(r, -4);
        return false;
    }
    trace("stage job=0x%08x device=%u slot=%u addr=0x%05x%s", t->job_id, d,
          t->operand_slot, addr, dv.inflight ? " (behind a running job)" : "");
    t->state = TileState::Staged;
    dv.staged = std::move(t);
    return true;
}

bool can_stage(const Runtime &r, const Device &dv) {
    if (dv.queue.empty() || dv.staged)
        return false;
    return r.set.stage || !dv.inflight;
}

/* Submit every descriptor that may go, stage the oldest queued slab that
 * may go, repeat until nothing moves. */
void pump_writes(Runtime &r) {
    if (r.link_failed)
        return;
    for (;;) {
        bool progress = false;
        for (unsigned d = 0; d < DEVICES; ++d) {
            Device &dv = r.dev[d];
            if (dv.staged && !dv.inflight) {
                if (!send_descriptor(r, dv, d))
                    return;
                progress = true;
            }
        }
        int best = -1;
        for (unsigned d = 0; d < DEVICES; ++d) {
            const Device &dv = r.dev[d];
            if (can_stage(r, dv) &&
                (best < 0 || dv.queue.front()->seq < r.dev[best].queue.front()->seq))
                best = static_cast<int>(d);
        }
        if (best >= 0) {
            if (!stage_slab(r, r.dev[best], static_cast<unsigned>(best)))
                return;
            progress = true;
        }
        if (!progress)
            break;
    }
    ++r.stats.passes;
    unsigned inflight = 0, behind = 0;
    for (unsigned d = 0; d < DEVICES; ++d) {
        if (r.dev[d].inflight) ++inflight;
        if (r.dev[d].inflight && r.dev[d].staged) ++behind;
    }
    if (inflight >= 2) ++r.stats.passes_2_inflight;
    if (inflight == DEVICES) ++r.stats.passes_4_inflight;
    if (behind) ++r.stats.passes_staged_behind;
}

/* ---- the read side ------------------------------------------------------- */

uint32_t le32(const unsigned char *p) {
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

void complete(Runtime &r, uint32_t job_id, const unsigned char *payload) {
    auto it = r.pjt.find(job_id);
    if (it == r.pjt.end())
        return;
    Tile *t = it->second;
    r.pjt.erase(it);
    Device &dv = r.dev[t->device];
    std::unique_ptr<Tile> owned = std::move(dv.inflight);
    --r.inflight_total;
    owned->state = TileState::Done;
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
                Device &dv = r.dev[t->device];
                std::unique_ptr<Tile> owned = std::move(dv.inflight);
                --r.inflight_total;
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

/* One read off the link; true if any byte came. */
bool pump_read(Runtime &r) {
    if (r.link_failed || !r.session)
        return false;
    if (r.rx_len == r.rx.size())
        r.rx.resize(r.rx.size() * 2);
    const int n = dpti_ftdi_read_some_session(
        r.session, r.rx.data() + r.rx_len,
        static_cast<uint32_t>(r.rx.size() - r.rx_len));
    ++r.stats.reads;
    if (n < 0) {
        fail_link(r, -7);
        return false;
    }
    if (n == 0) {
        ++r.stats.empty_reads;
        return false;
    }
    r.rx_len += static_cast<size_t>(n);
    r.last_activity = clock_t_::now();
    parse_rx(r);
    return true;
}

bool anything_pending(const Runtime &r) {
    return r.inflight_total > 0 || r.queued_total > 0 ||
           std::any_of(std::begin(r.dev), std::end(r.dev),
                       [](const Device &d) { return d.staged != nullptr; });
}

/* Read and write until `done()` holds, the link fails, or nothing is in
 * flight any more. */
template <class Pred>
void pump_until(Runtime &r, Pred done) {
    for (;;) {
        pump_writes(r);
        if (r.link_failed || done())
            return;
        if (!anything_pending(r))
            return;
        if (r.inflight_total == 0) {
            // Queued or staged tiles that cannot move: only a completion
            // frees them, and none is coming. (Not reachable with stage
            // credit 1, kept as a guard.)
            fail_link(r, -8);
            return;
        }
        const bool got = pump_read(r);
        if (r.link_failed)
            return;
        if (!got) {
            if (ns_between(r.last_activity, clock_t_::now()) >
                static_cast<unsigned long long>(r.set.timeout_ms) * 1000000ull) {
                std::fprintf(stderr,
                             "[fpga-async] no byte from the link for %u ms with %u "
                             "descriptor(s) in flight\n",
                             r.set.timeout_ms, r.inflight_total);
                fail_link(r, -8);
                return;
            }
            if (r.set.poll_us)
                std::this_thread::sleep_for(std::chrono::microseconds(r.set.poll_us));
        }
    }
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
    trace("session opened: epoch %u, stage=%d, queue_limit=%u, read_policy=%s",
          r.epoch, r.set.stage ? 1 : 0, r.set.queue_limit, policy_name(r.set.policy));
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

void report_timing(Runtime &r) {
    if (!r.set.timing || r.stats.tiles_submitted == 0)
        return;
    account_bytes(r);
    const fpga_async_stats_t &s = r.stats;
    std::fprintf(stderr,
                 "[fpga-async] timing tiles=%llu completed=%llu failed=%llu gemms=%llu "
                 "span_us=%llu submit_us=%llu wait_us=%llu reads=%llu empty_reads=%llu "
                 "mem_writes=%llu descriptors=%llu bytes_to_fpga=%llu bytes_from_fpga=%llu "
                 "passes=%llu overlap2=%llu overlap4=%llu staged_behind=%llu max_queued=%llu "
                 "stage=%d read_policy=%s queue_limit=%u per_device=%llu/%llu/%llu/%llu\n",
                 s.tiles_submitted, s.tiles_completed, s.tiles_failed, s.gemms_waited,
                 s.ns_span / 1000, s.ns_submit / 1000, s.ns_wait / 1000, s.reads,
                 s.empty_reads, s.mem_writes, s.descriptors, s.bytes_to_fpga,
                 s.bytes_from_fpga, s.passes, s.passes_2_inflight, s.passes_4_inflight,
                 s.passes_staged_behind, s.max_queued, r.set.stage ? 1 : 0,
                 policy_name(r.set.policy), r.set.queue_limit,
                 s.tiles_per_device[0], s.tiles_per_device[1], s.tiles_per_device[2],
                 s.tiles_per_device[3]);
}

} // namespace

/* ---- the API --------------------------------------------------------------- */

fpga_gemm_t *fpga_gemm_begin(int M, int N, int K, unsigned tiles_hint) {
    Runtime &r = rt();
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
    return rt().last_begin_error;
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
    OutputTile &ot = g->outputs[tile_id];
    if (ot.k_count == 0)
        ot.k_count = k_count;
    else if (ot.k_count != k_count || ot.submitted.count(k_index))
        return -17;

    auto t = std::make_unique<Tile>();
    t->slab.assign(r.cfg.slab_bytes, 0);
    // SYSTOLIC_ASYNC_VIOLATE: k beyond the device's capacity is packed as
    // far as the slab goes and sent, so the bitstream's refusal is seen.
    const int pack_k = static_cast<unsigned>(k) > k_max ? static_cast<int>(k_max) : k;
    const int prc = fpga_runtime_pack_operand_slab(
        m, pack_k, n, static_cast<int>(P), static_cast<int>(k_max), A, B,
        t->slab.data(), static_cast<unsigned>(t->slab.size()));
    if (prc != 0)
        return -13;

    ot.submitted.insert(k_index);
    t->gemm = g;
    t->job_id = (static_cast<uint32_t>(r.epoch) << 24) | (r.next_seq++ & 0x00ffffffu);
    t->tile_id = tile_id; t->k_index = k_index; t->k_count = k_count;
    t->m = m; t->k = k; t->n = n; t->P = static_cast<int>(P);
    t->C = C; t->ldc = ldc;
    t->device = device_id; t->start_cycle = start_cycle; t->est_cycles = est_cycles;
    t->seq = r.submit_seq++;
    trace("submit job=0x%08x gemm=%u tile=%u k=%u/%u %dx%dx%d device=%u", t->job_id,
          g->id, tile_id, k_index, k_count, m, k, n, device_id);

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

    pump_writes(r);
    if (!r.link_failed) {
        if (r.queued_total >= r.set.queue_limit) {
            pump_until(r, [&] { return r.queued_total < r.set.queue_limit; });
        } else if (r.set.policy == ReadPolicy::Always ||
                   (r.set.policy == ReadPolicy::Blocked && r.queued_total > 0 &&
                    r.inflight_total > 0)) {
            pump_read(r);
            pump_writes(r);
        }
    }
    r.stats.ns_submit += ns_between(t0, clock_t_::now());
    return 0;
}

int fpga_gemm_wait(fpga_gemm_t *handle) {
    Runtime &r = rt();
    const clock_t_::time_point t0 = clock_t_::now();
    Gemm *g = find_gemm(r, handle);
    if (!g)
        return -17;
    g->waited = true;
    pump_until(r, [&] { return g->outstanding == 0; });
    if (g->outstanding > 0 && g->first_error == 0)
        g->first_error = r.link_failed ? r.link_error : -8;
    const int rc = g->first_error;
    ++r.stats.gemms_waited;
    const clock_t_::time_point t1 = clock_t_::now();
    r.stats.ns_wait += ns_between(t0, t1);
    r.t_last_wait = t1;
    if (r.span_started)
        r.stats.ns_span = ns_between(r.t_first_submit, r.t_last_wait);
    account_bytes(r);
    trace("gemm %u wait: %u tile(s), rc=%d, %llu us since begin", g->id, g->submitted,
          rc, ns_between(g->t_begin, t1) / 1000);
    drop_gemm(r, g);
    return rc;
}

void fpga_async_runtime_get_stats(fpga_async_stats_t *out) {
    if (!out)
        return;
    account_bytes(rt());
    *out = rt().stats;
}

void fpga_async_runtime_reset_stats(void) {
    Runtime &r = rt();
    account_bytes(r);
    r.stats = fpga_async_stats_t{};
    r.span_started = false;
}

void fpga_async_runtime_close(void) {
    Runtime &r = rt();
    report_timing(r);
    for (unsigned d = 0; d < DEVICES; ++d) {
        r.dev[d].queue.clear();
        r.dev[d].staged.reset();
        r.dev[d].inflight.reset();
        r.dev[d].next_operand_slot = 0;
        r.dev[d].next_result_slot = 0;
    }
    r.pjt.clear();
    r.gemms.clear();
    r.queued_total = 0;
    r.inflight_total = 0;
    r.rx_len = 0;
    r.rx_in_payload = false;
    if (r.session) {
        dpti_ftdi_close(r.session);
        r.session = nullptr;
    }
    r.link_failed = false;
    r.link_error = 0;
    r.refusal_reported = false;
    r.span_started = false;
    // Settings are read again on the next use, so a test can change them
    // between sessions.
    r.settings_read = false;
}
