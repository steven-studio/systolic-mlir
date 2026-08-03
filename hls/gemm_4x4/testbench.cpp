// hls/gemm_4x4/testbench.cpp
//
// Sized entirely from design.h, so one testbench covers every point of
// the (R, C, K_DIM) sweep. The previous version hard-coded 4 everywhere
// AND declared a local named C, which -DC=<n> expanded into a literal --
// that is the "expected unqualified-id" the cosim build was hitting.
// Nothing here may be named R, C or K_DIM for the same reason.
//
// The reference accumulates over k in order, starting from the incoming
// Cinout value, because that is exactly what the array does: PE(i,j)
// performs its k-th MAC on beat i+j+k, so its accumulator sees the
// products in ascending k. Float addition is not associative, so any
// other order would produce a few ULP of difference and turn this into
// a tolerance test instead of an equality test. Fill and drain beats
// add 0.0f * 0.0f, which is exact and therefore invisible here.

#include "design.h"

#include <cstdio>

int main() {
    static float Amat[R][K_DIM];
    static float Bmat[K_DIM][C];
    static float Cmat[R][C];
    static float Cref[R][C];

    // Deterministic, and bounded independently of K_DIM: with K up to 64
    // an unbounded ramp would spread the partial sums far enough apart in
    // magnitude that rounding starts to matter.
    for (int i = 0; i < R; i++)
        for (int k = 0; k < K_DIM; k++)
            Amat[i][k] = ((i * 7 + k * 3) % 17) / 8.0f - 1.0f;

    for (int k = 0; k < K_DIM; k++)
        for (int j = 0; j < C; j++)
            Bmat[k][j] = ((k * 5 + j * 11) % 13) / 8.0f - 0.75f;

    for (int i = 0; i < R; i++)
        for (int j = 0; j < C; j++)
            Cmat[i][j] = ((i + j) % 5) / 4.0f;

    // Reference: same starting value, same k order as the accelerator.
    for (int i = 0; i < R; i++)
        for (int j = 0; j < C; j++) {
            float acc = Cmat[i][j];
            for (int k = 0; k < K_DIM; k++)
                acc += Amat[i][k] * Bmat[k][j];
            Cref[i][j] = acc;
        }

    matmul_4x4x4(Amat, Bmat, Cmat);

    int bad = 0;
    for (int i = 0; i < R; i++)
        for (int j = 0; j < C; j++) {
            // Exact: the orders match, so anything but equality is a real
            // difference, not rounding.
            if (Cmat[i][j] != Cref[i][j]) {
                if (bad < 10)
                    printf("Mismatch at (%d,%d): got %.9g, expected %.9g\n",
                           i, j, Cmat[i][j], Cref[i][j]);
                bad++;
            }
        }

    printf("R=%d C=%d K_DIM=%d TIME_STEPS=%d -> %d/%d mismatches\n",
           R, C, K_DIM, TIME_STEPS, bad, R * C);
    printf(bad == 0 ? "PASS\n" : "FAIL\n");
    return bad == 0 ? 0 : 1;
}
