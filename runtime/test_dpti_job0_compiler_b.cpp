#include "dpti_runtime.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace {

constexpr uint32_t OPERAND_BASE = 0x00010000u;
constexpr uint32_t RESULT_BASE  = 0x00003000u;

constexpr uint32_t PAYLOAD_BYTES = 1024;
constexpr int JOB_K = 16;

static int payload_index_a(
    int k,
    int row)
{
    const int win  = k / 8;
    const int koff = k % 8;

    return
        (win << 7) |
        (row << 3) |
        koff;
}

static int payload_index_b(
    int k,
    int col)
{
    const int win  = k / 8;
    const int koff = k % 8;

    return
        (win  << 7) |
        (1    << 6) |
        (koff << 3) |
        col;
}

static void store_f32_word(
    std::array<unsigned char, PAYLOAD_BYTES>& slab,
    int word_index,
    float value)
{
    uint32_t bits = 0;

    static_assert(
        sizeof(bits) == sizeof(value),
        "float must be 32-bit");

    std::memcpy(
        &bits,
        &value,
        sizeof(bits));

    const std::size_t byte_index =
        static_cast<std::size_t>(word_index) * 4;

    slab[byte_index + 0] =
        static_cast<unsigned char>(bits);

    slab[byte_index + 1] =
        static_cast<unsigned char>(bits >> 8);

    slab[byte_index + 2] =
        static_cast<unsigned char>(bits >> 16);

    slab[byte_index + 3] =
        static_cast<unsigned char>(bits >> 24);
}

static void build_operand_slab(
    std::array<unsigned char, PAYLOAD_BYTES>& slab)
{
    slab.fill(0);

    /*
     * A = [I4 | 0].
     */
    for (int k = 0; k < JOB_K; ++k) {
        for (int row = 0; row < 4; ++row) {
            if (k < 4 && row == k) {
                store_f32_word(
                    slab,
                    payload_index_a(k, row),
                    1.0f);
            }
        }
    }

    /*
     * Match compiler-generated JOB0 exactly:
     *
     *   B[k,col] = 16*k + col
     *
     * Therefore the first four rows are:
     *
     *   [ 0,  1,  2,  3]
     *   [16, 17, 18, 19]
     *   [32, 33, 34, 35]
     *   [48, 49, 50, 51]
     */
    for (int k = 0; k < JOB_K; ++k) {
        for (int col = 0; col < 4; ++col) {
            store_f32_word(
                slab,
                payload_index_b(k, col),
                static_cast<float>(16 * k + col));
        }
    }
}

} // namespace

