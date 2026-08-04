/* systolic_dispatch_sim.c
 *
 * Pure-software simulation backend for the transport-neutral dispatch
 * runtime interface (systolic_dispatch_open / systolic_dispatch_matmul4x4
 * -- Section design-offload, Section runtime-abstraction). Implements
 * exactly the same 4x4x4 multiply-accumulate contract the physical
 * accelerator exposes over UART (C_out = A @ B + C_init, all three
 * matrices row-major float32), but computes it directly in-process:
 * no serial port, no FPGA, no hardware dependency of any kind.
 *
 * This file provides the SAME two external symbol names as
 * systolic_dispatch_uart.c. The two files are mutually exclusive at
 * link time -- linking one or the other into the same MLIR-compiled
 * object file is the only difference between a build that drives the
 * physical board and one that runs entirely on the host CPU, with zero
 * changes to the generated tile loops, the MLIR pass, or the compiled
 * .o file itself.
 *
 * Build either backend by choosing which of these two .c files to link
 * against the same generated object file, e.g.:
 *   clang matmul.o systolic_dispatch_sim.c  -o matmul_sim
 *   clang matmul.o systolic_dispatch_uart.c fpga_matmul4x4.c \
 *       fpga_matmul_tiled.c -o matmul_uart
 */

int systolic_dispatch_open(void) {
    /* No real connection to open; return an arbitrary non-negative
     * placeholder handle so callers that check for a negative "failed
     * to open" value never see a false failure. */
    return 0;
}

int systolic_dispatch_matmul4x4(int handle, const float A[16], const float B[16],
                                 const float C_init[16], float C_out[16]) {
    (void)handle;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            float acc = C_init[i * 4 + j];
            for (int k = 0; k < 4; k++) {
                acc += A[i * 4 + k] * B[k * 4 + j];
            }
            C_out[i * 4 + j] = acc;
        }
    }
    return 0;
}
