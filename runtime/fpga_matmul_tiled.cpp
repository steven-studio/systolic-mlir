// fpga_matmul_tiled.cpp -- the scheduled FPGA entry point the compiler calls:
// one invocation of one physical array of the 1x8x8 + 3x4x4 bitstream over
// the DPTI link (dpti_ftdi.cpp).

#include "fpga_matmul_tiled.h"
#include "dpti_runtime.h"

#include <stdlib.h>
#include <string.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>

/*
 * Physical 4x4 accelerator operand slab.
 *
 * For K=16 the hardware consumes one 1024-byte combined A+B slab:
 *
 *   window 0:
 *     words   0..63   A
 *     words  64..127  B
 *
 *   window 1:
 *     words 128..191  A
 *     words 192..255  B
 *
 * A is a row-major 4x16 logical tile.
 * B is a row-major 16x4 logical tile.
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
    std::array<unsigned char, 1024>& slab,
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
 * Pack one invocation into the 1024-byte slab.
 *
 * The layout is the K = 16 image of the 8x8 geometry; a 4x4 array reads
 * lanes 0..3 of it. A full 8x8x16 or 4x4x16 tile therefore packs to exactly
 * the bytes the board run of 2026-10-09 sent. A smaller tile -- m < P,
 * n < P or k < 16 for the P x P array of the selected device -- leaves the
 * rest of the slab zero: zero rows of A and zero columns of B give zero rows
 * and columns of the product, and a zero reduction step adds +0.0, so the
 * m x n block of the P x P result is the m x k by k x n product.
 */
static int build_scheduled_operand_slab(
    int M,
    int K,
    int N,
    int P,
    const float *A,
    const float *B,
    std::array<unsigned char, 1024>& slab) {

    if (!A || !B)
        return -1;

    if (K < 1 || K > 16)
        return -2;

    if (M < 1 || N < 1 || M > P || N > P)
        return -3;

    slab.fill(0);

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
 * The 1x8x8 + 3x4x4 bitstream (tag tile-pipelined-1x8x8-3x4x4-20261009):
 *
 *   device_id 0: 8x8 instance 0
 *   device_id 1: 4x4 instance 0
 *   device_id 2: 4x4 instance 1
 *   device_id 3: 4x4 instance 2
 *
 * Every invocation is one 1024-byte operand slab of depth 16 at
 * OPERAND_BASE; the result tile is written back at RESULT_BASE and streamed
 * back over DPTI, P x P fp32 row-major for the P x P array.
 */
constexpr int      SCHED_K_MAX   = 16;
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
};

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

int scheduled_invocation(
    int M, int K, int N,
    const float *A, const float *B, float *C,
    unsigned device_id,
    unsigned start_cycle,
    unsigned est_cycles) {

    const int P = physical_geometry(device_id);

    if (P == 0)
        return -11;

    if (K < 1 || K > SCHED_K_MAX)
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

    std::array<unsigned char, 1024> slab{};

    if (build_scheduled_operand_slab(M, K, N, P, A, B, slab) != 0)
        return -13;

    /*
     * Stage the slab into the physical operand region validated by the
     * board run. The call is synchronous -- the result is back before it
     * returns -- so the next call can reuse the same DDR regions.
     */
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

    // Kept from the board-verified sequence: let MEM_WRITE settle.
    std::this_thread::sleep_for(
        std::chrono::milliseconds(1));

    dpti_job_t job = {};

    job.job_id = rt.next_job_id++;
    job.device_id = device_id;

    /*
     * The descriptor carries the physical invocation: the array's P x P
     * and the slab depth 16, the only shape the board has run. The logical
     * m x k x n lives in the zero padding of the slab.
     */
    job.m = static_cast<uint32_t>(P);
    job.n = static_cast<uint32_t>(P);
    job.k = static_cast<uint32_t>(SCHED_K_MAX);

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

    const uint32_t result_bytes_len =
        static_cast<uint32_t>(P * P * sizeof(float));

    std::array<unsigned char, 8 * 8 * sizeof(float)> result_bytes{};

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
            "[fpga-runtime] job %u -> device %u (%dx%d array): "
            "tile %dx%d, k=%d (padded to %dx%dx%d), "
            "start_cycle=%u est_cycles=%u, %u result bytes\n",
            job.job_id, device_id, P, P, M, N, K, P, SCHED_K_MAX, P,
            start_cycle, est_cycles, result_bytes_len);
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
}

void fpga_runtime_close(void) {
    ScheduledRuntime &rt = runtime_state();
    if (rt.session) {
        dpti_ftdi_close(rt.session);
        rt.session = nullptr;
    }
    rt.link_failed = false;
    rt.refusal_reported = false;
}
