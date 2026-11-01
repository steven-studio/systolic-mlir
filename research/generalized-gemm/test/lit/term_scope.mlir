// The domain is an input. The same loop, three scopes, three exact answers.
// RUN: %termreport %s --replay-fuel=4096 | FileCheck %s --check-prefix=ALL
// RUN: %termreport %s --scope=0:-128:126 --replay-fuel=4096 | FileCheck %s --check-prefix=TIGHT
// RUN: %termreport %s --scope=0:5:1 | FileCheck %s --check-prefix=EMPTY
// RUN: not %termreport %s --scope=0:-500:1 2>&1 | FileCheck %s --check-prefix=BAD
// ALL: "verdict": "PROVEN_TERMINATING_IF_DEFINED"
// TIGHT: "verdict": "PROVEN_TERMINATING"{{$}}
// TIGHT-NOT: IF_DEFINED
// EMPTY: "reason": "the parameter domain is empty
// EMPTY: "verdict": "UNKNOWN"
// BAD: malformed --scope
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
