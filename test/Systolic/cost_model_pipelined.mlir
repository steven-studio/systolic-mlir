// RUN: systolic-cost invocation --geometry 8 --k-max 16 --k 17 --pipelined | FileCheck %s --check-prefix=K17
// RUN: systolic-cost invocation --geometry 8 --k-max 16 --k 17 | FileCheck %s --check-prefix=SER
// RUN: systolic-cost timeline --geometry 8 --k-max 16 --k 16,16,16,16 --pipelined --read-bytes-per-cycle 16 | FileCheck %s --check-prefix=V2
// RUN: systolic-cost timeline --geometry 4 --k-max 16 --k 1,1,1,1 --pipelined --read-bytes-per-cycle 16 | FileCheck %s --check-prefix=RED
// RUN: systolic-cost timeline --geometry 8 --k-max 32 --k 32,32,1 --pipelined --compute-only | FileCheck %s --check-prefix=CMP
// RUN: systolic-cost timeline --geometry 8 --k-max 16 --k 16,16 --pipelined --accept 0,500 | FileCheck %s --check-prefix=ACC
// RUN: systolic-cost timeline --geometry 8 --k-max 16 --k 16,16 --pipeline-depth 1 --reduce-cycles 0 --feed-gap 0 | FileCheck %s --check-prefix=D1
// RUN: not systolic-cost timeline --geometry 8 --k-max 16 --k 16 --pipelined --pipeline-depth 2 --reduce-cycles 69 --feed-gap 26 2>&1 | FileCheck %s --check-prefix=BOTH
// RUN: not systolic-cost timeline --geometry 8 --k-max 16 --k 16 --pipeline-depth 4 --reduce-cycles 96 --feed-gap 26 2>&1 | FileCheck %s --check-prefix=BADR
// RUN: systolic-cost check-monotone --m 64 --n 64 --k 64 --pipelined | FileCheck %s --check-prefix=MONO
// RUN: systolic-cost check-monotone --m 100 --n 36 --k 147 --k8 24 --k4 12 --pipelined --shared-read-words-per-cycle 1 | FileCheck %s --check-prefix=MONO
// RUN: systolic-cost check-monotone --m 49 --n 512 --k 4608 --k8 17 --k4 7 --pipelined | FileCheck %s --check-prefix=MONO

// The per-accelerator timeline of consecutive invocations (CostModel.h,
// AcceleratorTimeline) on the pipelined RTL's constants (Systolic/FpgaFleet.h:
// pipeline depth 4, reduce 69, feed gap 26), worked by hand against the
// recurrences and against the stamps of rtl/.../dma/tb/tb_pipeline_overlap.sv
// (eval/pipeline_overlap/run.sh holds the two to the cycle):
//
//   fillStart_j = max(accept_j, fillEnd_{j-1}, feedStart_{j-1})
//   feedStart_j = max(fillStart_j + T_read, feedStart_{j-1} + k_{j-1} + 26,
//                     computeEnd_{j-2} + 3, writeEnd_{j-2} + 3)
//   computeEnd_j = max(feedStart_j + T_compute, computeEnd_{j-1} + 69)
//   writeEnd_j = max(computeEnd_j, writeEnd_{j-1}) + T_write

// K = 17 on an 8x8, K_MAX = 16: the second invocation's fill (139) starts at
// the first one's fold (267) and its fold at 406 -- while the first one is
// still being reduced (its result publishes at 392) -- so the tile spans 607
// instead of the serialized 823. The per-invocation terms are unchanged.
// K17: invocation j=0 k=16 read_bytes=1024 write_bytes=256 read=267 compute=125 write=91 total=483
// K17: invocation j=1 k=1 read_bytes=512 write_bytes=256 read=139 compute=110 write=91 total=340
// K17: tile geometry=8 k_max=16 k=17 invocations=2 read=406 compute=235 write=182 total=823 read_bytes=1536 write_bytes=512 span=607 pipelined=yes
// K17: event tile=0 j=0 k=16 accept=0 fill_start=0 fill_end=267 feed_start=267 compute_end=392 write_start=392 write_end=483 start_cycle=0
// K17: event tile=0 j=1 k=1 accept=0 fill_start=267 fill_end=406 feed_start=406 compute_end=516 write_start=516 write_end=607 start_cycle=42
// K17: stream tiles=1 invocations=2 end=607 compute_only_end=194

