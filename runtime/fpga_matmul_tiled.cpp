// fpga_matmul_tiled.cpp -- the scheduled FPGA entry point the compiler calls:
// one invocation of one physical array of the 1x8x8 + 3x4x4 bitstream over
// the DPTI link (dpti_ftdi.cpp).

#include "fpga_matmul_tiled.h"
#include "dpti_runtime.h"

#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

/*
 * Operand slab: the wire format dma_operand_writer decodes.
 *
 * The reduction is carried in windows of 8 steps. Window w is 128 words:
 *
 *     words w*128 +   0 .. w*128 +  63   A: lane (row) r, step koff at
 *                                           r*8 + koff
 *     words w*128 +  64 .. w*128 + 127   B: step koff, lane (col) c at
 *                                           64 + koff*8 + c
 *
 * eight lanes wide whatever the array (a 4x4 reads lanes 0..3). A slab of
 * depth K_MAX is ceil(K_MAX / 8) windows: at K_MAX = 16 the 1024-byte slab
 * of the original bitstream, at K_MAX = 17 three windows (1536 bytes), the
 * last one carrying step 16 and seven steps of padding. Window and offset
 * are k / 8 and k % 8 for ANY k; nothing here assumes a power of two.
 */
static int scheduled_payload_index_a(int k, int row) {
    const int win  = k / 8;
    const int koff = k % 8;

    return
        (win << 7) |
        (row << 3) |
        koff;
}

static int scheduled_payload_index_b(int k, int col) {
    const int win  = k / 8;
    const int koff = k % 8;

    return
        (win  << 7) |
        (1    << 6) |
        (koff << 3) |
        col;
}

static void scheduled_store_f32(
    std::vector<unsigned char>& slab,
    int word_index,
    float value) {

    uint32_t bits = 0;
    memcpy(&bits, &value, sizeof(bits));

    const size_t byte_index =
        static_cast<size_t>(word_index) * 4;

    slab[byte_index + 0] =
        static_cast<unsigned char>(bits);

    slab[byte_index + 1] =
        static_cast<unsigned char>(bits >> 8);

    slab[byte_index + 2] =
        static_cast<unsigned char>(bits >> 16);

    slab[byte_index + 3] =
        static_cast<unsigned char>(bits >> 24);
}

/*
 * Pack one invocation into the operand slab.
 *
 * The layout is the 8-lane image above; a 4x4 array reads lanes 0..3 of it.
 * A full 8x8x16 or 4x4x16 tile at K_MAX = 16 therefore packs to exactly the
 * bytes the board run of 2026-10-09 sent. A smaller tile leaves the rest of
 * the slab zero. Zero rows of A and zero columns of B (m < P, n < P) give
 * zero rows and columns of the P x P product, so its m x n block is the
 * m x k by k x n product. Steps K .. of the slab (k < K_MAX) are storage
 * only: the descriptor carries k = K, so the array reads ceil(K / 8) windows
 * and runs exactly K reduction steps; the zeros in the last window's steps
 * past K are read but never fed.
 */
static int build_scheduled_operand_slab(
    int M,
    int K,
    int N,
    int P,
    int k_max,
    const float *A,
    const float *B,
    std::vector<unsigned char>& slab) {

    if (!A || !B)
        return -1;

    if (K < 1 || K > k_max)
        return -2;

    if (M < 1 || N < 1 || M > P || N > P)
        return -3;

    // Every window step K - 1 lands in must lie inside the slab.
    const size_t windows = static_cast<size_t>((K + 7) / 8);
    if (slab.size() < windows * 128 * 4)
        return -4;

    std::fill(slab.begin(), slab.end(), static_cast<unsigned char>(0));

    for (int k = 0; k < K; ++k) {
        for (int row = 0; row < M; ++row) {
            scheduled_store_f32(
                slab,
                scheduled_payload_index_a(k, row),
                A[row * K + k]);
        }

        for (int col = 0; col < N; ++col) {
            scheduled_store_f32(
                slab,
                scheduled_payload_index_b(k, col),
                B[k * N + col]);
        }
    }

    return 0;
}

