// The relations that the termination certificate's proposer makes from the program's own paths
// (TerminationProposers.cpp; docs/GENERALIZED_GEMM_SCOPE.md, "Stage 3 on scf.while, continued:
// the certificate's relations"): the equality of a state with the one other state whose value
// every change gives it; the equality of a stepped counter, held in a state, with the sum it was
// given; and the orderings of the operands of a != comparison between a state and a state, a
// parameter or a constant. Each is a candidate, kept only where the solver shows it on every
// path into a cut. They are made after the limit on the candidates has cut those made before
// them, under a bound of their own.
// Here: the first two where a certificate needs them, the third where it equals a candidate
// that the limit cut (where a counter's range needs it is in nest_while.mlir), and three
// programs that must get no relation, with the invariants they keep pinned.
//
// A function of two nests of scf.while loops, taken whole. Each scf.while has two variables
// that a loop carries, its counter and the after region's argument, and the rank of the loop
// needs them equal in the body. The candidates made before the relations are more than their
// limit here, and what the limit cuts is the orderings that say so; the equality of the
// argument with the counter is made after the cut.
// RUN: %termreport %s --lower-structured --index-bit-width=64 --function=two_nests | FileCheck %s --check-prefix=TWO
// TWO: "function": "two_nests"
// TWO: "verdict": "PROVEN_TERMINATING_IF_DEFINED"
//
// The same over two small nests on one output, for the whole-function driver: with the
// certificate its gate is open, and the claim over both groups is made and verified. (Over the
// two nests above the claim is still not made, for another reason: the proposer of a
// segment's guard has too many patterns. No record rests on a claim over several nests.)
// RUN: %domreport %s --function=two_small --lower-structured --segments --groups=all --index-bit-width=16 --max-level=1 --candidates-per-group=1 --rlimit=20000000 --timeout-ms=120000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 0 and 3 are separate,roots 0 and 4 are separate,roots 1 and 2 are separate,roots 1 and 3 are separate,roots 1 and 4 are separate,roots 2 and 3 are separate,roots 2 and 4 are separate,roots 3 and 4 are separate" | FileCheck %s --check-prefix=JOINT
// JOINT: "gate": "OPEN",
// JOINT: "group_sets": [
// JOINT: "reason": "Theorem 16 and Theorem 15 over the set, over the scope",
// JOINT: "status": "VERIFIED",
// JOINT-NEXT: "tier": "SYMBOLIC"
// JOINT: "termination": "PROVEN_TERMINATING"
//
// Each counter stepped first: the addition is the first operation of the after region, and
// counter + step is held in a state across the loop inside, which the loop then takes its
// counter from. The proposer makes that sum's equality; the step is a constant from before the
// nest (a state), or a constant of the after region itself.
// RUN: %termreport %s --lower-structured --index-bit-width=64 --function=step_first | FileCheck %s --check-prefix=STEP
// STEP: "function": "step_first"
// STEP: "verdict": "PROVEN_TERMINATING_IF_DEFINED"
// RUN: %termreport %s --lower-structured --index-bit-width=64 --function=step_first_literal | FileCheck %s --check-prefix=LITERAL
// LITERAL: "function": "step_first_literal"
// LITERAL: "verdict": "PROVEN_TERMINATING_IF_DEFINED"
//
// No relation for a for loop: its counter is given a constant and its own sum, and the sum
// that the outer loop's body holds in a state here (%r) steps no counter. The invariants kept
// are the ones kept before the relations were made.
// RUN: %termreport %s --lower-structured --index-bit-width=64 --function=hoisted_sum | FileCheck %s --check-prefix=HOISTED
// HOISTED: "function": "hoisted_sum"
// HOISTED: "invariants_kept": 105,
// HOISTED: "verdict": "PROVEN_TERMINATING_IF_DEFINED"
//
// Nor for a state that is given the value of a state holding a constant: the argument %n of
// the outer loop's body is given %c8 at each iteration, on a path that starts at the outer
// loop's cut, where %c8 is a state. The candidates made before the relations already hold the
// equality of %n with that constant.
// RUN: %termreport %s --lower-structured --index-bit-width=64 --function=passed_constant | FileCheck %s --check-prefix=CONSTANT
// CONSTANT: "function": "passed_constant"
// CONSTANT: "invariants_kept": 84,
// CONSTANT: "verdict": "PROVEN_TERMINATING"{{$}}
//
// Nor for a comparison of computed values: the multiplication of an affine index (%i * 2) has
// an overflow check, a != between two computed values, which is no loop's condition.
// RUN: %termreport %s --lower-structured --index-bit-width=64 --function=scaled_index | FileCheck %s --check-prefix=SCALED
// SCALED: "function": "scaled_index"
// SCALED: "invariants_kept": 105,
// SCALED: "verdict": "PROVEN_TERMINATING_IF_DEFINED"
//
// A relation that equals a candidate the limit cut is made again. After a nest of scf.while
// loops, a loop goes on while %p differs from %q, two variables that the loop carries; their
// orderings are among the candidates made before the relations, at the end, where the limit
// cuts them. The orderings of the != comparison are those same facts, made after the cut; with
// %p <= %q the access to %v is inside it, no stop is left, and the verdict has no "if defined".
// RUN: %termreport %s --lower-structured --index-bit-width=64 --function=ne_carried | FileCheck %s --check-prefix=CARRIED
// CARRIED: "function": "ne_carried"
// CARRIED: "verdict": "PROVEN_TERMINATING"{{$}}
func.func @two_nests(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}, %d: memref<12x8xf32>) {
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
  %ilo1 = arith.constant 0 : index
  %ihi1 = arith.constant 16 : index
  %ist1 = arith.constant 1 : index
  %jlo1 = arith.constant 0 : index
  %jhi1 = arith.constant 8 : index
  %jst1 = arith.constant 1 : index
  %klo1 = arith.constant 0 : index
  %khi1 = arith.constant 12 : index
  %kst1 = arith.constant 1 : index
  %ires1 = scf.while (%iw1 = %ilo1) : (index) -> index {
    %icmp1 = arith.cmpi slt, %iw1, %ihi1 : index
    scf.condition(%icmp1) %iw1 : index
  } do {
  ^bb0(%i1: index):
    %jres1 = scf.while (%jw1 = %jlo1) : (index) -> index {
      %jcmp1 = arith.cmpi slt, %jw1, %jhi1 : index
      scf.condition(%jcmp1) %jw1 : index
    } do {
    ^bb0(%j1: index):
      %kres1 = scf.while (%kw1 = %klo1) : (index) -> index {
        %kcmp1 = arith.cmpi slt, %kw1, %khi1 : index
        scf.condition(%kcmp1) %kw1 : index
      } do {
      ^bb0(%k1: index):
        %z1 = memref.load %c[%i1, %j1] : memref<16x8xf32>
        %x1 = memref.load %a[%i1, %k1] : memref<16x12xf32>
        %y1 = memref.load %d[%k1, %j1] : memref<12x8xf32>
        %m1 = arith.mulf %x1, %y1 : f32
        %s1 = arith.addf %z1, %m1 fastmath<reassoc> : f32
        memref.store %s1, %c[%i1, %j1] : memref<16x8xf32>
        %knx1 = arith.addi %k1, %kst1 : index
        scf.yield %knx1 : index
      }
      %jnx1 = arith.addi %j1, %jst1 : index
      scf.yield %jnx1 : index
    }
    %inx1 = arith.addi %i1, %ist1 : index
    scf.yield %inx1 : index
  }
  return
}
func.func @step_first(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
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
    %inx0 = arith.addi %i0, %ist0 : index
    %jres0 = scf.while (%jw0 = %jlo0) : (index) -> index {
      %jcmp0 = arith.cmpi slt, %jw0, %jhi0 : index
      scf.condition(%jcmp0) %jw0 : index
    } do {
    ^bb0(%j0: index):
      %jnx0 = arith.addi %j0, %jst0 : index
      %kres0 = scf.while (%kw0 = %klo0) : (index) -> index {
        %kcmp0 = arith.cmpi slt, %kw0, %khi0 : index
        scf.condition(%kcmp0) %kw0 : index
      } do {
      ^bb0(%k0: index):
        %knx0 = arith.addi %k0, %kst0 : index
        %z0 = memref.load %c[%i0, %j0] : memref<16x8xf32>
        %x0 = memref.load %a[%i0, %k0] : memref<16x12xf32>
        %y0 = memref.load %b[%k0, %j0] : memref<12x8xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%i0, %j0] : memref<16x8xf32>
        scf.yield %knx0 : index
      }
      scf.yield %jnx0 : index
    }
    scf.yield %inx0 : index
  }
  return
}
func.func @step_first_literal(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %ilo0 = arith.constant 0 : index
  %ihi0 = arith.constant 16 : index
  %jlo0 = arith.constant 0 : index
  %jhi0 = arith.constant 8 : index
  %klo0 = arith.constant 0 : index
  %khi0 = arith.constant 12 : index
  %ires0 = scf.while (%iw0 = %ilo0) : (index) -> index {
    %icmp0 = arith.cmpi slt, %iw0, %ihi0 : index
    scf.condition(%icmp0) %iw0 : index
  } do {
  ^bb0(%i0: index):
    %ione0 = arith.constant 1 : index
    %inx0 = arith.addi %i0, %ione0 : index
    %jres0 = scf.while (%jw0 = %jlo0) : (index) -> index {
      %jcmp0 = arith.cmpi slt, %jw0, %jhi0 : index
      scf.condition(%jcmp0) %jw0 : index
    } do {
    ^bb0(%j0: index):
      %jone0 = arith.constant 1 : index
      %jnx0 = arith.addi %j0, %jone0 : index
      %kres0 = scf.while (%kw0 = %klo0) : (index) -> index {
        %kcmp0 = arith.cmpi slt, %kw0, %khi0 : index
        scf.condition(%kcmp0) %kw0 : index
      } do {
      ^bb0(%k0: index):
        %kone0 = arith.constant 1 : index
        %knx0 = arith.addi %k0, %kone0 : index
        %z0 = memref.load %c[%i0, %j0] : memref<16x8xf32>
        %x0 = memref.load %a[%i0, %k0] : memref<16x12xf32>
        %y0 = memref.load %b[%k0, %j0] : memref<12x8xf32>
        %m0 = arith.mulf %x0, %y0 : f32
        %s0 = arith.addf %z0, %m0 fastmath<reassoc> : f32
        memref.store %s0, %c[%i0, %j0] : memref<16x8xf32>
        scf.yield %knx0 : index
      }
      scf.yield %jnx0 : index
    }
    scf.yield %inx0 : index
  }
  return
}
func.func @hoisted_sum(%a: memref<17x12xf32>, %c: memref<16x8xf32>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  scf.for %i = %c0 to %c16 step %c1 {
    %r = arith.addi %i, %c1 : index
    scf.for %j = %c0 to %c8 step %c1 {
      scf.for %k = %c0 to %c12 step %c1 {
        %x = memref.load %a[%r, %k] : memref<17x12xf32>
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %s = arith.addf %z, %x : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
func.func @two_small(%a: memref<6x6xi32>, %b: memref<6x6xi32>, %c: memref<6x6xi32>, %d: memref<6x6xi32>, %e: memref<6x6xi32>) {
  %ilo0 = arith.constant 0 : index
  %ihi0 = arith.constant 3 : index
  %ist0 = arith.constant 1 : index
  %jlo0 = arith.constant 0 : index
  %jhi0 = arith.constant 3 : index
  %jst0 = arith.constant 1 : index
  %klo0 = arith.constant 0 : index
  %khi0 = arith.constant 3 : index
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
        %xoc0n0 = arith.constant 1 : index
        %xo0n0 = arith.addi %i0, %xoc0n0 : index
        %x0 = memref.load %a[%xo0n0, %k0] : memref<6x6xi32>
        %y0 = memref.load %b[%k0, %j0] : memref<6x6xi32>
        %z0 = memref.load %c[%i0, %j0] : memref<6x6xi32>
        %m0 = arith.muli %x0, %y0 : i32
        %s0 = arith.addi %m0, %z0 : i32
        memref.store %s0, %c[%i0, %j0] : memref<6x6xi32>
        %knx0 = arith.addi %k0, %kst0 : index
        scf.yield %knx0 : index
      }
      %jnx0 = arith.addi %j0, %jst0 : index
      scf.yield %jnx0 : index
    }
    %inx0 = arith.addi %i0, %ist0 : index
    scf.yield %inx0 : index
  }
  %klo1 = arith.constant 1 : index
  %khi1 = arith.constant 3 : index
  %kst1 = arith.constant 1 : index
  %jlo1 = arith.constant 0 : index
  %jhi1 = arith.constant 3 : index
  %jst1 = arith.constant 1 : index
  %ilo1 = arith.constant 1 : index
  %ihi1 = arith.constant 4 : index
  %ist1 = arith.constant 1 : index
  %kres1 = scf.while (%kw1 = %klo1) : (index) -> index {
    %kcmp1 = arith.cmpi slt, %kw1, %khi1 : index
    scf.condition(%kcmp1) %kw1 : index
  } do {
  ^bb0(%k1: index):
    %jres1 = scf.while (%jw1 = %jlo1) : (index) -> index {
      %jcmp1 = arith.cmpi slt, %jw1, %jhi1 : index
      scf.condition(%jcmp1) %jw1 : index
    } do {
    ^bb0(%j1: index):
      %ires1 = scf.while (%iw1 = %ilo1) : (index) -> index {
        %icmp1 = arith.cmpi slt, %iw1, %ihi1 : index
        scf.condition(%icmp1) %iw1 : index
      } do {
      ^bb0(%i1: index):
        %xoc0n1 = arith.constant 1 : index
        %xo0n1 = arith.addi %i1, %xoc0n1 : index
        %xoc1n1 = arith.constant 2 : index
        %xo1n1 = arith.addi %k1, %xoc1n1 : index
        %x1 = memref.load %a[%xo0n1, %xo1n1] : memref<6x6xi32>
        %ysc1n1 = arith.constant 2 : index
        %ys1n1 = arith.muli %j1, %ysc1n1 : index
        %y1 = memref.load %d[%k1, %ys1n1] : memref<6x6xi32>
        %zoc0n1 = arith.constant 1 : index
        %zo0n1 = arith.addi %i1, %zoc0n1 : index
        %zoc1n1 = arith.constant 1 : index
        %zo1n1 = arith.addi %j1, %zoc1n1 : index
        %z1 = memref.load %c[%zo0n1, %zo1n1] : memref<6x6xi32>
        %m1 = arith.muli %x1, %y1 : i32
        %s1 = arith.addi %m1, %z1 : i32
        %woc0n1 = arith.constant 1 : index
        %wo0n1 = arith.addi %i1, %woc0n1 : index
        %woc1n1 = arith.constant 1 : index
        %wo1n1 = arith.addi %j1, %woc1n1 : index
        memref.store %s1, %c[%wo0n1, %wo1n1] : memref<6x6xi32>
        %inx1 = arith.addi %i1, %ist1 : index
        scf.yield %inx1 : index
      }
      %jnx1 = arith.addi %j1, %jst1 : index
      scf.yield %jnx1 : index
    }
    %knx1 = arith.addi %k1, %kst1 : index
    scf.yield %knx1 : index
  }
  return
}
func.func @passed_constant(%a: memref<8xf32>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  cf.br ^head(%c0 : index)
^head(%i: index):
  %g = arith.cmpi slt, %i, %c8 : index
  cf.cond_br %g, ^body(%c8 : index), ^exit
^body(%n: index):
  cf.br ^inner(%c0 : index)
^inner(%j: index):
  %h = arith.cmpi slt, %j, %n : index
  cf.cond_br %h, ^work, ^next
^work:
  %x = memref.load %a[%j] : memref<8xf32>
  memref.store %x, %a[%j] : memref<8xf32>
  %v = arith.addi %j, %c1 : index
  cf.br ^inner(%v : index)
^next:
  %u = arith.addi %i, %c1 : index
  cf.br ^head(%u : index)
^exit:
  return
}
func.func @ne_carried(%a: memref<16x12xi32>, %b: memref<12x8xi32>, %c: memref<16x8xi32> {llvm.noalias}, %v: memref<12xi32>) {
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
        %z0 = memref.load %c[%i0, %j0] : memref<16x8xi32>
        %x0 = memref.load %a[%i0, %k0] : memref<16x12xi32>
        %y0 = memref.load %b[%k0, %j0] : memref<12x8xi32>
        %m0 = arith.muli %x0, %y0 : i32
        %s0 = arith.addi %z0, %m0 : i32
        memref.store %s0, %c[%i0, %j0] : memref<16x8xi32>
        %knx0 = arith.addi %k0, %kst0 : index
        scf.yield %knx0 : index
      }
      %jnx0 = arith.addi %j0, %jst0 : index
      scf.yield %jnx0 : index
    }
    %inx0 = arith.addi %i0, %ist0 : index
    scf.yield %inx0 : index
  }
  %p0 = arith.constant 0 : index
  %q0 = arith.constant 12 : index
  %one = arith.constant 1 : index
  %rp, %rq = scf.while (%p = %p0, %q = %q0) : (index, index) -> (index, index) {
    %go = arith.cmpi ne, %p, %q : index
    scf.condition(%go) %p, %q : index, index
  } do {
  ^bb0(%pb: index, %qb: index):
    %x = memref.load %v[%pb] : memref<12xi32>
    memref.store %x, %v[%pb] : memref<12xi32>
    %pn = arith.addi %pb, %one : index
    scf.yield %pn, %qb : index, index
  }
  return
}
func.func @scaled_index(%a: memref<31x12xf32>, %c: memref<16x8xf32>) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 12 {
        %x = affine.load %a[%i * 2, %k] : memref<31x12xf32>
        %z = affine.load %c[%i, %j] : memref<16x8xf32>
        %s = arith.addf %z, %x : f32
        affine.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
