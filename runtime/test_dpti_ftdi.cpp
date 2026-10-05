#include "dpti_runtime.h"

#include <cstdio>
#include <cstdint>

int main()
{
    dpti_job_t job = {
        .job_id      = 1,
        .device_id   = 0,

        .m           = 8,
        .n           = 8,
        .k           = 16,

        .start_cycle = 100,
        .est_cycles  = 125,

        .a_base      = 0x0000000000001000ULL,
        .b_base      = 0x0000000000002000ULL,
        .c_base      = 0x0000000000003000ULL,
    };

    std::printf(
        "============================================================\n"
        " DPTI FTDI Physical Transport Test\n"
        "============================================================\n"
        "\n"
        " job_id       = %u\n"
        " device_id    = %u\n"
        " shape        = %ux%ux%u\n"
        " start_cycle  = %u\n"
        " est_cycles   = %u\n"
        " A_BASE       = 0x%016llx\n"
        " B_BASE       = 0x%016llx\n"
        " C_BASE       = 0x%016llx\n"
        "\n",
        job.job_id,
        job.device_id,
        job.m,
        job.k,
        job.n,
        job.start_cycle,
        job.est_cycles,
        static_cast<unsigned long long>(job.a_base),
        static_cast<unsigned long long>(job.b_base),
        static_cast<unsigned long long>(job.c_base));

    std::printf("Submitting descriptor through FTDI...\n");

    const int rc = dpti_ftdi_submit(&job);

    if (rc != 0) {
        std::fprintf(
            stderr,
            "\nDPTI FTDI: FAIL rc=%d\n",
            rc);
        return 1;
    }

    std::printf(
        "\nDPTI FTDI: WRITE32 transport completed\n");

    return 0;
}
