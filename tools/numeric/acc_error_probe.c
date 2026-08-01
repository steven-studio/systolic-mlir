// acc_error_probe.c — 隔離「累加誤差」與「量化誤差」兩個來源
//
// 對每個 K 跑三種算法，全部以 fp64 累加為 ground truth：
//   (A) fp32 輸入 + fp32 累加   -> 誤差應隨 K 成長 O(sqrt(K))~O(K)
//   (B) int8 輸入 + fp64 累加   -> 純量化誤差，應與 K 無關
//   (C) int8 輸入 + int32 累加  -> 若累加無誤差，應與 (B) 完全相同
//
// 若 (C) == (B) 且 (B) 不隨 K 成長，則「換定點可消解 Kdim 誤差」成立。
//
// build: gcc -O2 -o acc_error_probe acc_error_probe.c -lm

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

static double frand(void) { return (double)rand() / RAND_MAX * 2.0 - 1.0; }

// 對稱式 per-tensor 量化: scale = max|x| / 127
static double quant_scale(const float *v, int n) {
    double m = 0.0;
    for (int i = 0; i < n; i++) { double a = fabs((double)v[i]); if (a > m) m = a; }
    return (m == 0.0) ? 1.0 : m / 127.0;
}

static int8_t qclamp(double x) {
    long r = lround(x);
    if (r >  127) r =  127;
    if (r < -127) r = -127;
    return (int8_t)r;
}

typedef struct { double mean_rel, max_rel; } stat_t;

static stat_t stats(const double *ref, const double *got, int n) {
    stat_t s = {0.0, 0.0};
    for (int i = 0; i < n; i++) {
        double denom = fabs(ref[i]) > 1e-12 ? fabs(ref[i]) : 1e-12;
        double e = fabs(got[i] - ref[i]) / denom;
        s.mean_rel += e;
        if (e > s.max_rel) s.max_rel = e;
    }
    s.mean_rel /= n;
    return s;
}

static void run_one(int M, int N, int K) {
    float  *A  = malloc((size_t)M * K * sizeof(float));
    float  *B  = malloc((size_t)K * N * sizeof(float));
    int8_t *Aq = malloc((size_t)M * K);
    int8_t *Bq = malloc((size_t)K * N);
    double *ref = malloc((size_t)M * N * sizeof(double));  // fp64 ground truth
    double *cA  = malloc((size_t)M * N * sizeof(double));  // fp32 accum
    double *cB  = malloc((size_t)M * N * sizeof(double));  // int8 in, fp64 accum
    double *cC  = malloc((size_t)M * N * sizeof(double));  // int8 in, int32 accum

    for (long i = 0; i < (long)M * K; i++) A[i] = (float)frand();
    for (long i = 0; i < (long)K * N; i++) B[i] = (float)frand();

    double sa = quant_scale(A, M * K), sb = quant_scale(B, K * N);
    for (long i = 0; i < (long)M * K; i++) Aq[i] = qclamp((double)A[i] / sa);
    for (long i = 0; i < (long)K * N; i++) Bq[i] = qclamp((double)B[i] / sb);

    int32_t acc_max = 0;  // 追蹤 int32 累加器峰值，確認未溢位

    for (int m = 0; m < M; m++) {
        for (int n = 0; n < N; n++) {
            double  a64 = 0.0;   // ground truth
            float   a32 = 0.0f;  // fp32 累加
            double  aq64 = 0.0;  // 量化後但用 fp64 累加
            int32_t a32i = 0;    // 量化後 int32 累加

            for (int k = 0; k < K; k++) {
                float  x = A[(long)m * K + k],  y = B[(long)k * N + n];
                int8_t xq = Aq[(long)m * K + k], yq = Bq[(long)k * N + n];

                a64  += (double)x * (double)y;
                a32  += x * y;                                    // 每次加法都捨入
                aq64 += ((double)xq * sa) * ((double)yq * sb);
                a32i += (int32_t)xq * (int32_t)yq;                // 整數，無捨入
            }
            if (labs((long)a32i) > acc_max) acc_max = (int32_t)labs((long)a32i);

            ref[m * N + n] = a64;
            cA [m * N + n] = (double)a32;
            cB [m * N + n] = aq64;
            cC [m * N + n] = (double)a32i * sa * sb;              // 一次性 dequant
        }
    }

    stat_t sA = stats(ref, cA, M * N);
    stat_t sB = stats(ref, cB, M * N);
    stat_t sC = stats(ref, cC, M * N);

    // (B) 與 (C) 的逐元素差異：驗證 int32 累加是否 bit-exact
    double bc_diff = 0.0;
    for (int i = 0; i < M * N; i++) {
        double d = fabs(cB[i] - cC[i]);
        if (d > bc_diff) bc_diff = d;
    }

    printf("%6d | %10.3e %10.3e | %10.3e %10.3e | %10.3e %10.3e | %9.2e | %10d %6.2f%%\n",
           K, sA.mean_rel, sA.max_rel, sB.mean_rel, sB.max_rel,
           sC.mean_rel, sC.max_rel, bc_diff,
           acc_max, 100.0 * acc_max / 2147483647.0);

    free(A); free(B); free(Aq); free(Bq);
    free(ref); free(cA); free(cB); free(cC);
}

int main(int argc, char **argv) {
    int M = 4, N = 4;
    if (argc >= 3) { M = atoi(argv[1]); N = atoi(argv[2]); }
    srand(12345);

    printf("M=%d N=%d, ground truth = fp64 accumulation\n\n", M, N);
    printf("     K | (A) fp32 in/fp32 acc  | (B) int8 in/fp64 acc  | (C) int8 in/int32 acc |  |B-C|max | int32 acc peak\n");
    printf("       |   mean_rel    max_rel |   mean_rel    max_rel |   mean_rel    max_rel |           |     value  of max\n");
    printf("-------+-----------------------+-----------------------+-----------------------+-----------+-------------------\n");

    int Ks[] = {4, 9, 16, 36, 64, 144, 256, 576, 1024, 2304, 4096, 9216};
    for (unsigned i = 0; i < sizeof(Ks) / sizeof(Ks[0]); i++) run_one(M, N, Ks[i]);
    return 0;
}