namespace {

/*
 * The 1x8x8 + 3x4x4 bitstream:
 *
 *   device_id 0: 8x8 instance 0     K_MAX_8X8 reduction steps
 *   device_id 1: 4x4 instance 0     K_MAX_4X4
 *   device_id 2: 4x4 instance 1     K_MAX_4X4
 *   device_id 3: 4x4 instance 2     K_MAX_4X4
 *
 * Every invocation is one operand slab at OPERAND_BASE, written by one
 * MEM_WRITE of the bitstream's fixed payload length (config slab_bytes: the
 * deepest geometry's slab, so any device's slab fits); the result tile is
 * written back at RESULT_BASE and streamed back over DPTI, P x P fp32
 * row-major for the P x P array.
 */
constexpr uint32_t OPERAND_BASE  = 0x00010000u;
constexpr uint32_t RESULT_BASE   = 0x00003000u;
constexpr int      SCHED_DEVICES = 4;

int physical_geometry(unsigned device_id) {
    if (device_id == 0)
        return 8;
    if (device_id >= 1 && device_id < SCHED_DEVICES)
        return 4;
    return 0;
}

/* ceil(k / 8) windows of 8 steps x 8 lanes x (A word + B word). */
unsigned slab_bytes_for(unsigned k_max) {
    return (k_max + 7u) / 8u * 8u * 8u * 8u;
}

/*
 * The bitstream configuration: SYSTOLIC_FPGA_CONFIG, or the archived
 * bitstream's 16 / 16. Loaded once.
 */
struct LoadedConfig {
    bool                  loaded = false;
    int                   rc = 0;
    fpga_runtime_config_t cfg{};
};

void set_text(char *dst, size_t n, const std::string &text) {
    std::snprintf(dst, n, "%s", text.c_str());
}

std::string trim(const std::string &s) {
    size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

/* A decimal integer in 1..max, the whole string, without a sign or leading
 * zeros: the build scripts that write and read the same file do arithmetic
 * in bash and Tcl, where a leading 0 means octal, so "016" is refused rather
 * than read as 16 here and as 14 there. */
bool parse_count(const std::string &text, unsigned max, unsigned &out) {
    if (text.empty() || text.size() > 10 || text[0] == '0')
        return false;
    for (char c : text)
        if (c < '0' || c > '9')
            return false;
    const unsigned long v = std::strtoul(text.c_str(), nullptr, 10);
    if (v < 1 || v > max)
        return false;
    out = static_cast<unsigned>(v);
    return true;
}

int load_config(fpga_runtime_config_t &cfg) {
    cfg = fpga_runtime_config_t{};
    cfg.k_max_8x8 = 16;
    cfg.k_max_4x4 = 16;
    cfg.pipeline_depth = 1;

    const char *path = std::getenv("SYSTOLIC_FPGA_CONFIG");
    if (!path || !*path) {
        set_text(cfg.source, sizeof cfg.source,
                 "built-in default (artifacts/fpga/tile_pipelined/"
                 "1x8x8-3x4x4: K_MAX_8X8 = K_MAX_4X4 = 16)");
        cfg.slab_bytes = slab_bytes_for(16);
        return 0;
    }
    set_text(cfg.source, sizeof cfg.source, path);

    auto fail = [&](const std::string &why) {
        set_text(cfg.error, sizeof cfg.error,
                 std::string("SYSTOLIC_FPGA_CONFIG=") + path + ": " + why);
        cfg.k_max_8x8 = cfg.k_max_4x4 = cfg.slab_bytes = 0;
        cfg.pipeline_depth = 0;
        return -16;
    };

    std::ifstream in(path);
    if (!in)
        return fail(std::string("cannot open (") + std::strerror(errno) + ")");

    bool have8 = false, have4 = false;
    std::string line;
    int lineno = 0;
    while (std::getline(in, line)) {
        ++lineno;
        const size_t hash = line.find('#');
        if (hash != std::string::npos)
            line.resize(hash);
        line = trim(line);
        if (line.empty())
            continue;
        const size_t eq = line.find('=');
        if (eq == std::string::npos)
            return fail("line " + std::to_string(lineno) +
                        ": expected KEY=VALUE");
        const std::string key = trim(line.substr(0, eq));
        const std::string val = trim(line.substr(eq + 1));
        unsigned v = 0;
        if (key == "K_MAX_8X8" || key == "K_MAX_4X4") {
            if (!parse_count(val, FPGA_K_MAX_LIMIT, v))
                return fail(key + " = '" + val + "' is not a decimal integer in 1.." +
                            std::to_string(FPGA_K_MAX_LIMIT) +
                            " (no sign, no leading zeros)");
            if (key == "K_MAX_8X8") { cfg.k_max_8x8 = v; have8 = true; }
            else                    { cfg.k_max_4x4 = v; have4 = true; }
        } else if (key == "COMPLETION_HEADER") {
            if (val != "0" && val != "1")
                return fail(key + " = '" + val + "' is not 0 or 1");
            cfg.completion_header = (val == "1") ? 1u : 0u;
        } else if (key == "PIPELINE_DEPTH") {
            // The context's job queue: how many invocations the bitstream
            // holds per accelerator. The compiler declares the fleet's
            // pipelining from it; this runtime's own protocol does not
            // change with it.
            if (!parse_count(val, 64u, v))
                return fail(key + " = '" + val + "' is not a decimal integer in 1..64 "
                            "(no sign, no leading zeros)");
            cfg.pipeline_depth = v;
        } else if (key == "N" || key == "NUM_8X8" || key == "NUM_4X4") {
            // This runtime drives the 1x8x8 + 3x4x4 device map; a bitstream
            // built with another fleet is not something it can address.
            const unsigned want = key == "N" ? 8u : key == "NUM_8X8" ? 1u : 3u;
            if (!parse_count(val, 1u << 16, v) || v != want)
                return fail(key + " = '" + val + "', but this runtime drives "
                            "the 1x8x8 + 3x4x4 fleet (" + key + " = " +
                            std::to_string(want) + ")");
        }
        // Every other key (bitstream hash, RTL revision, ...) is a record
        // for people and is not read here.
    }
    if (!have8 || !have4)
        return fail("K_MAX_8X8 and K_MAX_4X4 are both required");

    const unsigned b8 = slab_bytes_for(cfg.k_max_8x8);
    const unsigned b4 = slab_bytes_for(cfg.k_max_4x4);
    cfg.slab_bytes = b8 > b4 ? b8 : b4;
    return 0;
}

LoadedConfig &config_state() {
    static LoadedConfig state;
    if (!state.loaded) {
        state.rc = load_config(state.cfg);
        state.loaded = true;
    }
    return state;
}

/* K_MAX of a physical device; 0 for an unknown one. */
unsigned physical_k_max(const fpga_runtime_config_t &cfg, unsigned device_id) {
    if (device_id == 0)
        return cfg.k_max_8x8;
    if (device_id >= 1 && device_id < SCHED_DEVICES)
        return cfg.k_max_4x4;
    return 0;
}

struct ScheduledRuntime {
    DptiFtdiSession      *session = nullptr;
    unsigned              next_job_id = 0;

    /*
     * The session could not be opened, or a MEM_WRITE, descriptor or result
     * read failed on it. The link may be mid-packet, or a late result may
     * still arrive and be read as the next invocation's, so nothing more is
     * sent until fpga_runtime_close(); the next session starts purged.
     */
    bool                  link_failed = false;
    bool                  refusal_reported = false;

    fpga_runtime_stats_t  stats{};

    /* Every successful invocation's wall-clock, in microseconds, in order,
     * for the median / p95 of the SYSTOLIC_RUNTIME_TIMING summary. Reset
     * with the stats. */
    std::vector<uint32_t> invocation_us;
};

/*
 * SYSTOLIC_DPTI_SETTLE_US: the pause between the MEM_WRITE and the
 * descriptor. 1000 (1 ms) is what the board runs used and stays the
 * default; 0 removes it. The acceptance logic of the RTL already holds a
 * descriptor until the staged slab is in DDR (systolic_dma_top.sv,
 * core_job_ready: !hs_busy), so the pause is a measured quantity, not a
 * correctness requirement -- docs/ASYNC.md records what it costs.
 */
uint32_t settle_us() {
    static long v = -1;
    if (v < 0) {
        v = 1000;
        if (const char *s = std::getenv("SYSTOLIC_DPTI_SETTLE_US")) {
            char *end = nullptr;
            const long t = std::strtol(s, &end, 10);
            if (*s && end && !*end && t >= 0 && t <= 1000000)
                v = t;
            else
                std::fprintf(stderr,
                             "[fpga-runtime] SYSTOLIC_DPTI_SETTLE_US='%s' ignored: "
                             "expected 0..1000000\n", s);
        }
    }
    return static_cast<uint32_t>(v);
}

using clock_t_ = std::chrono::steady_clock;

unsigned long long ns_between(clock_t_::time_point a, clock_t_::time_point b) {
    return static_cast<unsigned long long>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(b - a).count());
}

ScheduledRuntime &runtime_state() {
    static ScheduledRuntime state;
    return state;
}

bool env_flag(const char *name) {
    const char *v = std::getenv(name);
    return v && *v && std::strcmp(v, "0") != 0;
}

/* SYSTOLIC_DPTI_TIMEOUT_MS overrides the 5 s the board run used, for a
 * slower transport (the RTL co-simulation). */
uint32_t read_timeout_ms() {
    static uint32_t timeout = 0;
    if (timeout == 0) {
        timeout = 5000;
        if (const char *v = std::getenv("SYSTOLIC_DPTI_TIMEOUT_MS")) {
            const long t = std::strtol(v, nullptr, 10);
            if (t > 0)
                timeout = static_cast<uint32_t>(t);
        }
    }
    return timeout;
}

} // namespace

