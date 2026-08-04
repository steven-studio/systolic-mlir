// run_shape.c
//
// 通用測試驅動:讀 argv 給的 M K N,以及兩個 row-major float32 binary 檔
// (A.bin, B.bin),呼叫既有的 fpga_matmul_tiled_auto() 算出 C,
// 寫成 row-major float32 binary (C.bin)。
//
// 用法:
//   ./run_shape M K N A.bin B.bin C.bin

#include <stdio.h>
#include <stdlib.h>
#include "fpga_matmul_tiled.h"

static float *read_floats(const char *path, size_t count) {
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); exit(1); }
    float *buf = malloc(sizeof(float) * count);
    size_t got = fread(buf, sizeof(float), count, f);
    if (got != count) {
        fprintf(stderr, "%s: expected %zu floats, got %zu\n", path, count, got);
        exit(1);
    }
    fclose(f);
    return buf;
}

static void write_floats(const char *path, const float *buf, size_t count) {
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); exit(1); }
    fwrite(buf, sizeof(float), count, f);
    fclose(f);
}

int main(int argc, char **argv) {
    if (argc != 7) {
        fprintf(stderr, "usage: %s M K N A.bin B.bin C.bin\n", argv[0]);
        return 1;
    }
    int M = atoi(argv[1]);
    int K = atoi(argv[2]);
    int N = atoi(argv[3]);
    const char *a_path = argv[4];
    const char *b_path = argv[5];
    const char *c_path = argv[6];

    float *A = read_floats(a_path, (size_t)M * K);
    float *B = read_floats(b_path, (size_t)K * N);
    float *C = malloc(sizeof(float) * (size_t)M * N);

    int rc = fpga_matmul_tiled_auto(M, K, N, A, B, C);
    if (rc != 0) {
        fprintf(stderr, "fpga_matmul_tiled_auto failed, rc=%d "
                        "(rc=-3 means /dev/ttyUSB1 open failed -- check the "
                        "board is plugged in and your user is in the "
                        "'dialout' group)\n", rc);
        return 2;
    }

    write_floats(c_path, C, (size_t)M * N);
    fprintf(stderr, "OK: %dx%dx%d -> %s\n", M, K, N, c_path);

    free(A);
    free(B);
    free(C);
    return 0;
}