int main()
{
    std::printf(
        "============================================================\n"
        " DPTI JOB0 Numerical Smoke Test\n"
        "============================================================\n");

    std::array<unsigned char, PAYLOAD_BYTES> slab{};

    build_operand_slab(slab);

    /*
     * Host-side operand sanity check.
     * Verify the exact FP32 words that will be staged.
     */
    std::printf("\nHOST B[0,*] before staging:\n");

    for (int col = 0; col < 4; ++col) {
        const int wi = payload_index_b(0, col);
        const std::size_t b =
            static_cast<std::size_t>(wi) * 4;

        const uint32_t bits =
            static_cast<uint32_t>(slab[b + 0]) |
            (static_cast<uint32_t>(slab[b + 1]) << 8) |
            (static_cast<uint32_t>(slab[b + 2]) << 16) |
            (static_cast<uint32_t>(slab[b + 3]) << 24);

        float value = 0.0f;
        std::memcpy(&value, &bits, sizeof(value));

        std::printf(
            "B[0,%d] word=%d value=%g bits=0x%08x\n",
            col,
            wi,
            static_cast<double>(value),
            bits);
    }

    /*
     * Sanity checks matching the RTL testbench reference.
     */
    std::printf(
        "A[0,0] word index = %d\n",
        payload_index_a(0, 0));

    std::printf(
        "B[0,0] word index = %d\n",
        payload_index_b(0, 0));

    std::printf(
        "B[0,1] word index = %d\n",
        payload_index_b(0, 1));

    std::printf(
        "B[1,0] word index = %d\n",
        payload_index_b(1, 0));

    DptiFtdiSession *session =
        dpti_ftdi_open();

    if (!session) {
        std::fprintf(
            stderr,
            "ERROR: failed to open persistent session\n");

        return 1;
    }

    std::printf(
        "staging operand slab: addr=0x%08x bytes=%u\n",
        OPERAND_BASE,
        PAYLOAD_BYTES);

    const int mem_rc =
        dpti_ftdi_mem_write_session(
            session,
            OPERAND_BASE,
            slab.data(),
            PAYLOAD_BYTES);

    if (mem_rc != 0) {
        std::fprintf(
            stderr,
            "ERROR: MEM_WRITE failed rc=%d\n",
            mem_rc);

        dpti_ftdi_close(session);
        return 1;
    }

    dpti_job_t job{};

    job.job_id = 100;
    job.device_id = 1;

    job.m = 4;
    job.n = 4;
    job.k = JOB_K;

    /*
     * Keep these aligned with the compiler cost model even though
     * this smoke test contains only one job.
     */
    job.start_cycle = 0;
    job.est_cycles = 117;

    /*
     * Current RTL uses one combined A+B operand slab selected by
     * a_base. b_base is not used by the numerical operand fetch.
     */
    job.a_base = OPERAND_BASE;
    job.b_base = 0;
    job.c_base = RESULT_BASE;

    std::printf(
        "submitting JOB0: A=0x%08x C=0x%08x\n",
        OPERAND_BASE,
        RESULT_BASE);

    const int submit_rc =
        dpti_ftdi_submit_session(
            session,
            &job);

    if (submit_rc != 0) {
        std::fprintf(
            stderr,
            "ERROR: JOB0 submission failed rc=%d\n",
            submit_rc);

        dpti_ftdi_close(session);
        return 1;
    }

    /*
     * A 4x4 FP32 result is exactly:
     *
     *   16 floats x 4 bytes = 64 bytes.
     */
    std::array<unsigned char, 64> result_bytes{};

    std::printf(
        "waiting for JOB0 numerical result: 64 bytes\n");

    const int read_rc =
        dpti_ftdi_read_exact_session(
            session,
            result_bytes.data(),
            static_cast<uint32_t>(result_bytes.size()),
            5000);

    if (read_rc != 0) {
        std::fprintf(
            stderr,
            "ERROR: JOB0 result read failed rc=%d\n",
            read_rc);

        dpti_ftdi_close(session);
        return 1;
    }

    constexpr float expected_c[4][4] = {
        { 0.0f,  1.0f,  2.0f,  3.0f},
        {16.0f, 17.0f, 18.0f, 19.0f},
        {32.0f, 33.0f, 34.0f, 35.0f},
        {48.0f, 49.0f, 50.0f, 51.0f}
    };

    int passed = 0;

    std::printf("\nJOB0 C:\n");

    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            const int element =
                row * 4 + col;

            const std::size_t b =
                static_cast<std::size_t>(element) * 4;

            const uint32_t bits =
                static_cast<uint32_t>(result_bytes[b + 0]) |
                (static_cast<uint32_t>(result_bytes[b + 1]) << 8) |
                (static_cast<uint32_t>(result_bytes[b + 2]) << 16) |
                (static_cast<uint32_t>(result_bytes[b + 3]) << 24);

            float value = 0.0f;

            std::memcpy(
                &value,
                &bits,
                sizeof(value));

            const float expected =
                expected_c[row][col];

            const bool ok =
                (value == expected);

            if (ok)
                ++passed;

            uint32_t expected_bits = 0;
            std::memcpy(
                &expected_bits,
                &expected,
                sizeof(expected_bits));

            std::printf(
                "C[%d,%d] value=% .9g bits=0x%08x expected=% .9g expected_bits=0x%08x %s\n",
                row,
                col,
                static_cast<double>(value),
                static_cast<unsigned>(bits),
                static_cast<double>(expected),
                static_cast<unsigned>(expected_bits),
                ok ? "PASS" : "FAIL");
        }

        std::printf("\n");
    }

    dpti_ftdi_close(session);

    std::printf(
        "\nNumerical check: %d / 16 FP32 values correct\n",
        passed);

    if (passed != 16) {
        std::fprintf(
            stderr,
            "JOB0 NUMERICAL E2E: FAIL\n");

        return 1;
    }

    std::printf(
        "============================================================\n"
        " JOB0 NUMERICAL E2E: PASS\n"
        " 16 / 16 FP32 values correct\n"
        "============================================================\n");

    return 0;
}
