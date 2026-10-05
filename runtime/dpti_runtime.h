#ifndef DPTI_RUNTIME_H
#define DPTI_RUNTIME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * DPTI AXI4-Lite register map
 * ============================================================ */

#define DPTI_REG_CONTROL      0x00
#define DPTI_REG_JOB_ID       0x04
#define DPTI_REG_DEVICE_ID    0x08
#define DPTI_REG_M            0x0C
#define DPTI_REG_N            0x10
#define DPTI_REG_K            0x14
#define DPTI_REG_START_CYCLE  0x18
#define DPTI_REG_EST_CYCLES   0x1C

#define DPTI_REG_A_BASE_LO    0x20
#define DPTI_REG_A_BASE_HI    0x24
#define DPTI_REG_B_BASE_LO    0x28
#define DPTI_REG_B_BASE_HI    0x2C
#define DPTI_REG_C_BASE_LO    0x30
#define DPTI_REG_C_BASE_HI    0x34

#define DPTI_CONTROL_SUBMIT   0x00000001u

/* ============================================================
 * Descriptor API
 * ============================================================ */

typedef struct {
    uint32_t job_id;
    uint32_t device_id;

    uint32_t m;
    uint32_t n;
    uint32_t k;

    uint32_t start_cycle;
    uint32_t est_cycles;

    uint64_t a_base;
    uint64_t b_base;
    uint64_t c_base;
} dpti_job_t;

typedef void (*dpti_write32_fn)(
    void *ctx,
    uint32_t addr,
    uint32_t data);

int dpti_job_validate(const dpti_job_t *job);

int dpti_submit(
    const dpti_job_t *job,
    dpti_write32_fn write32,
    void *ctx);

/* ============================================================
 * Physical FPGA bring-up API
 *
 * This is the existing smoke-test path.
 * ============================================================ */

typedef struct {
    int fd;
    uint32_t device_id;
} DptiDevice;

typedef struct {
    uint32_t job_id;
    uint32_t device_id;

    uint32_t m;
    uint32_t n;
    uint32_t k;

    uint32_t start_cycle;
    uint32_t est_cycles;

    uint64_t a_base;
    uint64_t b_base;
    uint64_t c_base;
} DptiJob;

int dpti_device_open(
    DptiDevice *dev,
    const char *port);

void dpti_device_close(
    DptiDevice *dev);

int dpti_execute_matmul(
    DptiDevice *dev,
    const DptiJob *job,
    const float *A,
    const float *B,
    float *C);

/* ============================================================
 * Physical DPTI descriptor transport.
 *
 * Persistent sessions are used when submitting multiple jobs that
 * share one compiler-generated scheduling timeline.
 * ============================================================ */

typedef struct DptiFtdiSession DptiFtdiSession;

DptiFtdiSession *dpti_ftdi_open(void);

int dpti_ftdi_submit_session(
    DptiFtdiSession *session,
    const dpti_job_t *job);

/*
 * Stage one complete operand slab into FPGA DDR.
 *
 * Current RTL MEM_WRITE protocol accepts exactly 1024 payload bytes:
 *
 *   byte 0      = 0x02
 *   byte 1..4   = DDR byte address, little-endian
 *   byte 5..8   = payload length, little-endian
 *   byte 9..    = payload
 */
int dpti_ftdi_mem_write_session(
    DptiFtdiSession *session,
    uint32_t address,
    const void *data,
    uint32_t length);

/*
 * Read exactly length bytes returned by the FPGA on the persistent
 * physical session.
 *
 * timeout_ms is an overall host-side timeout.
 */
int dpti_ftdi_read_exact_session(
    DptiFtdiSession *session,
    void *data,
    uint32_t length,
    uint32_t timeout_ms);

void dpti_ftdi_close(
    DptiFtdiSession *session);

/*
 * Convenience single-job API retained for existing smoke tests.
 *
 * Equivalent to:
 *
 *   session = dpti_ftdi_open();
 *   dpti_ftdi_submit_session(session, job);
 *   dpti_ftdi_close(session);
 */
int dpti_ftdi_submit(
    const dpti_job_t *job);

#ifdef __cplusplus
}
#endif

#endif
