// REQUIRES: cfront-clang
// The first product is only an identity candidate. The later reduction at
// the SAME original loop must retain CPU reporting, and the two sibling
// j loops must never be aliases of that common source site.
// RUN: %cfront --gemm-sites %t.json %s -o %t.mlir
// RUN: %python %S/../SystolicCC/Inputs/site_identity.py front %t.json
void shared_site(float A[restrict 4][4],float B[restrict 4][4],float C[restrict 4][4]) {
  for(int repeat=0;repeat<2;++repeat)
    for(int i=0;i<4;++i) {
      for(int j=0;j<4;++j) C[i][j]=A[i][0]*B[0][j];
      for(int j=0;j<4;++j) for(int k=0;k<4;++k) C[i][j]+=A[i][k]*B[k][j];
    }
}
