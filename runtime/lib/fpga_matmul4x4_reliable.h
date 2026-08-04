#ifndef FPGA_MATMUL4X4_RELIABLE_H
#define FPGA_MATMUL4X4_RELIABLE_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// Drop-in replacement for fpga_matmul4x4() with transport-level retry.
//
// Return codes:
//    0  success
//   -1  write failed after all retries
//   -2  read timed out after all retries
//   -4  FPGA_MATMUL_VERIFY was set and three sends produced three
//       mutually different responses
//
// Environment:
//   FPGA_MATMUL_RETRIES=<n>   attempts per transaction (default 3)
//   FPGA_MATMUL_VERIFY=1      send every request twice and compare the
//                             responses bitwise; on disagreement, take a
//                             third sample and use the 2-of-3 majority.
//                             Doubles transaction count. Use for
//                             verification sweeps, not production.
int fpga_matmul4x4_reliable(int fd,
                            const float A[16],
                            const float B[16],
                            const float C_init[16],
                            float C_out[16]);

typedef struct {
    unsigned long transactions;
    unsigned long resyncs;
    unsigned long retries;
    unsigned long transport_failures;
    unsigned long verify_mismatches;    // two sends disagreed
    unsigned long verify_recovered;     // 2-of-3 majority resolved it
    unsigned long verify_unrecovered;   // all three differed
} fpga_reliable_stats_t;

fpga_reliable_stats_t fpga_reliable_get_stats(void);
void fpga_reliable_reset_stats(void);
void fpga_reliable_print_stats(FILE *f);

#ifdef __cplusplus
}
#endif

#endif  // FPGA_MATMUL4X4_RELIABLE_H
