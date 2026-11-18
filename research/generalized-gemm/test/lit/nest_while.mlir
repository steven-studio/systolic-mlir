// GEMMs written with scf.while (docs/GENERALIZED_GEMM_SCOPE.md, stage 3 on scf.while). A counting
// scf.while carries its counter: the before region compares it with its bound and hands it to
// the after region, which holds the body and yields counter + 1. Lowered to control flow it is
// the loop an scf.for is, with one more state variable, the after region's argument, which
// holds the counter in the body. The research verifies such nests as it verifies scf.for
// nests; the body's indices are written over the after region's argument, a state that is no
// level, and the proposer of the views reads it as the level whose value it has at every
// sampled event (DomainProposer.cpp), a proposal the checker then proves over the scope.
//
// Three scf.while loops, the counters as indices.
// RUN: %nestreport %s --function=while_gemm --index-bit-width=64 --max-level=1 --candidates-per-group=1 --rlimit=20000000 --timeout-ms=120000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 0 and 3 are separate,roots 1 and 2 are separate,roots 1 and 3 are separate,roots 2 and 3 are separate" | FileCheck %s --check-prefix=GEMM
// GEMM: "axis": "I",
// GEMM: "lower": 0,
// GEMM: "upper": 16,
// GEMM: "axis": "J",
// GEMM: "lower": 0,
// GEMM: "upper": 8,
// GEMM: "axis": "K",
// GEMM: "lower": 0,
// GEMM: "upper": 12,
// GEMM: "theorem13": {
// GEMM: "status": "VERIFIED",
// GEMM-NEXT: "tier": "SYMBOLIC"
// GEMM: "loop": "scf.while",
// GEMM: "refusals": [],
// GEMM: "statement": 9,
// GEMM-NEXT: "status": "VERIFIED"
//
// Bounds that do not start at 0.
// RUN: %nestreport %s --function=while_shifted --index-bit-width=64 --max-level=1 --candidates-per-group=1 --rlimit=20000000 --timeout-ms=120000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 0 and 3 are separate,roots 1 and 2 are separate,roots 1 and 3 are separate,roots 2 and 3 are separate" | FileCheck %s --check-prefix=SHIFTED
// SHIFTED: "axis": "I",
// SHIFTED: "lower": 2,
// SHIFTED: "upper": 18,
// SHIFTED: "axis": "J",
// SHIFTED: "lower": 1,
// SHIFTED: "upper": 9,
// SHIFTED: "axis": "K",
// SHIFTED: "lower": 3,
// SHIFTED: "upper": 15,
// SHIFTED: "theorem13": {
// SHIFTED: "status": "VERIFIED",
// SHIFTED-NEXT: "tier": "SYMBOLIC"
// SHIFTED: "loop": "scf.while",
// SHIFTED: "refusals": [],
// SHIFTED: "statement": 9,
// SHIFTED-NEXT: "status": "VERIFIED"
//
// An scf.while between two scf.for loops.
// RUN: %nestreport %s --function=while_mixed --index-bit-width=64 --max-level=1 --candidates-per-group=1 --rlimit=20000000 --timeout-ms=120000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 0 and 3 are separate,roots 1 and 2 are separate,roots 1 and 3 are separate,roots 2 and 3 are separate" | FileCheck %s --check-prefix=MIXED
// MIXED: "axis": "I",
// MIXED: "lower": 0,
// MIXED: "upper": 16,
// MIXED: "axis": "J",
// MIXED: "lower": 0,
// MIXED: "upper": 8,
// MIXED: "axis": "K",
// MIXED: "lower": 0,
// MIXED: "upper": 12,
// MIXED: "theorem13": {
// MIXED: "status": "VERIFIED",
// MIXED-NEXT: "tier": "SYMBOLIC"
// MIXED: "loop": "scf.for",
// MIXED: "refusals": [],
// MIXED: "statement": 9,
// MIXED-NEXT: "status": "VERIFIED"
//
// Offsets and a scale computed in the body from the after regions' arguments: each view has
// its closed form over the levels, proposed and proved.
// RUN: %nestreport %s --function=while_offsets --index-bit-width=64 --max-level=1 --candidates-per-group=1 --rlimit=20000000 --timeout-ms=120000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 0 and 3 are separate,roots 1 and 2 are separate,roots 1 and 3 are separate,roots 2 and 3 are separate" | FileCheck %s --check-prefix=OFFSETS
// OFFSETS: "A": {
// OFFSETS: "form": "level0 + 2",
// OFFSETS: "form": "level2 + 1",
// OFFSETS: "proposed": true,
// OFFSETS-NEXT: "proved": true,
// OFFSETS: "B": {
// OFFSETS: "form": "2*level2 + 1",
// OFFSETS: "form": "level1",
// OFFSETS: "proposed": true,
// OFFSETS-NEXT: "proved": true,
// OFFSETS: "C": {
// OFFSETS: "form": "level0 + 1",
// OFFSETS: "form": "level1 + 2",
// OFFSETS: "proposed": true,
// OFFSETS-NEXT: "proved": true,
// OFFSETS: "refusals": [],
// OFFSETS: "statement": 9,
// OFFSETS-NEXT: "status": "VERIFIED"
//
// Not verified today: the middle loop steps its counter before the inner loop, so counter + 1 is
// a temporary held across the inner loop, and the termination certificate has no invariant
// that ties it to the counter (temporary = counter + step). (A limit of the certificate's
// proposer, recorded here as it is.)
// RUN: %nestreport %s --function=while_step_first --index-bit-width=64 --max-level=1 --candidates-per-group=1 --rlimit=20000000 --timeout-ms=120000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 0 and 3 are separate,roots 1 and 2 are separate,roots 1 and 3 are separate,roots 2 and 3 are separate" | FileCheck %s --check-prefix=STEPFIRST
// STEPFIRST: "no claim was made about the nest: the gate is UNKNOWN (no termination certificate)"
// STEPFIRST: "statement": 5,
// STEPFIRST-NEXT: "status": "NOT_VERIFIED"
//
// The outer loop's result, its counter's last value, is stored after the nest: no faithful copy.
// RUN: %nestreport %s --function=while_result_used --index-bit-width=64 --max-level=1 --candidates-per-group=1 --rlimit=20000000 --timeout-ms=120000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 0 and 3 are separate,roots 1 and 2 are separate,roots 1 and 3 are separate,roots 2 and 3 are separate" | FileCheck %s --check-prefix=RESULT
// RESULT: "the nest has no faithful copy: the loop yields a value that is used after it (result 0 of scf.while): the nest's effect is not on memory alone"
// RESULT: "statement": 5,
// RESULT-NEXT: "status": "{{UNSUPPORTED}}"
//
// The reduction carried in a value beside the counter: no claim.
// RUN: %nestreport %s --function=while_accumulator --index-bit-width=64 --max-level=1 --candidates-per-group=1 --rlimit=20000000 --timeout-ms=120000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 0 and 3 are separate,roots 1 and 2 are separate,roots 1 and 3 are separate,roots 2 and 3 are separate" | FileCheck %s --check-prefix=ACCUMULATOR
// ACCUMULATOR: "no claim was made about the nest: the gate is {{UNSUPPORTED}} (a scalar accumulator has no exact output view)"
// ACCUMULATOR: "statement": 5,
// ACCUMULATOR-NEXT: "status": "NOT_VERIFIED"
//
// The inner loop yields its counter unchanged: it never ends, and there is no certificate.
// RUN: %nestreport %s --function=while_no_step --index-bit-width=64 --max-level=1 --candidates-per-group=1 --rlimit=20000000 --timeout-ms=120000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 0 and 3 are separate,roots 1 and 2 are separate,roots 1 and 3 are separate,roots 2 and 3 are separate" | FileCheck %s --check-prefix=NOSTEP
// NOSTEP: "no claim was made about the nest: the gate is UNKNOWN (no termination certificate)"
// NOSTEP: "statement": 5,
// NOSTEP-NEXT: "status": "NOT_VERIFIED"
//
// Loops that go on by != are no supported form of the lowering (the scope document records them
// apart); the next two say what the research makes of them today.
// counter != bound, from 0 to the arrays' sizes: the loop's own condition gives no upper bound of
// the counter, and the accesses do (one outside its array stops the run), so the range is proved.
// RUN: %nestreport %s --function=while_ne --index-bit-width=64 --max-level=1 --candidates-per-group=1 --rlimit=20000000 --timeout-ms=120000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 0 and 3 are separate,roots 1 and 2 are separate,roots 1 and 3 are separate,roots 2 and 3 are separate" | FileCheck %s --check-prefix=NE
// NE: "axis": "I",
// NE: "lower": 0,
// NE: "upper": 16,
// NE: "axis": "J",
// NE: "lower": 0,
// NE: "upper": 8,
// NE: "axis": "K",
// NE: "lower": 0,
// NE: "upper": 12,
// NE: "theorem13": {
// NE: "status": "VERIFIED",
// NE-NEXT: "tier": "SYMBOLIC"
// NE: "loop": "scf.while",
// NE: "refusals": [],
// NE: "statement": 9,
// NE-NEXT: "status": "VERIFIED"
//
// Not verified today: counter != bound from bounds that do not start at 0 (or in arrays larger
// than the bounds). The accesses bound the counters by the arrays' sizes and by 0; that a counter
// stays at or above its start needs the fact that it stays at or below its bound (or it could
// wrap), and the termination certificate has no such invariant for a != loop. (A limit of the
// certificate's proposer, recorded here as it is.)
// RUN: %nestreport %s --function=while_ne_shifted --index-bit-width=64 --max-level=1 --candidates-per-group=1 --rlimit=20000000 --timeout-ms=120000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 0 and 3 are separate,roots 1 and 2 are separate,roots 1 and 3 are separate,roots 2 and 3 are separate" | FileCheck %s --check-prefix=NESHIFTED
// NESHIFTED: "no candidate for the nest: the search is UNKNOWN (no candidate was verified within the limits)"
// NESHIFTED: "statement": 9,
// NESHIFTED-NEXT: "status": "NOT_VERIFIED"
func.func @while_gemm(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %ilo0 = arith.constant 0 : index
  %ihi0 = arith.constant 16 : index
  %ist0 = arith.constant 1 : index
  %jlo0 = arith.constant 0 : index
  %jhi0 = arith.constant 8 : index
  %jst0 = arith.constant 1 : index
  %klo0 = arith.constant 0 : index
  %khi0 = arith.constant 12 : index
  %kst0 = arith.constant 1 : index
  %ires0 = scf.while (%iw0 = %ilo0) : (index) -> index {
    %icmp0 = arith.cmpi slt, %iw0, %ihi0 : index
    scf.condition(%icmp0) %iw0 : index
  } do {
  ^bb0(%i0: index):
    %jres0 = scf.while (%jw0 = %jlo0) : (index) -> index {
      %jcmp0 = arith.cmpi slt, %jw0, %jhi0 : index
      scf.condition(%jcmp0) %jw0 : index
    } do {
    ^bb0(%j0: index):
      %kres0 = scf.while (%kw0 = %klo0) : (index) -> index {
        %kcmp0 = arith.cmpi slt, %kw0, %khi0 : index
        scf.condition(%kcmp0) %kw0 : index
      } do {
      ^bb0(%k0: index):
        %z0 = memref.load %c[%i0, %j0] : memref<16x8xf32>
        %x0 = memref.load %a[%i0, %k0] : memref<16x12xf32>
        %y0 = memref.load %b[%k0, %j0] : memref<12x8xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%i0, %j0] : memref<16x8xf32>
        %knx0 = arith.addi %k0, %kst0 : index
        scf.yield %knx0 : index
      }
      %jnx0 = arith.addi %j0, %jst0 : index
      scf.yield %jnx0 : index
    }
    %inx0 = arith.addi %i0, %ist0 : index
    scf.yield %inx0 : index
  }
  return
}
func.func @while_shifted(%a: memref<18x15xf32>, %b: memref<15x9xf32>, %c: memref<18x9xf32> {llvm.noalias}) {
  %ilo0 = arith.constant 2 : index
  %ihi0 = arith.constant 18 : index
  %ist0 = arith.constant 1 : index
  %jlo0 = arith.constant 1 : index
  %jhi0 = arith.constant 9 : index
  %jst0 = arith.constant 1 : index
  %klo0 = arith.constant 3 : index
  %khi0 = arith.constant 15 : index
  %kst0 = arith.constant 1 : index
  %ires0 = scf.while (%iw0 = %ilo0) : (index) -> index {
    %icmp0 = arith.cmpi slt, %iw0, %ihi0 : index
    scf.condition(%icmp0) %iw0 : index
  } do {
  ^bb0(%i0: index):
    %jres0 = scf.while (%jw0 = %jlo0) : (index) -> index {
      %jcmp0 = arith.cmpi slt, %jw0, %jhi0 : index
      scf.condition(%jcmp0) %jw0 : index
    } do {
    ^bb0(%j0: index):
      %kres0 = scf.while (%kw0 = %klo0) : (index) -> index {
        %kcmp0 = arith.cmpi slt, %kw0, %khi0 : index
        scf.condition(%kcmp0) %kw0 : index
      } do {
      ^bb0(%k0: index):
        %z0 = memref.load %c[%i0, %j0] : memref<18x9xf32>
        %x0 = memref.load %a[%i0, %k0] : memref<18x15xf32>
        %y0 = memref.load %b[%k0, %j0] : memref<15x9xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%i0, %j0] : memref<18x9xf32>
        %knx0 = arith.addi %k0, %kst0 : index
        scf.yield %knx0 : index
      }
      %jnx0 = arith.addi %j0, %jst0 : index
      scf.yield %jnx0 : index
    }
    %inx0 = arith.addi %i0, %ist0 : index
    scf.yield %inx0 : index
  }
  return
}
func.func @while_mixed(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %ilo0 = arith.constant 0 : index
  %ihi0 = arith.constant 16 : index
  %ist0 = arith.constant 1 : index
  %jlo0 = arith.constant 0 : index
  %jhi0 = arith.constant 8 : index
  %jst0 = arith.constant 1 : index
  %klo0 = arith.constant 0 : index
  %khi0 = arith.constant 12 : index
  %kst0 = arith.constant 1 : index
  scf.for %i0 = %ilo0 to %ihi0 step %ist0 {
    %jres0 = scf.while (%jw0 = %jlo0) : (index) -> index {
      %jcmp0 = arith.cmpi slt, %jw0, %jhi0 : index
      scf.condition(%jcmp0) %jw0 : index
    } do {
    ^bb0(%j0: index):
      scf.for %k0 = %klo0 to %khi0 step %kst0 {
        %z0 = memref.load %c[%i0, %j0] : memref<16x8xf32>
        %x0 = memref.load %a[%i0, %k0] : memref<16x12xf32>
        %y0 = memref.load %b[%k0, %j0] : memref<12x8xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%i0, %j0] : memref<16x8xf32>
      }
      %jnx0 = arith.addi %j0, %jst0 : index
      scf.yield %jnx0 : index
    }
  }
  return
}
func.func @while_offsets(%a: memref<18x13xf32>, %b: memref<25x8xf32>, %c: memref<17x10xf32> {llvm.noalias}) {
  %ilo0 = arith.constant 0 : index
  %ihi0 = arith.constant 16 : index
  %ist0 = arith.constant 1 : index
  %jlo0 = arith.constant 0 : index
  %jhi0 = arith.constant 8 : index
  %jst0 = arith.constant 1 : index
  %klo0 = arith.constant 0 : index
  %khi0 = arith.constant 12 : index
  %kst0 = arith.constant 1 : index
  %ires0 = scf.while (%iw0 = %ilo0) : (index) -> index {
    %icmp0 = arith.cmpi slt, %iw0, %ihi0 : index
    scf.condition(%icmp0) %iw0 : index
  } do {
  ^bb0(%i0: index):
    %jres0 = scf.while (%jw0 = %jlo0) : (index) -> index {
      %jcmp0 = arith.cmpi slt, %jw0, %jhi0 : index
      scf.condition(%jcmp0) %jw0 : index
    } do {
    ^bb0(%j0: index):
      %kres0 = scf.while (%kw0 = %klo0) : (index) -> index {
        %kcmp0 = arith.cmpi slt, %kw0, %khi0 : index
        scf.condition(%kcmp0) %kw0 : index
      } do {
      ^bb0(%k0: index):
        %co0c0 = arith.constant 1 : index
        %co1c0 = arith.constant 2 : index
        %ao0c0 = arith.constant 2 : index
        %ao1c0 = arith.constant 1 : index
        %bs0c0 = arith.constant 2 : index
        %bo0c0 = arith.constant 1 : index
        %co00 = arith.addi %i0, %co0c0 : index
        %co10 = arith.addi %j0, %co1c0 : index
        %ao00 = arith.addi %i0, %ao0c0 : index
        %ao10 = arith.addi %k0, %ao1c0 : index
        %bs00 = arith.muli %k0, %bs0c0 : index
        %bo00 = arith.addi %bs00, %bo0c0 : index
        %z0 = memref.load %c[%co00, %co10] : memref<17x10xf32>
        %x0 = memref.load %a[%ao00, %ao10] : memref<18x13xf32>
        %y0 = memref.load %b[%bo00, %j0] : memref<25x8xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%co00, %co10] : memref<17x10xf32>
        %knx0 = arith.addi %k0, %kst0 : index
        scf.yield %knx0 : index
      }
      %jnx0 = arith.addi %j0, %jst0 : index
      scf.yield %jnx0 : index
    }
    %inx0 = arith.addi %i0, %ist0 : index
    scf.yield %inx0 : index
  }
  return
}
func.func @while_step_first(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  %ri = scf.while (%i = %c0) : (index) -> index {
    %ci = arith.cmpi slt, %i, %c16 : index
    scf.condition(%ci) %i : index
  } do {
  ^bb0(%ib: index):
    %rj = scf.while (%j = %c0) : (index) -> index {
      %cj = arith.cmpi slt, %j, %c8 : index
      scf.condition(%cj) %j : index
    } do {
    ^bb0(%jb: index):
      %jn = arith.addi %jb, %c1 : index
      %rk = scf.while (%k = %c0) : (index) -> index {
        %ck = arith.cmpi slt, %k, %c12 : index
        scf.condition(%ck) %k : index
      } do {
      ^bb0(%kb: index):
        %z = memref.load %c[%ib, %jb] : memref<16x8xf32>
        %x = memref.load %a[%ib, %kb] : memref<16x12xf32>
        %y = memref.load %b[%kb, %jb] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%ib, %jb] : memref<16x8xf32>
        %kn = arith.addi %kb, %c1 : index
        scf.yield %kn : index
      }
      scf.yield %jn : index
    }
    %in = arith.addi %ib, %c1 : index
    scf.yield %in : index
  }
  return
}
func.func @while_result_used(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}, %e: memref<4xindex>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  %ri = scf.while (%i = %c0) : (index) -> index {
    %ci = arith.cmpi slt, %i, %c16 : index
    scf.condition(%ci) %i : index
  } do {
  ^bb0(%ib: index):
    %rj = scf.while (%j = %c0) : (index) -> index {
      %cj = arith.cmpi slt, %j, %c8 : index
      scf.condition(%cj) %j : index
    } do {
    ^bb0(%jb: index):
      %rk = scf.while (%k = %c0) : (index) -> index {
        %ck = arith.cmpi slt, %k, %c12 : index
        scf.condition(%ck) %k : index
      } do {
      ^bb0(%kb: index):
        %z = memref.load %c[%ib, %jb] : memref<16x8xf32>
        %x = memref.load %a[%ib, %kb] : memref<16x12xf32>
        %y = memref.load %b[%kb, %jb] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%ib, %jb] : memref<16x8xf32>
        %kn = arith.addi %kb, %c1 : index
        scf.yield %kn : index
      }
      %jn = arith.addi %jb, %c1 : index
      scf.yield %jn : index
    }
    %in = arith.addi %ib, %c1 : index
    scf.yield %in : index
  }
  memref.store %ri, %e[%c0] : memref<4xindex>
  return
}
func.func @while_accumulator(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  %ri = scf.while (%i = %c0) : (index) -> index {
    %ci = arith.cmpi slt, %i, %c16 : index
    scf.condition(%ci) %i : index
  } do {
  ^bb0(%ib: index):
    %rj = scf.while (%j = %c0) : (index) -> index {
      %cj = arith.cmpi slt, %j, %c8 : index
      scf.condition(%cj) %j : index
    } do {
    ^bb0(%jb: index):
      %init = memref.load %c[%ib, %jb] : memref<16x8xf32>
      %rk, %sum = scf.while (%k = %c0, %acc = %init) : (index, f32) -> (index, f32) {
        %ck = arith.cmpi slt, %k, %c12 : index
        scf.condition(%ck) %k, %acc : index, f32
      } do {
      ^bb0(%kb: index, %accb: f32):
        %x = memref.load %a[%ib, %kb] : memref<16x12xf32>
        %y = memref.load %b[%kb, %jb] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %accb, %m fastmath<reassoc> : f32
        %kn = arith.addi %kb, %c1 : index
        scf.yield %kn, %s : index, f32
      }
      memref.store %sum, %c[%ib, %jb] : memref<16x8xf32>
      %jn = arith.addi %jb, %c1 : index
      scf.yield %jn : index
    }
    %in = arith.addi %ib, %c1 : index
    scf.yield %in : index
  }
  return
}
func.func @while_no_step(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  %ri = scf.while (%i = %c0) : (index) -> index {
    %ci = arith.cmpi slt, %i, %c16 : index
    scf.condition(%ci) %i : index
  } do {
  ^bb0(%ib: index):
    %rj = scf.while (%j = %c0) : (index) -> index {
      %cj = arith.cmpi slt, %j, %c8 : index
      scf.condition(%cj) %j : index
    } do {
    ^bb0(%jb: index):
      %rk = scf.while (%k = %c0) : (index) -> index {
        %ck = arith.cmpi slt, %k, %c12 : index
        scf.condition(%ck) %k : index
      } do {
      ^bb0(%kb: index):
        %z = memref.load %c[%ib, %jb] : memref<16x8xf32>
        %x = memref.load %a[%ib, %kb] : memref<16x12xf32>
        %y = memref.load %b[%kb, %jb] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%ib, %jb] : memref<16x8xf32>
        %kn = arith.addi %kb, %c1 : index
        scf.yield %kb : index
      }
      %jn = arith.addi %jb, %c1 : index
      scf.yield %jn : index
    }
    %in = arith.addi %ib, %c1 : index
    scf.yield %in : index
  }
  return
}
func.func @while_ne(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %ilo0 = arith.constant 0 : index
  %ihi0 = arith.constant 16 : index
  %ist0 = arith.constant 1 : index
  %jlo0 = arith.constant 0 : index
  %jhi0 = arith.constant 8 : index
  %jst0 = arith.constant 1 : index
  %klo0 = arith.constant 0 : index
  %khi0 = arith.constant 12 : index
  %kst0 = arith.constant 1 : index
  %ires0 = scf.while (%iw0 = %ilo0) : (index) -> index {
    %icmp0 = arith.cmpi ne, %iw0, %ihi0 : index
    scf.condition(%icmp0) %iw0 : index
  } do {
  ^bb0(%i0: index):
    %jres0 = scf.while (%jw0 = %jlo0) : (index) -> index {
      %jcmp0 = arith.cmpi ne, %jw0, %jhi0 : index
      scf.condition(%jcmp0) %jw0 : index
    } do {
    ^bb0(%j0: index):
      %kres0 = scf.while (%kw0 = %klo0) : (index) -> index {
        %kcmp0 = arith.cmpi ne, %kw0, %khi0 : index
        scf.condition(%kcmp0) %kw0 : index
      } do {
      ^bb0(%k0: index):
        %z0 = memref.load %c[%i0, %j0] : memref<16x8xf32>
        %x0 = memref.load %a[%i0, %k0] : memref<16x12xf32>
        %y0 = memref.load %b[%k0, %j0] : memref<12x8xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%i0, %j0] : memref<16x8xf32>
        %knx0 = arith.addi %k0, %kst0 : index
        scf.yield %knx0 : index
      }
      %jnx0 = arith.addi %j0, %jst0 : index
      scf.yield %jnx0 : index
    }
    %inx0 = arith.addi %i0, %ist0 : index
    scf.yield %inx0 : index
  }
  return
}
func.func @while_ne_shifted(%a: memref<18x15xf32>, %b: memref<15x9xf32>, %c: memref<18x9xf32> {llvm.noalias}) {
  %ilo0 = arith.constant 2 : index
  %ihi0 = arith.constant 18 : index
  %ist0 = arith.constant 1 : index
  %jlo0 = arith.constant 1 : index
  %jhi0 = arith.constant 9 : index
  %jst0 = arith.constant 1 : index
  %klo0 = arith.constant 3 : index
  %khi0 = arith.constant 15 : index
  %kst0 = arith.constant 1 : index
  %ires0 = scf.while (%iw0 = %ilo0) : (index) -> index {
    %icmp0 = arith.cmpi ne, %iw0, %ihi0 : index
    scf.condition(%icmp0) %iw0 : index
  } do {
  ^bb0(%i0: index):
    %jres0 = scf.while (%jw0 = %jlo0) : (index) -> index {
      %jcmp0 = arith.cmpi ne, %jw0, %jhi0 : index
      scf.condition(%jcmp0) %jw0 : index
    } do {
    ^bb0(%j0: index):
      %kres0 = scf.while (%kw0 = %klo0) : (index) -> index {
        %kcmp0 = arith.cmpi ne, %kw0, %khi0 : index
        scf.condition(%kcmp0) %kw0 : index
      } do {
      ^bb0(%k0: index):
        %z0 = memref.load %c[%i0, %j0] : memref<18x9xf32>
        %x0 = memref.load %a[%i0, %k0] : memref<18x15xf32>
        %y0 = memref.load %b[%k0, %j0] : memref<15x9xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%i0, %j0] : memref<18x9xf32>
        %knx0 = arith.addi %k0, %kst0 : index
        scf.yield %knx0 : index
      }
      %jnx0 = arith.addi %j0, %jst0 : index
      scf.yield %jnx0 : index
    }
    %inx0 = arith.addi %i0, %ist0 : index
    scf.yield %inx0 : index
  }
  return
}
