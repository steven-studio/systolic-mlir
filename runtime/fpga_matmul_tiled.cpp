#include "fpga_matmul_tiled.h"
#include "fpga_matmul4x4.h"
#include "systolic_runtime.h"
#include "dpti_runtime.h"
#include <stdlib.h>
#include <string.h>
#include <array>
#include <cstdint>
#include <cstdio>

static int ceil_div4(int x) { return (x + 3) / 4; }

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

static int build_scheduled_operand_slab(
    int M,
    int K,
    int N,
    const float *A,
    const float *B,
    std::array<unsigned char, 1024>& slab) {

    if (!A || !B)
        return -1;

    if (K != 16)
        return -2;

    if ((M != 4 && M != 8) ||
        (N != 4 && N != 8) ||
        M != N)
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

// 從 (MxK) row-major 矩陣中取出以 (row0, col0) 為左上角的 4x4 子區塊
// 超出原矩陣範圍的部分補 0
static void extract_tile(const float *M_mat, int rows, int cols,
                          int row0, int col0, float tile[16]) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            int r = row0 + i, c = col0 + j;
            tile[i*4+j] = (r < rows && c < cols) ? M_mat[r*cols + c] : 0.0f;
        }
    }
}

// 把 4x4 tile 寫回 C(MxN),只寫有效範圍(超出的部分丟棄)
static void writeback_tile(float *C, int M, int N,
                            int row0, int col0, const float tile[16]) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            int r = row0 + i, c = col0 + j;
            if (r < M && c < N) C[r*N + c] = tile[i*4+j];
        }
    }
}

int fpga_matmul_tiled(int fd, int M, int K, int N,
                       const float *A, const float *B, float *C) {
    int M_tiles = ceil_div4(M);
    int K_tiles = ceil_div4(K);
    int N_tiles = ceil_div4(N);

    float zero16[16];
    memset(zero16, 0, sizeof(zero16));

    for (int it = 0; it < M_tiles; it++) {
        for (int jt = 0; jt < N_tiles; jt++) {
            float acc[16];
            memcpy(acc, zero16, sizeof(acc));  // 這個輸出 tile 的累加起點

            for (int kt = 0; kt < K_tiles; kt++) {
                float a_tile[16], b_tile[16], out_tile[16];
                extract_tile(A, M, K, it*4, kt*4, a_tile);
                extract_tile(B, K, N, kt*4, jt*4, b_tile);

                // acc = a_tile @ b_tile + acc  (硬體協定原生支援累加)
                int rc = fpga_matmul4x4(fd, a_tile, b_tile, acc, out_tile);
                if (rc != 0) return rc;
                memcpy(acc, out_tile, sizeof(acc));
            }

            writeback_tile(C, M, N, it*4, jt*4, acc);
        }
    }
    return 0;
}

static int g_fpga_fd = -2;  // -2 = 尚未嘗試開啟

int fpga_matmul_tiled_auto(int M, int K, int N,
                            const float *A, const float *B, float *C) {
    if (g_fpga_fd == -2) {
        g_fpga_fd = fpga_uart_open("/dev/ttyUSB2");
    }
    if (g_fpga_fd < 0) return -3;  // 開啟失敗
    return fpga_matmul_tiled(g_fpga_fd, M, K, N, A, B, C);
}