/*
 * Read one completion record from the session. 0, or -7 (read failed),
 * -8 (timed out), -9 (the magic is not there: the host is out of step).
 */
int fpga_runtime_read_completion(fpga_completion_t *out) {
    ScheduledRuntime &rt = runtime_state();
    unsigned char h[FPGA_COMPLETION_BYTES];
    const int rc = dpti_ftdi_read_exact_session(
        rt.session, h, FPGA_COMPLETION_BYTES, read_timeout_ms());
    if (rc != 0)
        return rc == -4 ? -8 : -7;
    auto u32 = [&](int i) -> unsigned {
        return static_cast<unsigned>(h[i]) | (static_cast<unsigned>(h[i + 1]) << 8) |
               (static_cast<unsigned>(h[i + 2]) << 16) | (static_cast<unsigned>(h[i + 3]) << 24);
    };
    out->magic         = u32(0);
    out->job_id        = u32(4);
    out->device_id     = h[8];
    out->status        = h[9];
    out->payload_bytes = static_cast<unsigned>(h[10]) | (static_cast<unsigned>(h[11]) << 8);
    out->seq           = u32(12);
    if (out->magic != FPGA_COMPLETION_MAGIC) {
        std::fprintf(stderr,
                     "[fpga-runtime] completion record magic 0x%08x (expected 0x%08x): "
                     "the host is out of step with the link\n",
                     out->magic, FPGA_COMPLETION_MAGIC);
        return -9;
    }
    return 0;
}

