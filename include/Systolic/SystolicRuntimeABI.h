#ifndef SYSTOLIC_RUNTIME_ABI_H
#define SYSTOLIC_RUNTIME_ABI_H

#include <stdint.h>

#define SYSTOLIC_JOB_MAGIC   0x53594A42u
#define SYSTOLIC_JOB_VERSION 1u

typedef struct __attribute__((packed)) {
  uint32_t magic;
  uint32_t version;

  uint32_t job_id;
  uint32_t device_id;

  uint32_t m;
  uint32_t n;
  uint32_t k;
  uint32_t reserved;

  uint32_t start_cycle;
  uint32_t est_cycles;

  uint64_t a_base;
  uint64_t b_base;
  uint64_t c_base;
} SystolicJobDescriptor;

#ifdef __cplusplus
static_assert(sizeof(SystolicJobDescriptor) == 64,
              "SystolicJobDescriptor ABI must be exactly 64 bytes");
#else
_Static_assert(sizeof(SystolicJobDescriptor) == 64,
               "SystolicJobDescriptor ABI must be exactly 64 bytes");
#endif

#endif