int fpga_matmul_tiled_auto_scheduled(
    int M, int K, int N,
    const float *A, const float *B, float *C,
    unsigned device_id,
    unsigned start_cycle,
    unsigned est_cycles) {

    /*
     * Current compiler/RTL contract:
     *
     *   device_id 0: 8x16 @ 16x8 -> 8x8
     *   device_id 1: 4x16 @ 16x4 -> 4x4
     *
     * Both geometries use the same 1024-byte combined A+B slab.
     * The result size is determined by the selected output geometry.
     */
    if (K != 16)
        return -10;

    const bool is4x4 =
        M == 4 && N == 4 && device_id == 1;

    const bool is8x8 =
        M == 8 && N == 8 && device_id == 0;

    if (!is4x4 && !is8x8)
        return -11;

    if (!A || !B || !C)
        return -12;

    constexpr uint32_t OPERAND_BASE = 0x00010000u;
    constexpr uint32_t RESULT_BASE  = 0x00003000u;
    constexpr uint32_t MAX_RESULT_BYTES = 8u * 8u * sizeof(float);

    const uint32_t resultBytes =
        static_cast<uint32_t>(M * N * sizeof(float));

    static unsigned next_job_id = 0;
    static DptiFtdiSession *scheduled_session = nullptr;

    if (!scheduled_session) {
        scheduled_session = dpti_ftdi_open();

        if (!scheduled_session)
            return -3;
    }

    std::array<unsigned char, 1024> slab{};

    // Temporary diagnostic: inspect the first compiler-generated logical tile
    // exactly as it arrives at the scheduled runtime boundary.
    if (next_job_id == 0) {
        std::fprintf(
            stderr,
            "[SCHED-DIAG] JOB0 A (%dx%d):\n",
            M,
            K);

        for (int r = 0; r < M; ++r) {
            std::fprintf(stderr, "  A[%d]:", r);
            for (int k = 0; k < K; ++k)
                std::fprintf(
                    stderr,
                    " %g",
                    static_cast<double>(A[r * K + k]));
            std::fprintf(stderr, "\n");
        }

        std::fprintf(
            stderr,
            "[SCHED-DIAG] JOB0 B (%dx%d):\n",
            K,
            N);

        for (int k = 0; k < K; ++k) {
            std::fprintf(stderr, "  B[%d]:", k);
            for (int c = 0; c < N; ++c)
                std::fprintf(
                    stderr,
                    " %g",
                    static_cast<double>(B[k * N + c]));
            std::fprintf(stderr, "\n");
        }
    }

    const int pack_rc =
        build_scheduled_operand_slab(
            M,
            K,
            N,
            A,
            B,
            slab);

    if (pack_rc != 0)
        return -13;

    // Temporary diagnostic: decode selected JOB0 words from the
    // fully packed 1024-byte operand slab before staging it.
    if (next_job_id == 0) {
        auto load_slab_f32 = [&](int word_index) {
            uint32_t bits = 0;
            const size_t byte_index =
                static_cast<size_t>(word_index) * 4;

            bits |= static_cast<uint32_t>(slab[byte_index + 0]);
            bits |= static_cast<uint32_t>(slab[byte_index + 1]) << 8;
            bits |= static_cast<uint32_t>(slab[byte_index + 2]) << 16;
            bits |= static_cast<uint32_t>(slab[byte_index + 3]) << 24;

            float value = 0.0f;
            memcpy(&value, &bits, sizeof(value));
            return value;
        };

        const int a00 = scheduled_payload_index_a(0, 0);
        const int a11 = scheduled_payload_index_a(1, 1);

        const int b00  = scheduled_payload_index_b(0, 0);
        const int b01  = scheduled_payload_index_b(0, 1);
        const int b10  = scheduled_payload_index_b(1, 0);
        const int b120 = scheduled_payload_index_b(12, 0);
        const int b153 = scheduled_payload_index_b(15, 3);

        std::fprintf(
            stderr,
            "[SCHED-DIAG] JOB0 slab:"
            " A00=%g A11=%g"
            " B00=%g B01=%g B10=%g B120=%g B153=%g\n",
            static_cast<double>(load_slab_f32(a00)),
            static_cast<double>(load_slab_f32(a11)),
            static_cast<double>(load_slab_f32(b00)),
            static_cast<double>(load_slab_f32(b01)),
            static_cast<double>(load_slab_f32(b10)),
            static_cast<double>(load_slab_f32(b120)),
            static_cast<double>(load_slab_f32(b153)));
    }

    /*
     * Stage this compiler-generated logical tile into the same
     * physical operand region validated by the JOB0 numerical test.
     *
     * This runtime is intentionally synchronous for now: the result
     * is completely returned before this function exits, so the same
     * physical DDR regions can safely be reused by the next call.
     */
    const int mem_rc =
        dpti_ftdi_mem_write_session(
            scheduled_session,
            OPERAND_BASE,
            slab.data(),
            static_cast<uint32_t>(slab.size()));

    if (mem_rc != 0)
        return mem_rc;

    dpti_job_t physical_job = {};

    physical_job.job_id = next_job_id++;
    physical_job.device_id = device_id;

    physical_job.m = static_cast<uint32_t>(M);
    physical_job.n = static_cast<uint32_t>(N);
    physical_job.k = static_cast<uint32_t>(K);

    physical_job.start_cycle = start_cycle;
    physical_job.est_cycles = est_cycles;

    /*
     * Current RTL operand fetch uses one combined A+B slab selected
     * by a_base. b_base is therefore intentionally zero.
     *
     * These are FPGA DDR addresses, never host virtual pointers.
     */
    physical_job.a_base = OPERAND_BASE;
    physical_job.b_base = 0;
    physical_job.c_base = RESULT_BASE;

    const int submit_rc =
        dpti_ftdi_submit_session(
            scheduled_session,
            &physical_job);

    if (submit_rc != 0)
        return submit_rc;

    std::array<unsigned char, MAX_RESULT_BYTES> result_bytes{};

    /*
     * This is also the synchronization point for the current runtime:
     * do not return until the complete 4x4 result tile has arrived.
     */
    const int read_rc =
        dpti_ftdi_read_exact_session(
            scheduled_session,
            result_bytes.data(),
            resultBytes,
            5000);

    if (read_rc != 0)
        return read_rc;

    // Temporary diagnostic: inspect the raw FP32 tile returned
    // for the first compiler-generated physical job, before copying
    // it into the compiler-provided output buffer.
    if (physical_job.job_id == 0) {
        std::array<float, 64> result_tile{};

        memcpy(
            result_tile.data(),
            result_bytes.data(),
            resultBytes);

        std::fprintf(
            stderr,
            "[SCHED-DIAG] JOB0 raw result (%dx%d):\n",
            M,
            N);

        for (int r = 0; r < M; ++r) {
            for (int c = 0; c < N; ++c) {
                const float value = result_tile[r * N + c];

                uint32_t bits = 0;
                static_assert(sizeof(bits) == sizeof(value));
                memcpy(&bits, &value, sizeof(bits));

                const float expected =
                    static_cast<float>(r * 16 + c);

                uint32_t expected_bits = 0;
                memcpy(
                    &expected_bits,
                    &expected,
                    sizeof(expected_bits));

                std::fprintf(
                    stderr,
                    "  C[%d,%d] value=% .9g bits=0x%08x "
                    "expected=% .9g expected_bits=0x%08x%s\n",
                    r,
                    c,
                    static_cast<double>(value),
                    static_cast<unsigned>(bits),
                    static_cast<double>(expected),
                    static_cast<unsigned>(expected_bits),
                    bits == expected_bits ? "" : "  MISMATCH");
            }
        }
    }

    memcpy(
        C,
        result_bytes.data(),
        resultBytes);

    return 0;
}

