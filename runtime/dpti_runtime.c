#include "dpti_runtime.h"

#include <stddef.h>

/* ============================================================
 * Validation
 * ============================================================ */

int dpti_job_validate(const dpti_job_t *job)
{
    if (job == NULL)
        return -1;

    if (job->m == 0 ||
        job->n == 0 ||
        job->k == 0)
        return -2;

    if (job->est_cycles == 0)
        return -3;

    return 0;
}

/* ============================================================
 * Submit
 * ============================================================ */

int dpti_submit(const dpti_job_t *job,
                dpti_write32_fn write32,
                void *ctx)
{
    if (dpti_job_validate(job) != 0)
        return -1;

    if (write32 == NULL)
        return -2;

    /*
     * Descriptor registers.
     *
     * IMPORTANT:
     * CONTROL/submit is deliberately written last.
     */

    write32(ctx, DPTI_REG_JOB_ID,
            job->job_id);

    write32(ctx, DPTI_REG_DEVICE_ID,
            job->device_id);

    write32(ctx, DPTI_REG_M,
            job->m);

    write32(ctx, DPTI_REG_N,
            job->n);

    write32(ctx, DPTI_REG_K,
            job->k);

    write32(ctx, DPTI_REG_START_CYCLE,
            job->start_cycle);

    write32(ctx, DPTI_REG_EST_CYCLES,
            job->est_cycles);

    write32(ctx, DPTI_REG_A_BASE_LO,
            (uint32_t)(job->a_base & 0xffffffffULL));

    write32(ctx, DPTI_REG_A_BASE_HI,
            (uint32_t)(job->a_base >> 32));

    write32(ctx, DPTI_REG_B_BASE_LO,
            (uint32_t)(job->b_base & 0xffffffffULL));

    write32(ctx, DPTI_REG_B_BASE_HI,
            (uint32_t)(job->b_base >> 32));

    write32(ctx, DPTI_REG_C_BASE_LO,
            (uint32_t)(job->c_base & 0xffffffffULL));

    write32(ctx, DPTI_REG_C_BASE_HI,
            (uint32_t)(job->c_base >> 32));

    /*
     * Submit must be the final register write.
     */
    write32(ctx,
            DPTI_REG_CONTROL,
            DPTI_CONTROL_SUBMIT);

    return 0;
}
