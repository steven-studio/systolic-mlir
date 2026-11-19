// GEMMs written with scf.for (docs/GENERALIZED_GEMM_SCOPE.md, stage 3 on scf.for). The bounds
// and steps of an scf.for are values defined before the nest, which the importer keeps in
// state. The grammar tells operands apart by their values at a thin sample of the events; when
// the sample meets the innermost loop at one phase (here every twelfth of 1536 events, and K
// runs 12), its counter is 0 at every sample, like the bound held in state, and the search lost
// the counter: no candidate. When the search with the thin sample verifies no candidate at the
// symbolic tier, it now runs once more with each leaf compared on every event, and takes that
// outcome only when it verifies a candidate symbolically (SynthesisAnalysis.cpp).
//
// RUN: %nestreport %s --function=scf_gemm --index-bit-width=64 --max-level=1 --candidates-per-group=1 --rlimit=20000000 --timeout-ms=120000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 1 and 2 are separate" | FileCheck %s --check-prefix=NEST
// NEST: "axis": "I",
// NEST: "value": "(state:bv64 5)"
// NEST: "axis": "J",
// NEST: "value": "(state:bv64 6)"
// NEST: "axis": "K",
// NEST: "value": "(state:bv64 7)"
// NEST: "theorem13": {
// NEST: "status": "VERIFIED",
// NEST-NEXT: "tier": "SYMBOLIC"
// NEST: "loop": "scf.for",
// NEST: "refusals": [],
// NEST: "statement": 5,
// NEST-NEXT: "status": "VERIFIED"
//
// The search of the same nest: the three counters, verified at the symbolic tier.
// RUN: %synthreport %s --function=scf_gemm --lower-structured --index-bit-width=64 --max-level=1 --max-candidates=1 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 1 and 2 are separate" | FileCheck %s --check-prefix=SYNTH
// SYNTH: "atoms": 3,
// SYNTH: "state": 5,
// SYNTH: "state": 6,
// SYNTH: "state": 7,
// SYNTH: "status": "VERIFIED",
// SYNTH-NEXT: "tier": "SCOPE_ENUMERATED"
// SYNTH: "status": "VERIFIED",
// SYNTH-NEXT: "tier": "SYMBOLIC_EXACT"
// SYNTH: "status": "CANDIDATE_FOUND"
//
// Bounds that do not start at 0, each held in state: J starts at a state that holds 1, K at
// one that holds 3. The termination certificate's proposer reads such a start through the
// constant the state holds (TerminationProposers.cpp), so J >= 1 and K >= 3 are proposed,
// checked, and the coordinates' range (V3) is proved.
// RUN: %nestreport %s --function=scf_shifted --index-bit-width=64 --max-level=1 --candidates-per-group=1 --rlimit=20000000 --timeout-ms=120000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 1 and 2 are separate" | FileCheck %s --check-prefix=SHIFTED
// SHIFTED: "axis": "I",
// SHIFTED: "lower": 2,
// SHIFTED: "axis": "J",
// SHIFTED: "lower": 1,
// SHIFTED: "axis": "K",
// SHIFTED: "lower": 3,
// SHIFTED: "loop": "scf.for",
// SHIFTED: "refusals": [],
// SHIFTED: "statement": 9,
// SHIFTED-NEXT: "status": "VERIFIED"
//
// A step of 2 for I (1536 events again, the sample meets K at one phase): the second search
// verifies nothing at the symbolic tier either, so the outcome is the first search's, as it was
// before the second search existed.
// RUN: %synthreport %s --function=scf_stepped_aligned --lower-structured --index-bit-width=64 --max-level=1 --max-candidates=1 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 1 and 2 are separate" | FileCheck %s --check-prefix=KEPT
// KEPT: "atoms": 4,
// KEPT-NEXT: "candidates": [],
// KEPT-NEXT: "clauses": 3,
// KEPT-NEXT: "exhausted": false,
// KEPT-NEXT: "group": 0,
// KEPT-NEXT: "level": 1,
// KEPT-NEXT: "proposals": 0,
// KEPT-NEXT: "reason": "no candidate was verified within the limits",
// KEPT-NEXT: "refuted": 0,
// KEPT-NEXT: "rounds": 99,
// KEPT-NEXT: "status": "UNKNOWN"
//
// A step of 2 with 768 events: the sample meets K at two phases, the first search finds the
// counters, proposed but not verified symbolically, and so does the second.
// RUN: %synthreport %s --function=scf_stepped --lower-structured --index-bit-width=64 --max-level=1 --max-candidates=1 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 1 and 2 are separate" | FileCheck %s --check-prefix=PROPOSED
// PROPOSED: "atoms": 3,
// PROPOSED: "tier": "PROPOSED"
// PROPOSED: "status": "CANDIDATE_FOUND"
func.func @scf_gemm(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  scf.for %i = %c0 to %c16 step %c1 {
    scf.for %j = %c0 to %c8 step %c1 {
      scf.for %k = %c0 to %c12 step %c1 {
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %x = memref.load %a[%i, %k] : memref<16x12xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
func.func @scf_stepped(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  scf.for %i = %c0 to %c16 step %c2 {
    scf.for %j = %c0 to %c8 step %c1 {
      scf.for %k = %c0 to %c12 step %c1 {
        %z = memref.load %c[%i, %j] : memref<16x8xf32>
        %x = memref.load %a[%i, %k] : memref<16x12xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<16x8xf32>
      }
    }
  }
  return
}
func.func @scf_stepped_aligned(%a: memref<32x12xf32>, %b: memref<12x8xf32>, %c: memref<32x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c32 = arith.constant 32 : index
  scf.for %i = %c0 to %c32 step %c2 {
    scf.for %j = %c0 to %c8 step %c1 {
      scf.for %k = %c0 to %c12 step %c1 {
        %z = memref.load %c[%i, %j] : memref<32x8xf32>
        %x = memref.load %a[%i, %k] : memref<32x12xf32>
        %y = memref.load %b[%k, %j] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<32x8xf32>
      }
    }
  }
  return
}
func.func @scf_shifted(%a: memref<18x15xf32>, %b: memref<15x9xf32>, %c: memref<18x9xf32> {llvm.noalias}) {
  %ilo = arith.constant 2 : index
  %ihi = arith.constant 18 : index
  %ist = arith.constant 1 : index
  %jlo = arith.constant 1 : index
  %jhi = arith.constant 9 : index
  %jst = arith.constant 1 : index
  %klo = arith.constant 3 : index
  %khi = arith.constant 15 : index
  %kst = arith.constant 1 : index
  scf.for %i = %ilo to %ihi step %ist {
    scf.for %j = %jlo to %jhi step %jst {
      scf.for %k = %klo to %khi step %kst {
        %z = memref.load %c[%i, %j] : memref<18x9xf32>
        %x = memref.load %a[%i, %k] : memref<18x15xf32>
        %y = memref.load %b[%k, %j] : memref<15x9xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%i, %j] : memref<18x9xf32>
      }
    }
  }
  return
}