int fpga_batch_matmul_tiled_auto(int batch, int M, int K, int N,
                                  const float *A, const float *B, float *C) {
    for (int b = 0; b < batch; b++) {
        const float *Ab = A + (size_t)b * M * K;
        const float *Bb = B + (size_t)b * K * N;
        float *Cb = C + (size_t)b * M * N;
        int rc = fpga_matmul_tiled_auto(M, K, N, Ab, Bb, Cb);
        if (rc != 0) return rc;
    }
    return 0;
}

int fpga_vecmat_tiled_auto(int K, int N, const float *x, const float *A,
                            float *y) {
    // Treat x as a 1xK matrix; fpga_matmul_tiled_auto's zero-padding
    // handles M=1 not being a multiple of 4 the same way it handles
    // any other non-aligned dimension.
    return fpga_matmul_tiled_auto(1, K, N, x, A, y);
}

int fpga_matvec_tiled_auto(int M, int K, const float *A, const float *x,
                            float *y) {
    // Treat x as a Kx1 matrix; fpga_matmul_tiled_auto's zero-padding
    // handles N=1 not being a multiple of 4 the same way it handles
    // M=1 in fpga_vecmat_tiled_auto.
    return fpga_matmul_tiled_auto(M, K, 1, A, x, y);
}

int fpga_dot_tiled_auto(int K, const float *x, const float *y,
                         float *result) {
    return fpga_matmul_tiled_auto(1, K, 1, x, y, result);
}
