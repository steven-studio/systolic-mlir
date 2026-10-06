#ifndef DPTI_FPGA_TRANSPORT_H
#define DPTI_FPGA_TRANSPORT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Execute one DPTI matrix invocation through the current UART protocol.
 *
 * Current RTL protocol:
 *
 *   FRAME_START (4 bytes)
 *   k_dim       (4 bytes, little-endian)
 *   A/B payload (RX_BYTES bytes)
 *   FRAME_END   (4 bytes)
 *   result      (8*N*N bytes)
 *
 * The transport is currently parameterized for the deployed
 * N=8 / K_MAX=16 configuration.
 */
int dpti_fpga_transport_matmul(
    int fd,
    int M,
    int K,
    int N,
    const float *A,
    const float *B,
    float *C);

#ifdef __cplusplus
}
#endif

#endif
