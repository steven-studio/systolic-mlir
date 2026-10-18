// Calibration: one device only (no shared-read contention). 8x8: K=64 as 4x16, K=33 as 16,16,1, K=4 as 1x4, K=48 as 3x16.
localparam integer NJ = 14;
localparam integer JOB_DEV   [0:NJ-1] = '{ 0,  0,  0,  0,   0,  0,  0,   0, 0, 0, 0,   0,  0,  0 };
localparam integer JOB_KDESC [0:NJ-1] = '{16, 16, 16, 16,  16, 16,  1,   1, 1, 1, 1,  16, 16, 16 };
localparam integer JOB_KLOG  [0:NJ-1] = '{16, 16, 16, 16,  16, 16,  1,   1, 1, 1, 1,  16, 16, 16 };
localparam integer JOB_START [0:NJ-1] = '{ 0,  0,  0,  0,   0,  0,  0,   0, 0, 0, 0,   0,  0,  0 };
localparam integer JOB_KOFF  [0:NJ-1] = '{ 0, 16, 32, 48,   0, 16, 32,   0, 1, 2, 3,   0, 16, 32 };
localparam integer JOB_KTOT  [0:NJ-1] = '{64, 64, 64, 64,  33, 33, 33,   4, 4, 4, 4,  48, 48, 48 };
