#ifndef SYSTOLIC_RUNTIME_H
#define SYSTOLIC_RUNTIME_H

#include "Systolic/SystolicRuntimeABI.h"

#ifdef __cplusplus
extern "C" {
#endif

int systolic_runtime_submit(
    const SystolicJobDescriptor *job);

int systolic_runtime_wait(
    uint32_t job_id);

#ifdef __cplusplus
}
#endif

#endif
