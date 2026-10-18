// K-folded chains: dev0 K=64 as 4x16; dev1 K=48 as 3x16; dev0 K=33 as 16,16,1; dev2 k=5; dev0 K=4 as 1,1,1,1
localparam integer NJ = 15;
localparam integer JOB_DEV   [0:NJ-1] = '{ 0,  0,  0,  0,   1,  1,  1,   0,  0,  0,   2,   0, 0, 0, 0 };
localparam integer JOB_KDESC [0:NJ-1] = '{16, 16, 16, 16,  16, 16, 16,  16, 16,  1,   5,   1, 1, 1, 1 };
localparam integer JOB_KLOG  [0:NJ-1] = '{16, 16, 16, 16,  16, 16, 16,  16, 16,  1,   5,   1, 1, 1, 1 };
localparam integer JOB_START [0:NJ-1] = '{ 0,  0,  0,  0,   0,  0,  0,   0,  0,  0,   0,   0, 0, 0, 0 };
localparam integer JOB_KOFF  [0:NJ-1] = '{ 0, 16, 32, 48,   0, 16, 32,   0, 16, 32,   0,   0, 1, 2, 3 };
localparam integer JOB_KTOT  [0:NJ-1] = '{64, 64, 64, 64,  48, 48, 48,  33, 33, 33,   5,   4, 4, 4, 4 };
