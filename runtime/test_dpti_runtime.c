#include "dpti_runtime.h"

#include <stdio.h>
#include <stdint.h>

static void mock_write32(void *ctx,
                         uint32_t addr,
                         uint32_t data)
{
    (void)ctx;

    printf("WRITE32 0x%02x 0x%08x\n",
           addr,
           data);
}

int main(void)
{
    dpti_job_t job = {
        .job_id = 1,
        .device_id = 0,

        .m = 8,
        .n = 8,
        .k = 16,

        .start_cycle = 100,
        .est_cycles = 125,

        .a_base = 0x0000000000001000ULL,
        .b_base = 0x0000000000002000ULL,
        .c_base = 0x0000000000003000ULL,
    };

    printf("============================================================\n");
    printf(" C -> DPTI runtime ABI test\n");
    printf("============================================================\n\n");

    if (dpti_submit(&job, mock_write32, NULL) != 0) {
        fprintf(stderr, "ERROR: dpti_submit failed\n");
        return 1;
    }

    printf("\nPASS: C -> DPTI register ABI\n");

    return 0;
}