namespace {

int scheduled_invocation(
    int M, int K, int N,
    const float *A, const float *B, float *C,
    unsigned device_id,
    unsigned start_cycle,
    unsigned est_cycles) {

    const LoadedConfig &hw = config_state();
    if (hw.rc != 0)
        return -16;

    const int P = physical_geometry(device_id);

    if (P == 0)
        return -11;

    // This device's own capacity: K_MAX_8X8 or K_MAX_4X4.
    const int k_max =
        static_cast<int>(physical_k_max(hw.cfg, device_id));

    if (K < 1 || K > k_max)
        return -10;

    if (M < 1 || N < 1 || M > P || N > P)
        return -11;

    if (!A || !B || !C)
        return -12;

    // dpti_job_validate() would refuse a zero estimate after the slab was
    // already on the link; refuse it before anything is sent.
    if (est_cycles == 0)
        return -15;

    ScheduledRuntime &rt = runtime_state();

    if (rt.link_failed)
        return -14;

    if (!rt.session) {
        rt.session = dpti_ftdi_open();

        if (!rt.session) {
            // No board (or no permission): retrying for every invocation
            // only repeats the same failure.
            rt.link_failed = true;
            return -3;
        }
    }

    // One MEM_WRITE of the bitstream's fixed payload length; this device's
    // slab is its first ceil(k_max / 8) windows.
    std::vector<unsigned char> slab(hw.cfg.slab_bytes, 0);

    if (build_scheduled_operand_slab(M, K, N, P, k_max, A, B, slab) != 0)
        return -13;

    /*
     * Stage the slab into the physical operand region validated by the
     * board run. The call is synchronous -- the result is back before it
     * returns -- so the next call can reuse the same DDR regions.
     */
    const clock_t_::time_point t_start = clock_t_::now();

    const int mem_rc =
        dpti_ftdi_mem_write_session(
            rt.session,
            OPERAND_BASE,
            slab.data(),
            static_cast<uint32_t>(slab.size()));

    if (mem_rc != 0) {
        rt.link_failed = true;
        return -4;
    }

    const clock_t_::time_point t_mem_write = clock_t_::now();

    // Kept from the board-verified sequence: let MEM_WRITE settle
    // (1 ms unless SYSTOLIC_DPTI_SETTLE_US says otherwise).
    if (const uint32_t us = settle_us())
        std::this_thread::sleep_for(std::chrono::microseconds(us));

    const clock_t_::time_point t_settle = clock_t_::now();

    dpti_job_t job = {};

    job.job_id = rt.next_job_id++;
    job.device_id = device_id;

    /*
     * The descriptor carries this invocation: the array's P x P, and K, the
     * reduction steps it runs (1 <= K <= K_MAX of this device, checked
     * above). K_MAX is the operand buffer's capacity, not the depth of every
     * invocation: the RTL reads ceil(K / 8) windows of the slab and feeds
     * exactly K steps (systolic_accel_context.sv: job_k_reg, k_dim). The
     * logical m x n lives in the zero rows and columns of the slab -- the
     * array computes its whole P x P tile in the same time either way.
     */
    job.m = static_cast<uint32_t>(P);
    job.n = static_cast<uint32_t>(P);
    job.k = static_cast<uint32_t>(K);

    job.start_cycle = start_cycle;
    job.est_cycles = est_cycles;

    /*
     * FPGA DDR addresses, never host pointers. The RTL fetches one
     * combined A+B slab from a_base, so b_base is zero.
     */
    job.a_base = OPERAND_BASE;
    job.b_base = 0;
    job.c_base = RESULT_BASE;

    const int submit_rc =
        dpti_ftdi_submit_session(
            rt.session,
            &job);

    if (submit_rc != 0) {
        rt.link_failed = true;
        return submit_rc == -6 ? -6 : -5;
    }

    const clock_t_::time_point t_submit = clock_t_::now();

    const uint32_t result_bytes_len =
        static_cast<uint32_t>(P * P * sizeof(float));

    std::array<unsigned char, 8 * 8 * sizeof(float)> result_bytes{};

    // A bitstream with the completion record sends it ahead of the tile:
    // the job_id and device it names must be this invocation's, and the
    // payload length the tile's. Anything else means the host and the link
    // are out of step (or the descriptor was refused), and this session
    // sends nothing more.
    if (hw.cfg.completion_header) {
        fpga_completion_t c{};
        const int hdr_rc = fpga_runtime_read_completion(&c);
        if (hdr_rc != 0) {
            rt.link_failed = true;
            return hdr_rc;
        }
        if (c.job_id != job.job_id || c.device_id != device_id ||
            c.status != FPGA_COMPLETION_OK || c.payload_bytes != result_bytes_len) {
            std::fprintf(
                stderr,
                "[fpga-runtime] completion record job=%u device=%u status=%u "
                "payload=%u B does not match invocation job=%u device=%u "
                "(%u B expected)%s\n",
                c.job_id, c.device_id, c.status, c.payload_bytes,
                job.job_id, device_id, result_bytes_len,
                c.status == FPGA_COMPLETION_BAD_K ? "; the bitstream refused k" :
                c.status == FPGA_COMPLETION_BAD_DEV ? "; the bitstream has no such device" : "");
            rt.link_failed = true;
            return -9;
        }
    }

    // The synchronization point: the whole P x P tile is back.
    const int read_rc =
        dpti_ftdi_read_exact_session(
            rt.session,
            result_bytes.data(),
            result_bytes_len,
            read_timeout_ms());

    if (read_rc != 0) {
        rt.link_failed = true;
        return read_rc == -4 ? -8 : -7;
    }

    const clock_t_::time_point t_read = clock_t_::now();

    {
        fpga_runtime_stats_t &s = rt.stats;
        const unsigned long long total = ns_between(t_start, t_read);
        s.ns_total     += total;
        s.ns_mem_write += ns_between(t_start, t_mem_write);
        s.ns_settle    += ns_between(t_mem_write, t_settle);
        s.ns_submit    += ns_between(t_settle, t_submit);
        s.ns_read      += ns_between(t_submit, t_read);
        if (s.ns_min == 0 || total < s.ns_min) s.ns_min = total;
        if (total > s.ns_max)                  s.ns_max = total;
        if (rt.invocation_us.size() < (1u << 20))
            rt.invocation_us.push_back(static_cast<uint32_t>(total / 1000));
    }

    /*
     * C_out = C_in + A x B: the FPGA computed this invocation's product;
     * add its valid m x n block into the caller's tile. K invocations of
     * one output tile chain through C exactly this way.
     */
    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            float v = 0.0f;
            memcpy(&v,
                   result_bytes.data() +
                       (static_cast<size_t>(i) * P + j) * sizeof(float),
                   sizeof(float));
            C[i * N + j] += v;
        }
    }

    rt.stats.invocations++;
    rt.stats.invocations_per_device[device_id]++;

    if (env_flag("SYSTOLIC_RUNTIME_TRACE")) {
        std::fprintf(
            stderr,
            "[fpga-runtime] job %u -> device %u (%dx%d array, K_MAX %d): "
            "tile %dx%d, job.k=%u, "
            "start_cycle=%u est_cycles=%u, %zu-byte slab, %u result bytes\n",
            job.job_id, device_id, P, P, k_max, M, N, job.k,
            start_cycle, est_cycles, slab.size(), result_bytes_len);
    }

    return 0;
}

} // namespace

