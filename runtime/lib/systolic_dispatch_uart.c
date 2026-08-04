/* systolic_dispatch_uart.c
 *
 * UART backend for the transport-neutral dispatch runtime interface that
 * TileMatmulForFpgaPattern's generated IR calls against
 * (systolic_dispatch_open / systolic_dispatch_matmul4x4 --
 * Section design-offload, Section runtime-abstraction).
 *
 * This file is a thin wrapper: it delegates to the existing UART-specific
 * primitives (fpga_get_uart_fd, fpga_matmul4x4) already implemented in
 * fpga_matmul_tiled.c / fpga_matmul4x4.c, so the actual wire protocol
 * (Section runtime) is completely unchanged. Its only job is to expose
 * that protocol under the transport-neutral names the MLIR pass depends
 * on, so linking THIS file in (rather than systolic_dispatch_sim.c) is
 * the only thing that selects UART as the backend for a given build --
 * no MLIR IR, and no other part of the compiler, changes between builds.
 */

#include "fpga_matmul4x4.h"
#include "fpga_matmul_tiled.h"

int systolic_dispatch_open(void) {
    return fpga_get_uart_fd();
}

int systolic_dispatch_matmul4x4(int handle, const float A[16], const float B[16],
                                 const float C_init[16], float C_out[16]) {
    return fpga_matmul4x4(handle, A, B, C_init, C_out);
}