// Serialized (the archived bitstream): the second invocation is accepted at
// the first one's write end; span = total.
// SER: tile geometry=8 k_max=16 k=17 invocations=2 read=406 compute=235 write=182 total=823 read_bytes=1536 write_bytes=512 span=823 pipelined=no
// SER: event tile=0 j=0 k=16 accept=0 fill_start=0 fill_end=267 feed_start=267 compute_end=392 write_start=392 write_end=483 start_cycle=0
// SER: event tile=0 j=1 k=1 accept=483 fill_start=483 fill_end=622 feed_start=622 compute_end=732 write_start=732 write_end=823 start_cycle=125

// The beat-wide v2 operand path (16 B/cycle: T_read = 64 + 11 = 75 for two
// windows), four 8x8 invocations of 16 steps: the second fold (150) starts
// 50 cycles before the first result publishes (200) -- the next tile
// accumulating while the previous one reduces, the PE's ping-pong -- and
// the third fold waits for the first tile's write-back (291 + 3 = 294: its
// result bank), the fourth for the second's (382 + 3 = 385). Measured on the
// RTL, dev0 v2 chain of tb_pipeline_overlap: fold 75 / 150 / 294, cv 200 /
// 275 / 419, done 291 / 382 / 510 after the first acceptance.
// V2: event j=0 k=16 accept=0 fill_start=0 fill_end=75 feed_start=75 compute_end=200 write_start=200 write_end=291 start_cycle=0
// V2: event j=1 k=16 accept=0 fill_start=75 fill_end=150 feed_start=150 compute_end=275 write_start=291 write_end=382 start_cycle=42
// V2: event j=2 k=16 accept=0 fill_start=150 fill_end=225 feed_start=294 compute_end=419 write_start=419 write_end=510 start_cycle=128
// V2: event j=3 k=16 accept=0 fill_start=294 fill_end=369 feed_start=385 compute_end=510 write_start=510 write_end=601 start_cycle=197
// V2: stream invocations=4 end=601 compute_only_end=332

// Reduction-bound: 4x4 invocations of one step on v2 (T_read = 43 for one
// window, T_compute = 102). The second fold starts at 86, its own latency
// would publish at 188, but the first tile's bank reduction holds the PEs'
// reduction path: 145 + 69 = 214 (measured 69 apart on the RTL).
// RED: event j=0 k=1 accept=0 fill_start=0 fill_end=43 feed_start=43 compute_end=145 write_start=145 write_end=176 start_cycle=0
// RED: event j=1 k=1 accept=0 fill_start=43 fill_end=86 feed_start=86 compute_end=214 write_start=214 write_end=245 start_cycle=27
// RED: event j=2 k=1 accept=0 fill_start=86 fill_end=129 feed_start=179 compute_end=283 write_start=283 write_end=314 start_cycle=105
// RED: event j=3 k=1 accept=0 fill_start=179 fill_end=222 feed_start=248 compute_end=352 write_start=352 write_end=383 start_cycle=174

// The compute-only timeline (transfers free) is where start_cycle lives: a
// 65-step reduction on an 8x8 of K_MAX 32 folds 32, 32, 1; the second fold
// follows the first after its 32 steps and the 26-cycle gap, the third once
// the first tile has published (141 + 3). Serialized these were 0, 141, 282.
// CMP: event j=0 k=32 accept=0 fill_start=0 fill_end=0 feed_start=0 compute_end=141 write_start=141 write_end=141 start_cycle=0
// CMP: event j=1 k=32 accept=0 fill_start=0 fill_end=0 feed_start=58 compute_end=210 write_start=210 write_end=210 start_cycle=58
// CMP: event j=2 k=1 accept=0 fill_start=58 fill_end=58 feed_start=144 compute_end=279 write_start=279 write_end=279 start_cycle=144

// An observed acceptance (the host's pace, the DPTI readback) moves a
// descriptor later; everything after follows, on both timelines.
// ACC: event j=0 k=16 accept=0 fill_start=0 fill_end=267 feed_start=267 compute_end=392 write_start=392 write_end=483 start_cycle=0
// ACC: event j=1 k=16 accept=500 fill_start=500 fill_end=767 feed_start=767 compute_end=892 write_start=892 write_end=983 start_cycle=500

// Depth 1 given explicitly is the serialized model to the cycle.
// D1: event j=1 k=16 accept=483 fill_start=483 fill_end=750 feed_start=750 compute_end=875 write_start=875 write_end=966 start_cycle=125

// BOTH: --pipelined and --pipeline-depth / --reduce-cycles / --feed-gap exclude each other
// BADR: --pipeline-depth must be >= 1, --reduce-cycles in 0..95 and --feed-gap >= 0

// The binary search over pipelined accelerators: every tile's interval is
// the stream's, the search's minimum is minimal, feasibility monotone.
// MONO: schedule makespan={{[0-9]+}}
// MONO: check valid=yes minimal=yes monotone=yes