int fpga_matmul_tiled_auto_scheduled(
    int M, int K, int N,
    const float *A, const float *B, float *C,
    unsigned device_id,
    unsigned start_cycle,
    unsigned est_cycles) {

    ScheduledRuntime &rt = runtime_state();

    /*
     * The bytes this call puts on / takes off the link, failed calls
     * included. A session the call opens starts counting at zero.
     */
    const unsigned long long written_before =
        rt.session ? dpti_ftdi_bytes_written(rt.session) : 0;
    const unsigned long long read_before =
        rt.session ? dpti_ftdi_bytes_read(rt.session) : 0;

    const int rc =
        scheduled_invocation(
            M, K, N, A, B, C,
            device_id, start_cycle, est_cycles);

    if (rt.session) {
        rt.stats.bytes_to_fpga +=
            dpti_ftdi_bytes_written(rt.session) - written_before;
        rt.stats.bytes_from_fpga +=
            dpti_ftdi_bytes_read(rt.session) - read_before;
    }

    if (rc != 0) {
        fpga_runtime_stats_t &stats = rt.stats;

        stats.failed_calls++;

        if (stats.first_error == 0)
            stats.first_error = rc;

        if (rc != -14) {
            std::fprintf(
                stderr,
                "[fpga-runtime] invocation on device %u (tile %dx%d, k=%d) "
                "failed: rc=%d\n",
                device_id, M, N, K, rc);
        } else if (!rt.refusal_reported) {
            rt.refusal_reported = true;
            std::fprintf(
                stderr,
                "[fpga-runtime] the DPTI link failed earlier in this "
                "session; this and every further invocation is refused "
                "(rc=-14)\n");
        }
    }

    return rc;
}

