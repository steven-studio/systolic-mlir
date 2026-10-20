// REQUIRES: cfront-clang
//
// The caller-side shim systolic-cc generates for an offloaded function has
// the function's own parameter names in scope, and pastes expressions of
// them (the run-time extent of an array whose rows a parameter places) into
// its body. Its own locals and helpers therefore carry the implementation's
// reserved prefix (__systolic_cc_), so no parameter or array name can be
// shadowed by them or shadow them. One compile per name, selected with -D:
//   - a row-offset parameter named bad, checked, e (the --verify check's
//     locals), snapshots, completed, hw, before, after, once, inv, d, n, got,
//     want, same, close_runtime: each compiles, the result equals the
//     program compiled by clang alone, and --verify checks all 512 elements
//     the call writes (32 rows of 16 from the pointer: the offset is 16);
//   - with the offset named bad, a result corrupted beyond the first 16
//     rows is a NUMERICAL MISMATCH, never "verified";
//   - an input array named save_C or ref_C and a written array named e or
//     got: the kernel and the check use the caller's arrays.
// On the software runtime model (NOT the FPGA).
//
// RUN: %host-cc -std=c11 %s -o %t.cpu
// RUN: %t.cpu > %t.cpu.out
//
// RUN: %systolic-cc --fp-reassociation --verify -DOFF=bad %s -o %t.bad > /dev/null 2>&1
// RUN: %t.bad > %t.bad.out 2> %t.bad.err
// RUN: FileCheck %s --check-prefix=V512 --input-file=%t.bad.err
// RUN: grep -v '^\[systolic-cc\]' %t.bad.out | diff %t.cpu.out -
// RUN: env SOFT_RUNTIME_FAULT=corrupt SOFT_RUNTIME_FAULT_AT=3 not %t.bad 2>&1 | FileCheck %s --check-prefix=CORRUPT
//
// RUN: %systolic-cc --fp-reassociation --verify -DOFF=checked %s -o %t.checked > /dev/null 2>&1
// RUN: %t.checked 2>&1 | FileCheck %s --check-prefix=V512
// RUN: %systolic-cc --fp-reassociation --verify -DOFF=e %s -o %t.e > /dev/null 2>&1
// RUN: %t.e 2>&1 | FileCheck %s --check-prefix=V512
// RUN: %systolic-cc --fp-reassociation --verify -DOFF=snapshots %s -o %t.snapshots > /dev/null 2>&1
// RUN: %t.snapshots 2>&1 | FileCheck %s --check-prefix=V512
// RUN: %systolic-cc --fp-reassociation --verify -DOFF=completed %s -o %t.completed > /dev/null 2>&1
// RUN: %t.completed 2>&1 | FileCheck %s --check-prefix=V512
// RUN: %systolic-cc --fp-reassociation --verify -DOFF=hw %s -o %t.hw > /dev/null 2>&1
// RUN: %t.hw 2>&1 | FileCheck %s --check-prefix=V512
// RUN: %systolic-cc --fp-reassociation --verify -DOFF=before %s -o %t.before > /dev/null 2>&1
// RUN: %t.before 2>&1 | FileCheck %s --check-prefix=V512
// RUN: %systolic-cc --fp-reassociation --verify -DOFF=after %s -o %t.after > /dev/null 2>&1
// RUN: %t.after 2>&1 | FileCheck %s --check-prefix=V512
// RUN: %systolic-cc --fp-reassociation --verify -DOFF=once %s -o %t.once > /dev/null 2>&1
// RUN: %t.once 2>&1 | FileCheck %s --check-prefix=V512
// RUN: %systolic-cc --fp-reassociation --verify -DOFF=inv %s -o %t.inv > /dev/null 2>&1
// RUN: %t.inv 2>&1 | FileCheck %s --check-prefix=V512
// RUN: %systolic-cc --fp-reassociation --verify -DOFF=d %s -o %t.d > /dev/null 2>&1
// RUN: %t.d 2>&1 | FileCheck %s --check-prefix=V512
// RUN: %systolic-cc --fp-reassociation --verify -DOFF=n %s -o %t.n > /dev/null 2>&1
// RUN: %t.n 2>&1 | FileCheck %s --check-prefix=V512
// RUN: %systolic-cc --fp-reassociation --verify -DOFF=got %s -o %t.got > /dev/null 2>&1
// RUN: %t.got 2>&1 | FileCheck %s --check-prefix=V512
// RUN: %systolic-cc --fp-reassociation --verify -DOFF=want %s -o %t.want > /dev/null 2>&1
// RUN: %t.want 2>&1 | FileCheck %s --check-prefix=V512
// RUN: %systolic-cc --fp-reassociation --verify -DOFF=same %s -o %t.same > /dev/null 2>&1
// RUN: %t.same 2>&1 | FileCheck %s --check-prefix=V512
// RUN: %systolic-cc --fp-reassociation --verify -DOFF=close_runtime %s -o %t.close > /dev/null 2>&1
// RUN: %t.close 2>&1 | FileCheck %s --check-prefix=V512
//
// Without --verify too.
// RUN: %systolic-cc --fp-reassociation -DOFF=hw %s -o %t.plain_hw > /dev/null 2>&1
// RUN: %t.plain_hw | grep -v '^\[systolic-cc\]' | diff %t.cpu.out -
// RUN: %systolic-cc --fp-reassociation -DOFF=once %s -o %t.plain_once > /dev/null 2>&1
// RUN: %t.plain_once | grep -v '^\[systolic-cc\]' | diff %t.cpu.out -
//
// Array names.
// RUN: %systolic-cc --fp-reassociation --verify -DAN=save_C %s -o %t.save_C > /dev/null 2>&1
// RUN: %t.save_C > %t.save_C.out 2> %t.save_C.err
// RUN: FileCheck %s --check-prefix=V512 --input-file=%t.save_C.err
// RUN: grep -v '^\[systolic-cc\]' %t.save_C.out | diff %t.cpu.out -
// RUN: %systolic-cc --fp-reassociation --verify -DAN=ref_C %s -o %t.ref_C > /dev/null 2>&1
// RUN: %t.ref_C > %t.ref_C.out 2> %t.ref_C.err
// RUN: FileCheck %s --check-prefix=V512 --input-file=%t.ref_C.err
// RUN: grep -v '^\[systolic-cc\]' %t.ref_C.out | diff %t.cpu.out -
// RUN: %systolic-cc --fp-reassociation --verify -DCN=e %s -o %t.Ce > /dev/null 2>&1
// RUN: %t.Ce > %t.Ce.out 2> %t.Ce.err
// RUN: FileCheck %s --check-prefix=V512 --input-file=%t.Ce.err
// RUN: grep -v '^\[systolic-cc\]' %t.Ce.out | diff %t.cpu.out -
// RUN: %systolic-cc --fp-reassociation --verify -DCN=got %s -o %t.Cgot > /dev/null 2>&1
// RUN: %t.Cgot > %t.Cgot.out 2> %t.Cgot.err
// RUN: FileCheck %s --check-prefix=V512 --input-file=%t.Cgot.err
// RUN: grep -v '^\[systolic-cc\]' %t.Cgot.out | diff %t.cpu.out -

