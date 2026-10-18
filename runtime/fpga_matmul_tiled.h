#ifndef FPGA_MATMUL_TILED_H
#define FPGA_MATMUL_TILED_H

#ifdef __cplusplus
extern "C" {
#endif

// Scheduled FPGA entry point: one invocation of one physical array.
//
// --lower-systolic-matmul-tile-to-fpga emits one call per scheduled
// systolic.matmul_tile; device_id / start_cycle / est_cycles are that
// tile's device and schedule.
//
//   C[M][N] += A[M][K] x B[K][N]       all row-major fp32, contiguous
//
// on the 1x8x8 + 3x4x4 bitstream: device 0 is the 8x8 (M, N <= 8),
// devices 1..3 the 4x4s (M, N <= 4), and 1 <= K <= K_MAX of that device:
// K_MAX_8X8 for device 0, K_MAX_4X4 for devices 1..3 (see
// fpga_runtime_get_config). The tile is staged as one operand slab
// (fpga_runtime_config_t.slab_bytes, the bitstream's fixed MEM_WRITE length;
// zero past the tile), submitted as one DPTI descriptor whose k is K -- the
// array runs exactly K reduction steps; K_MAX is the capacity, not the depth
// of every invocation -- and the array's result tile is read back and added
// into C, so invocations chained through C accumulate.
//
// Returns 0, or, with nothing sent on the link:
//   -10  K outside 1..K_MAX of device_id
//   -11  device_id unknown / tile too large
//   -12  null pointer             -13  slab packing failed
//   -15  est_cycles is 0 (a descriptor needs a positive estimate)
//   -16  the bitstream configuration (SYSTOLIC_FPGA_CONFIG) is invalid
// or a link failure, after which nothing more is attempted on the session
// until fpga_runtime_close():
//    -3  the DPTI session could not be opened
//    -4  the operand slab's MEM_WRITE failed
//    -5  the descriptor was not accepted for submission
//    -6  a descriptor WRITE32 failed; the rest of it, CONTROL = submit
//        included, was not sent
//    -7  reading the result tile failed
//    -8  the result tile did not arrive within SYSTOLIC_DPTI_TIMEOUT_MS
//    -9  the completion record did not match this invocation (another
//        job_id or device, or the bitstream refused the descriptor): the
//        link is out of step with the host, nothing more is sent
//   -14  refused: an earlier call on this session failed with -3 .. -9
int fpga_matmul_tiled_auto_scheduled(
    int M, int K, int N,
    const float *A, const float *B, float *C,
    unsigned device_id,
    unsigned start_cycle,
    unsigned est_cycles);

// What the scheduled entry point has done since the last reset (or since
// the program started). The bytes are counted at the FTDI layer, so they
// are what crossed the wire, including the bytes of calls that failed.
typedef struct {
    unsigned long long invocations;                /* results received */
    unsigned long long invocations_per_device[4];  /* by physical device id */
    unsigned long long bytes_to_fpga;              /* every byte written */
    unsigned long long bytes_from_fpga;            /* every byte read */
    unsigned long long failed_calls;
    int                first_error;                /* 0 = none */

    /* Host wall-clock of the successful invocations, in nanoseconds
     * (std::chrono::steady_clock), from the first byte of the MEM_WRITE to
     * the last byte of the result tile. The four phases add up to ns_total:
     * the MEM_WRITE itself, the settle sleep after it (SYSTOLIC_DPTI_SETTLE_US,
     * 1000 by default), the descriptor's 14 WRITE32 and the result read. */
    unsigned long long ns_total;
    unsigned long long ns_min;                     /* over invocations; 0 if none */
    unsigned long long ns_max;
    unsigned long long ns_mem_write;
    unsigned long long ns_settle;
    unsigned long long ns_submit;
    unsigned long long ns_read;
} fpga_runtime_stats_t;

void fpga_runtime_get_stats(fpga_runtime_stats_t *out);
void fpga_runtime_reset_stats(void);

// The largest K_MAX the DPTI/DMA protocol can carry. One invocation's
// operand slab is ceil(K_MAX / 8) windows of 8 reduction steps, each window
// 32 beats of 16 bytes, and the RTL's DMA descriptor counts beats in 16 bits
// (rtl/multi_200t/tile_pipelined/dma/systolic_accel_context.sv), so
// ceil(K_MAX / 8) * 32 <= 65535, i.e. K_MAX <= 16376.
#define FPGA_K_MAX_LIMIT 16376u

// The reduction capacity of the bitstream on the board, per geometry. The
// two values are independent synthesis parameters (K_MAX_8X8, K_MAX_4X4)
// and any 1..FPGA_K_MAX_LIMIT; neither has to be a power of two.
//
// Read once, on first use, from the file named by SYSTOLIC_FPGA_CONFIG --
// the fpga_config.txt the bitstream build (dma_top_build.tcl) or the RTL
// co-simulation build (test/e2e/cosim/build.sh) writes, KEY=VALUE lines:
//   K_MAX_8X8=<n>  K_MAX_4X4=<n>       required
//   N=8  NUM_8X8=1  NUM_4X4=3          optional; checked against this
//                                      runtime's device map if present
//   DDR_BYTES=<n>                      optional: bytes of DDR behind the
//                                      bitstream (the co-simulation's model
//                                      records 1048576); the asynchronous
//                                      runtime fits its per-device slots in
//                                      it. Absent: the board, not bounded
//   PIPELINE_DEPTH=<n>                 optional, 1..64: invocations one
//                                      accelerator context holds at a time
//                                      (its job queue). Absent = 1: the
//                                      archived bitstream, which takes a
//                                      context's next job only once the
//                                      previous result is written back.
//                                      Read by the compiler (systolic-matmul
//                                      declares the fleet's pipelining from
//                                      it) and by the asynchronous runtime
//                                      (fpga_async_runtime.h: descriptors in
//                                      flight per device); the scheduled
//                                      entry point below submits one
//                                      invocation at a time either way.
// Without SYSTOLIC_FPGA_CONFIG it is the archived bitstream's
// (artifacts/fpga/tile_pipelined/1x8x8-3x4x4): K_MAX_8X8 = K_MAX_4X4 = 16,
// PIPELINE_DEPTH = 1.
typedef struct {
    unsigned k_max_8x8;     /* device 0 */
    unsigned k_max_4x4;     /* devices 1..3 */
    unsigned slab_bytes;    /* one MEM_WRITE: max over geometries of
                               ceil(K_MAX / 8) * 8 depths * 64 bytes */
    unsigned pipeline_depth;/* invocations a context holds at a time; 1 =
                               serialized (the archived bitstream) */
    unsigned ddr_bytes;     /* DDR_BYTES: the DDR the bitstream can address,
                               when recorded (the co-simulation's model); 0 =
                               not recorded (the board's 512 MiB) */
    char     source[512];   /* the file, or the built-in default */
    char     error[512];    /* why the configuration is invalid, or "" */
    /* COMPLETION_HEADER=1 in the file: every result the bitstream sends
     * back starts with a 16-byte completion record (magic, job_id,
     * device_id, status, payload bytes, seq) -- bitstreams built from
     * dma_top_build.tcl after 2026-10-14. 0 (the default, and every older
     * bitstream): bare tiles, matched by order. */
    unsigned completion_header;
} fpga_runtime_config_t;

/* The completion record, as the host reads it off the link (little-endian
 * fields; FPGA_COMPLETION_BYTES on the wire). */
#define FPGA_COMPLETION_BYTES 16u
#define FPGA_COMPLETION_MAGIC 0x59534A43u   /* "CJSY" on the wire */
#define FPGA_COMPLETION_OK      0u
#define FPGA_COMPLETION_BAD_K   1u          /* k outside 1..K_MAX of the device */
#define FPGA_COMPLETION_BAD_DEV 2u          /* no such device */
typedef struct {
    unsigned magic;
    unsigned job_id;
    unsigned device_id;
    unsigned status;
    unsigned payload_bytes;
    unsigned seq;
} fpga_completion_t;

// 0, or -16 with out->error saying why (every invocation then fails with
// -16 before anything is sent).
int fpga_runtime_get_config(fpga_runtime_config_t *out);

// Read one completion record from the open session (bitstreams with
// COMPLETION_HEADER=1). 0, or -7 read failed, -8 timed out, -9 no magic
// where one was expected. The dispatch tests use it; the scheduled entry
// point reads its own, and the asynchronous runtime (fpga_async_runtime.h)
// parses records out of its own receive buffer.
int fpga_runtime_read_completion(fpga_completion_t *out);

// The device table and the slab format, for the asynchronous runtime, which
// stages the same slabs for the same arrays through its own session.
//
// The array of a physical device: 8 for device 0, 4 for devices 1..3,
// 0 for anything else.
unsigned fpga_runtime_device_geometry(unsigned device_id);

// K_MAX of a physical device under this configuration; 0 for an unknown one.
unsigned fpga_runtime_device_k_max(const fpga_runtime_config_t *cfg,
                                   unsigned device_id);

// Pack one invocation into an operand slab of slab_bytes bytes (the
// bitstream's fixed MEM_WRITE length, fpga_runtime_config_t.slab_bytes):
// the m x k A and k x n B tiles, row-major, in the lane image
// dma_operand_writer decodes, zero elsewhere. P is the array (8 or 4),
// k_max its capacity. 0, or -12 null pointer, -10 K outside 1..k_max,
// -11 tile outside 1..P, -13 the slab is too small for K.
int fpga_runtime_pack_operand_slab(int M, int K, int N, int P, int k_max,
                                   const float *A, const float *B,
                                   unsigned char *slab, unsigned slab_bytes);

// Close the DPTI session the scheduled entry point opened on first use.
void fpga_runtime_close(void);


#ifdef __cplusplus
}  // extern "C"
#endif

#endif