void fpga_runtime_get_stats(fpga_runtime_stats_t *out) {
    if (out)
        *out = runtime_state().stats;
}

void fpga_runtime_reset_stats(void) {
    runtime_state().stats = fpga_runtime_stats_t{};
    runtime_state().invocation_us.clear();
}

namespace {

/*
 * SYSTOLIC_RUNTIME_TIMING=1: one machine-readable line on stderr at close
 * (and at every stats reset that follows invocations), so a harness run
 * keeps the host-side cost of every case next to its verdict without the
 * driver having to know about timing at all. Percentiles are of the
 * per-invocation wall-clock (nearest-rank).
 */
void report_timing(ScheduledRuntime &rt) {
    if (!env_flag("SYSTOLIC_RUNTIME_TIMING"))
        return;
    const fpga_runtime_stats_t &s = rt.stats;
    if (s.invocations == 0 && rt.invocation_us.empty())
        return;

    std::vector<uint32_t> sorted(rt.invocation_us);
    std::sort(sorted.begin(), sorted.end());
    auto rank = [&](double q) -> unsigned long long {
        if (sorted.empty())
            return 0;
        size_t i = static_cast<size_t>(q * static_cast<double>(sorted.size()));
        if (i >= sorted.size())
            i = sorted.size() - 1;
        return sorted[i];
    };

    std::fprintf(
        stderr,
        "[fpga-runtime] timing invocations=%llu total_us=%llu min_us=%llu "
        "median_us=%llu p95_us=%llu max_us=%llu mem_write_us=%llu "
        "settle_us=%llu submit_us=%llu read_us=%llu settle_cfg_us=%u "
        "bytes_to_fpga=%llu bytes_from_fpga=%llu\n",
        s.invocations,
        s.ns_total / 1000,
        s.ns_min / 1000,
        rank(0.5),
        rank(0.95),
        s.ns_max / 1000,
        s.ns_mem_write / 1000,
        s.ns_settle / 1000,
        s.ns_submit / 1000,
        s.ns_read / 1000,
        settle_us(),
        s.bytes_to_fpga,
        s.bytes_from_fpga);
}

} // namespace

