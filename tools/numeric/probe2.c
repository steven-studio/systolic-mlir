// probe2.c — 掃量化位寬 (int8/int12/int16) 與資料分布 (對稱 vs ReLU 後非負)
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

static double frand_sym(void){ return (double)rand()/RAND_MAX*2.0-1.0; }
static double frand_pos(void){ double v=(double)rand()/RAND_MAX*2.0-1.0; return v>0?v:0.0; } // ReLU 後

static void run(int M,int N,int K,int bits,int pos){
    long qmax=(1L<<(bits-1))-1;
    float *A=malloc((size_t)M*K*4),*B=malloc((size_t)K*N*4);
    long *Aq=malloc((size_t)M*K*sizeof(long)),*Bq=malloc((size_t)K*N*sizeof(long));
    double (*gen)(void)= pos?frand_pos:frand_sym;
    for(long i=0;i<(long)M*K;i++)A[i]=(float)gen();
    for(long i=0;i<(long)K*N;i++)B[i]=(float)frand_sym();   // weight 一律對稱
    double ma=0,mb=0;
    for(long i=0;i<(long)M*K;i++){double a=fabs(A[i]); if(a>ma)ma=a;}
    for(long i=0;i<(long)K*N;i++){double a=fabs(B[i]); if(a>mb)mb=a;}
    double sa=ma?ma/qmax:1, sb=mb?mb/qmax:1;
    for(long i=0;i<(long)M*K;i++){long r=lround(A[i]/sa); Aq[i]=r>qmax?qmax:(r<-qmax?-qmax:r);}
    for(long i=0;i<(long)K*N;i++){long r=lround(B[i]/sb); Bq[i]=r>qmax?qmax:(r<-qmax?-qmax:r);}
    double sum_f32=0,sum_q=0,max_f32=0,max_q=0; long accpeak=0;
    for(int m=0;m<M;m++)for(int n=0;n<N;n++){
        double r64=0; float a32=0; long ai=0;
        for(int k=0;k<K;k++){
            float x=A[(long)m*K+k],y=B[(long)k*N+n];
            r64+=(double)x*(double)y; a32+=x*y; ai+=Aq[(long)m*K+k]*Bq[(long)k*N+n];
        }
        if(labs(ai)>accpeak)accpeak=labs(ai);
        double d=fabs(r64)>1e-12?fabs(r64):1e-12;
        double ef=fabs((double)a32-r64)/d, eq=fabs((double)ai*sa*sb-r64)/d;
        sum_f32+=ef; sum_q+=eq; if(ef>max_f32)max_f32=ef; if(eq>max_q)max_q=eq;
    }
    int need=0; while((1L<<need)<=accpeak) need++;
    printf("%5d | int%-2d | %-8s | %9.2e %9.2e | %9.2e %9.2e | %2d bit\n",
        K,bits,pos?"ReLU+":"sym",sum_f32/(M*N),max_f32,sum_q/(M*N),max_q,need+1);
    free(A);free(B);free(Aq);free(Bq);
}
int main(void){
    srand(12345);
    printf("    K | 型別  | 輸入分布 |   fp32 acc 誤差     |   量化+int acc 誤差 | acc 實需位寬\n");
    printf("      |       |          |   mean       max    |   mean       max    |\n");
    printf("------+-------+----------+---------------------+---------------------+-------------\n");
    int Ks[]={16,144,1024,4096};
    for(unsigned i=0;i<4;i++){
        for(int b=8;b<=16;b+=4) run(4,4,Ks[i],b,0);
        for(int b=8;b<=16;b+=4) run(4,4,Ks[i],b,1);
        printf("------+-------+----------+---------------------+---------------------+-------------\n");
    }
    return 0;
}
