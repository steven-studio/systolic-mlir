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
// devices 1..3 the 4x4s (M, N <= 4), and 1 <= K <= 16. The tile is
// zero-padded to the array and to depth 16, staged as one 1024-byte slab,
// submitted as one DPTI descriptor, and the array's result tile is read
// back and added into C -- so invocations chained through C accumulate.
//
// Returns 0, or:
//   -10  K outside 1..16          -11  device_id unknown / tile too large
//   -12  null pointer             -13  slab packing failed
//   -14  an earlier MEM_WRITE / descriptor / read failed on this DPTI
//        session; nothing more is sent on it until fpga_runtime_close()
//    -3  DPTI session could not be opened
//   other negative values         DPTI MEM_WRITE / submit / read failure
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
} fpga_runtime_stats_t;

void fpga_runtime_get_stats(fpga_runtime_stats_t *out);
void fpga_runtime_reset_stats(void);

// Close the DPTI session the scheduled entry point opened on first use.
void fpga_runtime_close(void);


#ifdef __cplusplus
}  // extern "C"
#endif

#endif
