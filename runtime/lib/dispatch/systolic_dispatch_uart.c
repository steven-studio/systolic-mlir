/* systolic_dispatch_uart_new.c -- UART backend for systolic_dispatch_new.h.
 *
 * Deliberately does NOT route through fpga_get_uart_fd(). That function
 * hardcodes /dev/ttyUSB1 and a baud rate chosen for the old 4x4 bitstream,
 * and it is shared with fpga_matmul_tiled_auto()'s separate tiling path.
 * Inheriting it would mean this file silently opens the wrong port at the
 * wrong speed, and fixing it would perturb a code path that is not being
 * touched. A private cached fd is cheaper and keeps the blast radius at
 * this file.
 */

#include "systolic_dispatch_new.h"
#include "fpga_matmul_rk_new.h"
#include "fpga_matmul4x4.h"   /* fpga_uart_open_baud */

#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_PORT "/dev/ttyUSB2"
#define DEFAULT_BAUD 2000000

/* Both array instances share one UART, so alternating between them buys
 * nothing while the link is the bottleneck -- measured at 1.0x. Fixed at 0
 * rather than exposed, so that a future reader does not mistake the second
 * instance for idle throughput. */
#define DEVICE 0

static int cached_fd = -1;

int systolic_dispatch_open(void)
{
    if (cached_fd >= 0)
        return cached_fd;

    const char *port = getenv("SYSTOLIC_PORT");

    if (port == NULL || port[0] == '\0')
        port = DEFAULT_PORT;

    int baud = DEFAULT_BAUD;

    const char *baud_env = getenv("SYSTOLIC_BAUD");

    if (baud_env != NULL && baud_env[0] != '\0') {
        const int parsed = atoi(baud_env);

        /* Reject junk rather than quietly falling back: a typo'd baud that
         * silently became 2000000 would look like it worked, and a typo'd
         * baud that silently became 0 would fail somewhere far away. */
        if (parsed > 0)
            baud = parsed;
        else
            fprintf(stderr,
                    "systolic: ignoring SYSTOLIC_BAUD=\"%s\" (not a positive "
                    "integer), using %d\n", baud_env, baud);
    }

    cached_fd = fpga_uart_open_baud(port, baud);

    if (cached_fd < 0)
        fprintf(stderr,
                "systolic: cannot open %s at %d baud. Set SYSTOLIC_PORT / "
                "SYSTOLIC_BAUD if the board moved.\n", port, baud);

    return cached_fd;
}

int systolic_dispatch_matmul(int handle, int K,
                             const float *A, const float *B,
                             const float *C_init, float *C_out)
{
    return sys_matmul(handle, DEVICE, K, A, B, C_init, C_out);
}
