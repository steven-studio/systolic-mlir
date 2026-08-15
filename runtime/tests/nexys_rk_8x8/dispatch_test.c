/* dispatch_test.c */
#include "systolic_dispatch_new.h"
#include <stdio.h>
int main(void) {
    int h = systolic_dispatch_open();
    if (h < 0) return 1;
    /* A = [[1,2],[3,4],0...], B = [[10,...],[100,...]], K=2
     * C[0][0] = 0 + 1*10 + 2*100 = 210  -- 交換 A/B 會得到別的數 */
    static float A[8*2] = {1,2, 3,4}, B[2*8] = {10,0,0,0,0,0,0,0,
                                                100,0,0,0,0,0,0,0};
    static float Cin[64] = {0}, Cout[64];
    if (systolic_dispatch_matmul(h, 2, A, B, Cin, Cout) != 0) return 1;
    printf("C[0][0]=%g (want 210)  C[1][0]=%g (want 430)\n",
           Cout[0], Cout[8]);
    return (Cout[0] == 210.0f && Cout[8] == 430.0f) ? 0 : 1;
}
