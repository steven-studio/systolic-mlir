// Gap-bound: 4x4 at K_MAX 32 on the v2 operand path, K = 33 as k = 32 then
// k = 1, four times.  The k = 1 job's fill (one window, 43 cycles) lands long
// before the previous job's 32 steps + FEED_GAP have passed, and the bank /
// accumulator-set rules are not binding either (the job two back published
// 136 cycles ago), so its fold is released by the quiet-gap rule alone: the
// gap PE(0,0) sees must be exactly FEED_GAP (26).  Run with PE_QUIET_GAP
// below the PE's minimum (the negative control), the two tiles merge in the
// PEs and the bench must FAIL (fewer publishes than folds, wrong sums).
localparam integer NJ = 8;
localparam integer JOB_DEV   [0:NJ-1] = '{ 1,  1,  1,  1,  1,  1,  1,  1 };
localparam integer JOB_KDESC [0:NJ-1] = '{32,  1, 32,  1, 32,  1, 32,  1 };
localparam integer JOB_KLOG  [0:NJ-1] = '{32,  1, 32,  1, 32,  1, 32,  1 };
localparam integer JOB_START [0:NJ-1] = '{ 0,  0,  0,  0,  0,  0,  0,  0 };
localparam integer JOB_KOFF  [0:NJ-1] = '{ 0, 32,  0, 32,  0, 32,  0, 32 };
localparam integer JOB_KTOT  [0:NJ-1] = '{33, 33, 33, 33, 33, 33, 33, 33 };
