// REQUIRES: cfront-clang
// Same-line sibling sites, an outward-moving anchor, mixed CPU/device work,
// and source executions distinct from completed runtime tiles. Hardware is
// MOCKED here; these tests do not establish FPGA hardware correctness.
// RUN: %systolic-cc --fp-reassociation --verify %s -o %t > %t.compile 2>&1
// RUN: %python %S/Inputs/site_identity.py metadata %t.systolic
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t > %t.mock 2>%t.verify
// RUN: %python %S/Inputs/site_identity.py run %t.mock hardware
// RUN: FileCheck %s --check-prefix=VERIFY --input-file=%t.verify
// VERIFY-COUNT-2: verified:
// VERIFY-NOT: MISMATCH
// RUN: %t > %t.soft 2>/dev/null
// RUN: %python %S/Inputs/site_identity.py run %t.soft software
// A late failure after some tiles wrote C must restore the whole function.
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 SOFT_RUNTIME_FAULT=fail SOFT_RUNTIME_FAULT_AT=6 %t > %t.fault 2>/dev/null
// RUN: %python %S/Inputs/site_identity.py run %t.fault fallback
// RUN: %systolic-cc %s -o %t.strict > %t.strict.compile 2>&1
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t.strict > %t.strict.out
// RUN: %python %S/Inputs/site_identity.py run %t.strict.out strict
// A macro can expand two different nests at exactly the same location.
// It must fail closed, rather than mark both sites or overwrite job counts.
// RUN: not %systolic-cc --fp-reassociation -DMACRO_COLLISION %s -o %t.macro 2>&1 | FileCheck %s --check-prefix=MACRO
// MACRO: source site mapping at {{[0-9]+}}:{{[0-9]+}} is ambiguous (2 candidates)
// Even a unique macro nest has no safely rewritable CPU fallback range.
// RUN: not %systolic-cc --fp-reassociation -DMACRO_COLLISION -DSINGLE_MACRO %s -o %t.single 2>&1 | FileCheck %s --check-prefix=SINGLE
// SINGLE: CPU fallback reporting cannot be instrumented safely

#define MIN(a,b) ((a)<(b)?(a):(b))
#ifdef MACRO_COLLISION
#ifdef SINGLE_MACRO
#define TWO { for(int i=0;i<16;++i) for(int j=0;j<5;++j) for(int k=0;k<6;++k) C[i][j]+=A[i][k]*B[k][j]; }
#else
#define TWO { for(int i=0;i<16;++i) for(int j=0;j<5;++j) for(int k=0;k<6;++k) C[i][j]+=A[i][k]*B[k][j]; for(int x=0;x<4;++x) for(int y=0;y<5;++y) for(int z=0;z<6;++z) D[x][y]+=A[x][z]*B[z][y]; }
#endif
void collision(float A[restrict 16][6],float B[restrict 6][5],float C[restrict 16][5],float D[restrict 16][5]) { TWO }
#else
#include <stdio.h>
#include <string.h>
#include "systolic_offload.h"
void same_line(float A[restrict 16][6],float B[restrict 6][5],float C[restrict 16][5],float D[restrict 16][5]) { for(int t=0;t<16;++t) { for(int i=t;i<MIN(t+1,16);++i) for(int j=0;j<5;++j) for(int k=0;k<6;++k) C[i][j]+=A[i][k]*B[k][j]; } for(int x=0;x<4;++x) for(int y=0;y<5;++y) for(int z=0;z<6;++z) D[x][y]+=A[x][z]*B[z][y]; }
void mixed(float A[restrict 16][6],float B[restrict 6][5],float C[restrict 16][5],float X[16][6],float Y[6][5],float Z[16][5]) { for(int t=0;t<16;++t) for(int i=t;i<MIN(t+1,16);++i) for(int j=0;j<5;++j) for(int k=0;k<6;++k) C[i][j]+=A[i][k]*B[k][j]; for(int x=0;x<4;++x) for(int y=0;y<5;++y) for(int z=0;z<6;++z) Z[x][y]+=X[x][z]*Y[z][y]; }
static float mul(float a,float b) { return a*b; }
static void reference(int rows,float A[16][6],float B[6][5],float C[16][5]) {
  for(int i=0;i<rows;++i) for(int j=0;j<5;++j) for(int k=0;k<6;++k) C[i][j]+=mul(A[i][k],B[k][j]);
}
static int exercise(int which) {
  float A[16][6],B[6][5],C[16][5],D[16][5],R[16][5],S[16][5];
  for(int i=0;i<16;++i) for(int k=0;k<6;++k) A[i][k]=(i+k)%5-2;
  for(int k=0;k<6;++k) for(int j=0;j<5;++j) B[k][j]=(k+j)%3-1;
  for(int i=0;i<16;++i) for(int j=0;j<5;++j) C[i][j]=D[i][j]=R[i][j]=S[i][j]=1;
  reference(16,A,B,R);reference(4,A,B,S);
  systolic_offload_stats_t before,after;
  systolic_offload_stats(&before);
  printf("BEGIN %s\n",which ? "mixed" : "same_line");
  if(which) mixed(A,B,C,A,B,D); else same_line(A,B,C,D);
  systolic_offload_stats(&after);
  unsigned long long jobs=0;
  for(int d=0;d<4;++d) jobs+=after.per_device[d]-before.per_device[d];
  int equal=memcmp(C,R,sizeof C)==0 && memcmp(D,S,sizeof D)==0;
  printf("END jobs=%llu failed=%llu equal=%d\n",jobs,after.failed-before.failed,equal);
  return !equal;
}
int main(void) { int bad=exercise(0);bad+=exercise(1);return bad; }
#endif
