// The runtime calls fpga_matmul4x4(fd, A, B, C_acc, C_out) in the K-tile
// loop, i.e. with a NON-ZERO C_init. Verify that path.
#include <cstdio>
#include <cstdlib>
#include "design.h"
int main(){
  float A[4][4],B[4][4],Cbuf[4][4],Cref[4][4];
  srand(7);
  int bad=0;
  for(int trial=0;trial<200;trial++){
    for(int i=0;i<4;i++)for(int k=0;k<4;k++)A[i][k]=(float)((rand()%2001)-1000)/100.0f;
    for(int k=0;k<4;k++)for(int j=0;j<4;j++)B[k][j]=(float)((rand()%2001)-1000)/100.0f;
    for(int i=0;i<4;i++)for(int j=0;j<4;j++){
      float init=(float)((rand()%2001)-1000)/100.0f;
      Cbuf[i][j]=init; Cref[i][j]=init;
    }
    for(int i=0;i<4;i++)for(int j=0;j<4;j++){
      float s=Cref[i][j];
      for(int k=0;k<4;k++) s+=A[i][k]*B[k][j];
      Cref[i][j]=s;
    }
    matmul_4x4x4(A,B,Cbuf);
    for(int i=0;i<4;i++)for(int j=0;j<4;j++){
      float d=Cbuf[i][j]-Cref[i][j]; if(d<0)d=-d;
      float mag=Cref[i][j]<0?-Cref[i][j]:Cref[i][j];
      if(d > 1e-3f*(mag>1.0f?mag:1.0f)){ bad++; if(bad<4) printf("trial %d (%d,%d): %f vs %f\n",trial,i,j,Cbuf[i][j],Cref[i][j]); }
    }
  }
  printf("200 randomised trials with non-zero C_init: %d mismatches -> %s\n", bad, bad?"FAIL":"PASS");
  return bad?1:0;
}