#include <stdio.h>

#ifndef OFF
#define OFF r0
#endif
#ifndef AN
#define AN A
#endif
#ifndef CN
#define CN C
#endif

// 16 rows of C from row OFF: called with OFF = 16, the rows 16..31 of a
// 32-row array are written, rows 0..15 are not.
void gemm_named(float AN[restrict 16][16], float B[restrict 16][16],
                float CN[restrict 32][16], int OFF) {
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 16; ++j)
      for (int k = 0; k < 16; ++k)
        CN[OFF + i][j] += AN[i][k] * B[k][j];
}

static float A_[16][16], B_[16][16], C_[32][16];

int main(void) {
  for (int ii = 0; ii < 16; ++ii)
    for (int jj = 0; jj < 16; ++jj) {
      A_[ii][jj] = (float)((ii * 3 + jj) % 7 - 3);
      B_[ii][jj] = (float)((ii * 5 + jj * 3) % 5 - 2);
    }
  for (int ii = 0; ii < 32; ++ii)
    for (int jj = 0; jj < 16; ++jj)
      C_[ii][jj] = (float)((ii + jj) % 3 - 1);
  gemm_named(A_, B_, C_, 16);
  double s = 0;
  for (int ii = 0; ii < 32; ++ii)
    for (int jj = 0; jj < 16; ++jj)
      s += C_[ii][jj] * (double)(ii * 16 + jj + 1);
  printf("checksum %.1f\n", s);
  return 0;
}

// V512: gemm_named (shim_names.c:{{[0-9]+}}): verified: 512 elements equal the CPU reference (bitwise)
//
// CORRUPT:     gemm_named (shim_names.c:{{[0-9]+}}): first mismatch {{.*}}
// CORRUPT:     gemm_named (shim_names.c:{{[0-9]+}}): NUMERICAL MISMATCH: 1 of 512 elements differ from the CPU reference (bitwise)
// CORRUPT-NOT: verified:
