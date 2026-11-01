// The four verdicts and their exact semantic distinctions, at i8.
// RUN: %termreport %s | FileCheck %s

// The JSON keys are sorted: function, reason, stops, verdict, witness.
// CHECK: "function": "wrap"
// CHECK: "verdict": "PROVEN_TERMINATING"{{$}}
func.func @wrap(%n: i8) {
  %z = arith.constant 0 : i8
  %c = arith.constant 3 : i8
  cf.br ^h(%z : i8)
^h(%i: i8):
  %g = arith.cmpi slt, %i, %n : i8
  cf.cond_br %g, ^b, ^x
^b:
  %u = arith.addi %i, %c : i8
  cf.br ^h(%u : i8)
^x:
  return
}

// The same source with nsw is not the same program: overflow reaches a branch.
// CHECK: "function": "nsw"
// CHECK: "kind": "UB",
// CHECK-NEXT: "sites":
// CHECK-NEXT: "status": "REACHABLE"
// CHECK: "verdict": "PROVEN_TERMINATING_IF_DEFINED"{{$}}
func.func @nsw(%n: i8) {
  %z = arith.constant 0 : i8
  %c = arith.constant 3 : i8
  cf.br ^h(%z : i8)
^h(%i: i8):
  %g = arith.cmpi slt, %i, %n : i8
  cf.cond_br %g, ^b, ^x
^b:
  %u = arith.addi %i, %c overflow<nsw> : i8
  cf.br ^h(%u : i8)
^x:
  return
}

// CHECK: "function": "spin"
// CHECK: "verdict": "REFUTED_NONTERMINATING"
// CHECK: "witness"
// CHECK: "period": 1
func.func @spin() {
  %t = arith.constant true
  cf.br ^h
^h:
  cf.cond_br %t, ^h, ^x
^x:
  return
}

// A failed rank search is no statement about the program: this loop terminates.
// CHECK: "function": "piecewise"
// CHECK: "verdict": "UNKNOWN"
func.func @piecewise(%n: i8) {
  %c0 = arith.constant 0 : i8
  %c2 = arith.constant 2 : i8
  %a = arith.constant 3 : i8
  %b = arith.constant 7 : i8
  cf.br ^h(%c0 : i8)
^h(%i: i8):
  %g = arith.cmpi slt, %i, %n : i8
  cf.cond_br %g, ^body, ^x
^body:
  %r = arith.remsi %i, %c2 : i8
  %e = arith.cmpi eq, %r, %c0 : i8
  cf.cond_br %e, ^t, ^f
^t:
  %p = arith.addi %i, %a : i8
  cf.br ^h(%p : i8)
^f:
  %q = arith.addi %i, %b : i8
  cf.br ^h(%q : i8)
^x:
  return
}

// An unknown call is not termination.
// CHECK: "function": "external"
// CHECK: "reason": "unsupported: ExternalCall
// CHECK: "verdict": "UNKNOWN"
func.func private @opaque()
func.func @external() {
  call @opaque() : () -> ()
  return
}
