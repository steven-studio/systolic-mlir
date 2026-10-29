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
 * Physical DPTI descriptor transport.
 *
 * Persistent sessions are used when submitting multiple jobs that
 * share one compiler-generated scheduling timeline.
 * ============================================================ */

typedef struct DptiFtdiSession DptiFtdiSession;

DptiFtdiSession *dpti_ftdi_open(void);

/*
 * Write one descriptor, CONTROL = submit last. Returns 0, -1 no session,
 * -2 no job, -3 invalid job, or -6 when a WRITE32 did not go out (the rest
 * of the descriptor, the submit included, is then not sent).
 *
 * Phase T1 (2026-10-29), descriptor coalescing: with
 * SYSTOLIC_DPTI_COALESCE_DESCRIPTORS=1 in the environment when the session
 * is opened, the 14 WRITE32 packets of a descriptor (6 bytes each: opcode
 * 0x01, register, 32-bit value little-endian) are built into one 84-byte
 * buffer and handed to the transport in ONE write call instead of 14. The
 * bytes, their order and therefore the register writes the FPGA decodes
 * are identical: dpti_submit() emits the same (register, value) sequence
 * through the same packet encoder either way, and the batch is the
 * concatenation B_0 || B_1 || ... || B_13 of the packets it would have
 * written one by one. Only the host's write-call boundaries change. Off by
 * default (the per-packet path stays the reference). A failed batch write
 * is -6 like a failed WRITE32: nothing is retried, the session's caller
 * marks the link failed; the device may hold a prefix of the register
 * writes, which cannot submit a job because CONTROL is the last packet.
 * The MEM_WRITE path is not touched.
 */
int dpti_ftdi_submit_session(
    DptiFtdiSession *session,
    const dpti_job_t *job);

/* Bytes of one descriptor on the wire: 14 WRITE32 packets of 6 bytes. */
#define DPTI_DESCRIPTOR_WRITES 14u
#define DPTI_DESCRIPTOR_BYTES  (DPTI_DESCRIPTOR_WRITES * 6u)

/* 1 when this session coalesces descriptors (see above), else 0. */
int dpti_ftdi_descriptor_coalescing(
    const DptiFtdiSession *session);

/*
 * Stage one complete operand slab into FPGA DDR.
 *
 * The RTL's MEM_WRITE accepts exactly the payload length its build fixed --
 * the deepest geometry's slab, ceil(K_MAX / 8) * 512 bytes (1024 at
 * K_MAX_8X8 = K_MAX_4X4 = 16); length must be a positive multiple of 16:
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

/*
 * Read whatever the FPGA has sent so far, at most max_length bytes, with
 * one call into libftdi: the asynchronous runtime's receive side. Returns
 * the number of bytes stored (0 when nothing was pending), or -1 no session,
 * -2 bad arguments, -3 the read failed.
 *
 * On the FT2232H "nothing pending" costs one latency-timer period (the chip
 * answers the USB IN request only when it has data or the timer expires,
 * SYSTOLIC_DPTI_LATENCY_MS), so the caller reads when it expects something,
 * not in a tight loop. Against the RTL co-simulation it returns at once.
 */
int dpti_ftdi_read_some_session(
    DptiFtdiSession *session,
    void *data,
    uint32_t max_length);

void dpti_ftdi_close(
    DptiFtdiSession *session);

/*
 * Bytes written to / read from the FT2232H on this session so far.
 */
unsigned long long dpti_ftdi_bytes_written(
    const DptiFtdiSession *session);

/*
 * Transport write calls (ftdi_write_data) made on this session so far,
 * failed ones included: 16 per invocation on the per-packet path (2 for
 * the MEM_WRITE, 14 for the descriptor), 3 with descriptor coalescing.
 */
unsigned long long dpti_ftdi_write_calls(
    const DptiFtdiSession *session);

unsigned long long dpti_ftdi_bytes_read(
    const DptiFtdiSession *session);

#ifdef __cplusplus
}
#endif

#endif
