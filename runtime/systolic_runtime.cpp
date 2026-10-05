#include "systolic_runtime.h"
#include "dpti_runtime.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int systolic_runtime_submit(
    const SystolicJobDescriptor *job) {

  if (!job)
    return -EINVAL;

  if (job->magic != SYSTOLIC_JOB_MAGIC)
    return -EINVAL;

  if (job->version != SYSTOLIC_JOB_VERSION)
    return -EINVAL;

  /*
   * Compiler ABI -> DPTI descriptor.
   *
   * The compiler-generated descriptor is translated directly into
   * the canonical dpti_job_t ABI.
   */
  dpti_job_t dpti_job = {};

  dpti_job.job_id      = job->job_id;
  dpti_job.device_id   = job->device_id;

  dpti_job.m           = job->m;
  dpti_job.n           = job->n;
  dpti_job.k           = job->k;

  dpti_job.start_cycle = job->start_cycle;
  dpti_job.est_cycles  = job->est_cycles;

  dpti_job.a_base      = job->a_base;
  dpti_job.b_base      = job->b_base;
  dpti_job.c_base      = job->c_base;

  /*
   * The actual transport backend is provided by dpti_submit().
   *
   * Do not invent a second DptiDevice/DptiJob API here.
   */
  /*
   * Temporary descriptor bring-up backend.
   *
   * Validate the complete compiler -> DPTI register ABI before
   * connecting the physical transport.
   */
  return dpti_submit(
      &dpti_job,
      [](void *, uint32_t addr, uint32_t data) {
        fprintf(stderr,
                "[DPTI] write32 addr=0x%02x data=0x%08x\\n",
                addr, data);
      },
      nullptr);
}

int systolic_runtime_wait(uint32_t job_id) {
  fprintf(stderr,
          "[systolic-runtime] wait job=%u\n",
          job_id);

  /*
   * Completion transport will be connected after submit transport.
   */
  return 0;
}
