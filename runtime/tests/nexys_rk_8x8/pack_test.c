#include "fpga_matmul_rk_new.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    int K = atoi(argv[1]), dev = atoi(argv[2]);
    static float A[SYS_R * SYS_K_MAX], B[SYS_K_MAX * SYS_C];
    static float Cin[SYS_R * SYS_C], Cout[SYS_R * SYS_C];

    for (int i = 0; i < SYS_R; i++)
        for (int k = 0; k < K; k++)
            A[i * K + k] = (float)(100 * i + k);

    for (int k = 0; k < K; k++)
        for (int j = 0; j < SYS_C; j++)
            B[k * SYS_C + j] = -(float)(10 * k + j) - 0.5f;

    for (int i = 0; i < SYS_R; i++)
        for (int j = 0; j < SYS_C; j++)
            Cin[i * SYS_C + j] = (float)(1000 + 8 * i + j);

    return sys_matmul(-1, dev, K, A, B, Cin, Cout);
}
