// RUN: systolic-cost invocation --geometry 8 --k-max 16 --k 17 | FileCheck %s --check-prefix=K17
// RUN: systolic-cost invocation --geometry 8 --k-max 16 --k 17 --compute-only | FileCheck %s --check-prefix=CMP
// RUN: systolic-cost invocation --geometry 8 --k-max 17 --k 17 | FileCheck %s --check-prefix=M17
// RUN: systolic-cost invocation --geometry 4 --k-max 7 --k 8 | FileCheck %s --check-prefix=K7
// RUN: systolic-cost invocation --geometry 8 --k-max 256 --k 37 | FileCheck %s --check-prefix=K37
// RUN: systolic-cost invocation --geometry 8 --k-max 256 --k 300 | FileCheck %s --check-prefix=K300
// RUN: systolic-cost invocation --geometry 8 --k-max 1 --k 40 | FileCheck %s --check-prefix=K1
// RUN: systolic-cost invocation --geometry 4 --k-max 16376 --k 16376 | FileCheck %s --check-prefix=BIG
// RUN: not systolic-cost invocation --geometry 8 --k-max 1 --k 9223372036854775807 2>&1 | FileCheck %s --check-prefix=OVF
// RUN: not systolic-cost invocation --geometry 8 --k-max 0 --k 4 2>&1 | FileCheck %s --check-prefix=BAD
// RUN: systolic-cost invocation --geometry 8 --k-max 16 --k 17 --extra-read-overhead 213/11 | FileCheck %s --check-prefix=EXTRA8
// RUN: systolic-cost invocation --geometry 4 --k-max 7 --k 8 --extra-read-overhead 213/11 | FileCheck %s --check-prefix=EXTRA4
// RUN: not systolic-cost invocation --geometry 8 --k-max 16 --k 17 --extra-read-overhead 5/ 2>&1 | FileCheck %s --check-prefix=EXTRABAD

// The read / compute / write model of one invocation (CostModel.h) on the
// FPGA fleet's constants (Systolic/FpgaFleet.h), worked by hand. Variable-K:
// invocation i of a K-step reduction runs k_i = min(K_MAX, K - i * K_MAX)
// steps -- K_MAX is the operand buffer's capacity -- and
//
//   read    = ceil(W_read / 4.0 B/cycle) + 11,
//             W_read = 2 operands x 8 lanes x ceil(k_i / 8) * 8 steps x 4 B
//   compute = k_i + 2g - 2 + 95
//   write   = ceil(g * g * 4 B / 3.2 B/cycle) + 11

// K = 17 on an 8x8 with K_MAX = 16: 16 steps, then 1. The second
// invocation is one window (512 B -> 128 + 11) and 1 + 14 + 95.
// K17: invocation j=0 k=16 read_bytes=1024 write_bytes=256 read=267 compute=125 write=91 total=483
// K17: invocation j=1 k=1 read_bytes=512 write_bytes=256 read=139 compute=110 write=91 total=340
// K17: tile geometry=8 k_max=16 k=17 invocations=2 read=406 compute=235 write=182 total=823 read_bytes=1536 write_bytes=512

// No read or write term: the compute-only model, K + I * (R + C - 2 + H).
// CMP: invocation j=0 k=16 {{.*}} read=0 compute=125 write=0 total=125
// CMP: invocation j=1 k=1 {{.*}} read=0 compute=110 write=0 total=110
// CMP: tile geometry=8 k_max=16 k=17 invocations=2 read=0 compute=235 write=0 total=235

// K_MAX = 17, not a multiple of 8: three windows on the wire (1536 B).
// M17: invocation j=0 k=17 read_bytes=1536 write_bytes=256 read=395 compute=126 write=91 total=612
// M17: tile geometry=8 k_max=17 k=17 invocations=1

// A 4x4 moves the same 8 lanes and keeps 4: K = 8 on K_MAX = 7 is 7 + 1,
// one window each; write 64 B -> 20 + 11.
// K7: invocation j=0 k=7 read_bytes=512 write_bytes=64 read=139 compute=108 write=31 total=278
// K7: invocation j=1 k=1 read_bytes=512 write_bytes=64 read=139 compute=102 write=31 total=272
// K7: tile geometry=4 k_max=7 k=8 invocations=2 read=278 compute=210 write=62 total=550

// K_MAX = 256, K = 37: one invocation of 37 steps, five windows -- not 256.
// K37: invocation j=0 k=37 read_bytes=2560 write_bytes=256 read=651 compute=146 write=91 total=888
// K37: tile geometry=8 k_max=256 k=37 invocations=1

// K_MAX = 256, K = 300: 256 + 44 steps; the second invocation is six
// windows (3072 B -> 768 + 11) and 44 + 14 + 95.
// K300: invocation j=0 k=256 read_bytes=16384 write_bytes=256 read=4107 compute=365 write=91 total=4563
// K300: invocation j=1 k=44 read_bytes=3072 write_bytes=256 read=779 compute=153 write=91 total=1023
// K300: tile geometry=8 k_max=256 k=300 invocations=2 read=4886 compute=518 write=182 total=5586

// K_MAX = 1: one invocation per reduction step, each a whole window.
// K1: invocation j=0 k=1 read_bytes=512 write_bytes=256 read=139 compute=110 write=91 total=340
// K1: invocation j=16..38 same as j=15
// K1: invocation j=39 k=1
// K1: tile geometry=8 k_max=1 k=40 invocations=40 read=5560 compute=4400 write=3640 total=13600

// The deepest slab the descriptor can name (K_MAX = 16376): 2047 windows.
// BIG: invocation j=0 k=16376 read_bytes=1048064 write_bytes=64 read=262027 compute=16477 write=31 total=278535

// 2^63 - 1 invocations do not fit in int64 cycles: refused, not wrapped.
// OVF: cannot price the tile (invalid or overflowing)

// BAD: --geometry, --k-max and --k must be >= 1

// --extra-read-overhead C8/C4: a what-if term outside the model, added to
// every invocation's read (213 on an 8x8, 11 on a 4x4: the result readback
// between two invocations measured in simulation).
// EXTRA8: invocation j=0 k=16 read_bytes=1024 write_bytes=256 read=480 compute=125 write=91 total=696
// EXTRA8: invocation j=1 k=1 read_bytes=512 write_bytes=256 read=352 compute=110 write=91 total=553
// EXTRA4: invocation j=0 k=7 read_bytes=512 write_bytes=64 read=150 compute=108 write=31 total=289
// EXTRA4: invocation j=1 k=1 read_bytes=512 write_bytes=64 read=150 compute=102 write=31 total=283
// EXTRABAD: --extra-read-overhead must be C or C8/C4, each >= 0
