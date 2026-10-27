// REQUIRES: cfront-clang
// RUN: %python %S/Inputs/general_affine_runner.py --source %s --clang %host-cc --out %t --cc %systolic-cc
// No mock hardware: the runner checks actual software runtime completions,
// independently expands emitted jobs, and compares all physical MAC addresses.
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#ifdef AUDIT_STATS
#include "systolic_offload.h"
#endif
#if CASE < 3
#define N 16
#else
#define N 64
#endif
static float A[N][N], B[N][N], C[N][N];
#if CASE == 1
void kernel(float C[restrict 16][N], float A[restrict 16][N], float B[restrict 16][N]) {
#ifdef REPEAT
    for (int p=0; p<2; ++p)
#endif
    for (int i=0; i<4; ++i)
        for (int j=0; j<4; ++j)
            for (int k=0; k<4; ++k)
        C[i+j-2*k+6][i-2*j+k+6] += A[i+j-2*k+6][i+j+k] * B[i+j+k][i-2*j+k+6];
}
#elif CASE == 2
void kernel(float C[restrict 16][N], float A[restrict 16][N], float B[restrict 16][N]) {
    for (int i=0; i<4; ++i)
        for (int j=0; j<4; ++j)
            for (int k=0; k<4; ++k)
        C[i][j] += A[2*i+k][i+2*k] * B[2*k+j][k+2*j];
}
#elif CASE == 3
void kernel(float C[restrict 64][N], float A[restrict 64][N], float B[restrict 64][N]) {
    for (int i=0; i<4; ++i)
        for (int j=0; j<4; ++j)
            for (int k=0; k<4; ++k)
                for (int l=0; l<4; ++l)
        C[i][j] += A[i+2*k+3*l][2*i+4*k+5*l] * B[5*k+6*l+j][3*k+2*l+2*j];
}
#elif CASE == 4
void kernel(float C[restrict 64][N], float A[restrict 64][N], float B[restrict 64][N]) {
    for (int i=0; i<4; ++i)
        for (int j=0; j<4; ++j)
            for (int k=0; k<4; ++k)
                for (int l=0; l<4; ++l)
        C[i+2*k+3*l][j+5*k+6*l] += A[i+3*k+5*l][2*i+3*l+6*k] * B[3*j+2*k+3*l][5*j+6*k+l];
}
#endif
int main(void) {
  for (int i=0; i<N; ++i) for (int j=0; j<N; ++j) {
    A[i][j]=(float)((i*3+j)%7-3);
    B[i][j]=(float)((i+j*5)%11-5);
    C[i][j]=(float)((i+2*j)%5-2);
  }
  kernel(C,A,B);
  for (int i=0; i<N; ++i) for (int j=0; j<N; ++j) {
    uint32_t b; memcpy(&b,&C[i][j],4); printf("%08x\n",b);
  }
#ifdef AUDIT_STATS
  systolic_offload_stats_t st; systolic_offload_stats(&st);
  unsigned long long total=0;
  for(int d=0; d<4; ++d) total+=st.per_device[d];
  fprintf(stderr,"AUDIT completed=%llu failed=%llu hardware=%d\n",total,st.failed,st.is_hardware);
#endif
  return 0;
}
