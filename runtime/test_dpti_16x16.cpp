#include "dpti_runtime.h"

#include <cstdio>
#include <cstdint>

static void make_job(
    dpti_job_t *job,
    uint32_t job_id,
    int tile_i,
    int tile_j)
{
    /*
     * Compiler-style decomposition:
     *
     *   C[16x16] = A[16x16] @ B[16x16]
     *
     * Output tiling:
     *
     *   4 x 4 = 16 logical output jobs
     *
     * Each job computes:
     *
     *   A[4x16] @ B[16x4] -> C[4x4]
     *
     * The K dimension is not split at the compiler level.
     */

    *job = {};

    job->job_id = job_id;

    /*
     * Current RTL prototype physical-device mapping:
     *
     *   device_id 0 -> 8x8 accelerator
     *   device_id 1 -> 4x4 accelerator
     */
    job->device_id = 1;

    job->m = 4;
    job->n = 4;
    job->k = 16;

    /*
     * Match the compiler-generated schedule:
     *
     *   est_cycles = 117
     *   start_cycle = job_id * 117
     */
    job->start_cycle = job_id * 117;
    job->est_cycles = 117;

    /*
     * Descriptor-address placeholders only.
     *
     * This test validates compiler-style descriptor submission and
     * scheduling through one persistent physical session.  Numerical
     * matrix payload execution is a separate integration step.
     */
    job->a_base =
        0x0000000000001000ULL +
        static_cast<uint64_t>(tile_i) * 0x1000;

    job->b_base =
        0x0000000000002000ULL +
        static_cast<uint64_t>(tile_j) * 0x1000;

    job->c_base =
        0x0000000000003000ULL +
        static_cast<uint64_t>(tile_i * 4 + tile_j) * 0x100;
}

int main()
{
    constexpr int M_TILES = 4;
    constexpr int N_TILES = 4;

    constexpr int EXPECTED_JOBS =
        M_TILES * N_TILES;

    std::printf(
        "============================================================\n"
        " DPTI Compiler-Style 16x16 Descriptor Test\n"
        "============================================================\n\n");

    std::printf(
        "logical GEMM : 16x16 @ 16x16 = 16x16\n"
        "logical jobs : %d\n"
        "job shape    : 4x16 @ 16x4 -> 4x4\n"
        "device_id    : 1\n"
        "est_cycles   : 117\n\n",
        EXPECTED_JOBS);

    DptiFtdiSession *session =
        dpti_ftdi_open();

    if (!session) {
        std::fprintf(
            stderr,
            "DPTI 16x16: failed to open persistent session\n");

        return 1;
    }

    int submitted = 0;

    for (int ti = 0; ti < M_TILES; ++ti) {
        for (int tj = 0; tj < N_TILES; ++tj) {

            dpti_job_t job{};

            make_job(
                &job,
                static_cast<uint32_t>(submitted),
                ti,
                tj);

            std::printf(
                "job=%2u  C=(%d,%d)"
                "  device=%u"
                "  shape=%ux%ux%u"
                "  start=%u"
                "  est=%u\n",
                job.job_id,
                ti,
                tj,
                job.device_id,
                job.m,
                job.k,
                job.n,
                job.start_cycle,
                job.est_cycles);

            const int rc =
                dpti_ftdi_submit_session(
                    session,
                    &job);

            if (rc != 0) {
                std::fprintf(
                    stderr,
                    "\nDPTI 16x16: submission failed"
                    " job=%u rc=%d\n",
                    job.job_id,
                    rc);

                dpti_ftdi_close(session);
                return 1;
            }

            ++submitted;
        }
    }

    dpti_ftdi_close(session);

    std::printf(
        "\n============================================================\n"
        " DPTI Compiler-Style 16x16 Descriptor Test: PASS\n"
        " submitted = %d / %d jobs\n"
        " final scheduled end cycle = %d\n"
        "============================================================\n",
        submitted,
        EXPECTED_JOBS,
        EXPECTED_JOBS * 117);

    return submitted == EXPECTED_JOBS ? 0 : 1;
}
