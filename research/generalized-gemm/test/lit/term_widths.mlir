// Width changes the answer. Nothing here depends on a run reaching the bound.
// RUN: python3 %S/Inputs/gen_loops.py 32 > %t32.mlir
// RUN: %termreport %t32.mlir --function=b3_wrap --replay-fuel=1000 | FileCheck %s --check-prefix=PROVEN
// RUN: %termreport %t32.mlir --function=c_nsw --replay-fuel=1000 | FileCheck %s --check-prefix=DEFINED
// RUN: %termreport %t32.mlir --function=e_rect --replay-fuel=1000 | FileCheck %s --check-prefix=PROVEN
// RUN: %termreport %t32.mlir --function=g_dec3 --replay-fuel=1000 | FileCheck %s --check-prefix=PROVEN
// A cycle of 2^31 steps is real, but out of reach here: never a refutation by
// timeout, and never a proof.
// RUN: %termreport %t32.mlir --function=b2_wrap --replay-fuel=1000 | FileCheck %s --check-prefix=NEITHER
// RUN: python3 %S/Inputs/gen_loops.py 64 > %t64.mlir
// RUN: %termreport %t64.mlir --function=b3_wrap --replay-fuel=1000 | FileCheck %s --check-prefix=PROVEN
// RUN: %termreport %t64.mlir --function=c_nsw --replay-fuel=1000 | FileCheck %s --check-prefix=DEFINED
// RUN: %termreport %t64.mlir --function=h_true --replay-fuel=1000 | FileCheck %s --check-prefix=REFUTED
// PROVEN: "verdict": "PROVEN_TERMINATING"{{$}}
// DEFINED: "verdict": "PROVEN_TERMINATING_IF_DEFINED"
// NEITHER: "verdict": "UNKNOWN"
// REFUTED: "verdict": "REFUTED_NONTERMINATING"
func.func private @unused()