int fpga_runtime_get_config(fpga_runtime_config_t *out) {
    const LoadedConfig &hw = config_state();
    if (out)
        *out = hw.cfg;
    return hw.rc;
}

unsigned fpga_runtime_device_geometry(unsigned device_id) {
    return static_cast<unsigned>(physical_geometry(device_id));
}

unsigned fpga_runtime_device_k_max(const fpga_runtime_config_t *cfg,
                                   unsigned device_id) {
    return cfg ? physical_k_max(*cfg, device_id) : 0;
}

int fpga_runtime_pack_operand_slab(int M, int K, int N, int P, int k_max,
                                   const float *A, const float *B,
                                   unsigned char *slab, unsigned slab_bytes) {
    if (!A || !B || !slab)
        return -12;
    if (K < 1 || K > k_max)
        return -10;
    if (M < 1 || N < 1 || M > P || N > P)
        return -11;
    const size_t windows = static_cast<size_t>((K + 7) / 8);
    if (slab_bytes < windows * 128 * 4)
        return -13;

    std::memset(slab, 0, slab_bytes);
    auto store = [&](int word_index, float value) {
        uint32_t bits = 0;
        std::memcpy(&bits, &value, sizeof bits);
        unsigned char *p = slab + static_cast<size_t>(word_index) * 4;
        p[0] = static_cast<unsigned char>(bits);
        p[1] = static_cast<unsigned char>(bits >> 8);
        p[2] = static_cast<unsigned char>(bits >> 16);
        p[3] = static_cast<unsigned char>(bits >> 24);
    };
    for (int k = 0; k < K; ++k) {
        for (int row = 0; row < M; ++row)
            store(scheduled_payload_index_a(k, row), A[row * K + k]);
        for (int col = 0; col < N; ++col)
            store(scheduled_payload_index_b(k, col), B[k * N + col]);
    }
    return 0;
}

void fpga_runtime_close(void) {
    ScheduledRuntime &rt = runtime_state();
    report_timing(rt);
    if (rt.session) {
        dpti_ftdi_close(rt.session);
        rt.session = nullptr;
    }
    rt.link_failed = false;
    rt.refusal_reported = false;
}
