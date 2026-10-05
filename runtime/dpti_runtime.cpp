#include "dpti_runtime.h"
#include "dpti_fpga_transport.h"

#include "fpga_matmul4x4.h"
#include "fpga_matmul_tiled.h"


/*
 * Physical FPGA DPTI transport.
 */
extern "C" int dpti_fpga_submit(
    DptiDevice *dev,
    const DptiJob *job,
    const float *A,
    const float *B,
    float *C)
{
    if (!dev || !job || !A || !B || !C)
        return -1;

    if (dev->fd < 0)
        return -4;

    /*
     * DPTI -> current FPGA UART transport.
     *
     * This is intentionally the physical transport boundary:
     * DPTI does not call the legacy 4x4 UART protocol anymore.
     */
    return dpti_fpga_transport_matmul(
        dev->fd,
        static_cast<int>(job->m),
        static_cast<int>(job->k),
        static_cast<int>(job->n),
        A,
        B,
        C);
}

namespace {

int validate_job(const DptiJob *job)
{
    if (!job)
        return -1;

    if (job->m == 0 || job->n == 0 || job->k == 0)
        return -2;

    /*
     * Current DPTI bring-up uses physical accelerator device 0.
     * Keep this explicit instead of silently accepting an unsupported
     * scheduler device ID.
     */
    if (job->device_id != 0)
        return -3;

    return 0;
}

} // namespace

extern "C" int dpti_device_open(
    DptiDevice *dev,
    const char *port)
{
    if (!dev || !port)
        return -1;

    int fd = fpga_uart_open(port);

    if (fd < 0)
        return -2;

    dev->fd = fd;
    dev->device_id = 0;

    return 0;
}

extern "C" void dpti_device_close(DptiDevice *dev)
{
    if (!dev)
        return;

    if (dev->fd >= 0)
        fpga_uart_close(dev->fd);

    dev->fd = -1;
}

static int dpti_physical_submit(
    DptiDevice *dev,
    const DptiJob *job)
{
    if (!dev || !job)
        return -1;

    int rc = validate_job(job);

    if (rc != 0)
        return rc;

    if (dev->fd < 0)
        return -4;

    /*
     * Descriptor-level DPTI entry point.
     *
     * The current physical UART path still uses the existing validated
     * matmul transaction protocol. The job descriptor is therefore
     * validated here, while the actual descriptor-to-RTL wire protocol
     * is added in the next integration step.
     */
    return 0;
}

extern "C" int dpti_execute_matmul(
    DptiDevice *dev,
    const DptiJob *job,
    const float *A,
    const float *B,
    float *C)
{
    if (!dev || !job || !A || !B || !C)
        return -1;

    if (validate_job(job) != 0)
        return -2;

    /*
     * DPTI FPGA entry point.
     *
     * Keep this separate from the legacy UART/tiled API.
     * The DptiJob descriptor is the runtime boundary that carries
     * device selection, shape, schedule, and memory addresses.
     */
    return dpti_fpga_submit(
        dev,
        job,
        A,
        B,
        C);
}
