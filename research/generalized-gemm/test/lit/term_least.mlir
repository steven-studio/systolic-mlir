// An affine rank by least coefficients (TerminationProposers.cpp, leastAffine). The search
// that takes the template's coefficients and constant from the solver's models finds a rank or
// not as the solver chooses its models: with Z3 4.13 it gives none for @t3_swap and @t3_low at
// 8 bits, with Z3 4.8 it gave one. Every command here runs with that search off
// (--max-cegis-rounds=0) but the last ones, so what is checked does not turn on it: the rank is
// the one whose coefficients are least, and its constant is the template's largest, 2^(w+1).
//
// The programs are the catalogue's (Inputs/gen_loops.py), and no other family has a rank for
// them: with both searches off there is none.

// RUN: python3 %S/Inputs/gen_loops.py 8 > %t8.mlir
// RUN: python3 %S/Inputs/gen_loops.py 4 > %t4.mlir
// RUN: python3 %S/Inputs/gen_loops.py 64 > %t64.mlir

// while (x > 0 && y > 0) (x, y) = (y - 1, x): x + y, and 512 = 2^9.
// RUN: %termreport %t8.mlir --function=t3_swap --max-cegis-rounds=0 | FileCheck %s --check-prefix=SWAP
// SWAP:     "origin": "T3 bounded affine, least coefficients"
// SWAP:     "rank": [
// SWAP:     "value": "512"
// SWAP-NOT: "value":
// SWAP:     "value": "1"
// SWAP-NOT: "value":
// SWAP:     "value": "1"
// SWAP-NOT: "value":
// SWAP:     "strict":
// SWAP:     "verdict": "PROVEN_TERMINATING"{{$}}

// The constant follows the width: 32 = 2^5 at 4 bits, 2^65 at 64.
// RUN: %termreport %t4.mlir --function=t3_swap --max-cegis-rounds=0 | FileCheck %s --check-prefix=SWAP4
// SWAP4:     "origin": "T3 bounded affine, least coefficients"
// SWAP4:     "rank": [
// SWAP4:     "value": "32"
// SWAP4-NOT: "value":
// SWAP4:     "value": "1"
// SWAP4-NOT: "value":
// SWAP4:     "value": "1"
// SWAP4-NOT: "value":
// SWAP4:     "strict":
// SWAP4:     "verdict": "PROVEN_TERMINATING"{{$}}
// RUN: %termreport %t64.mlir --function=t3_swap --max-cegis-rounds=0 --replay-fuel=1000 | FileCheck %s --check-prefix=SWAP64
// SWAP64:     "origin": "T3 bounded affine, least coefficients"
// SWAP64:     "rank": [
// SWAP64:     "value": "36893488147419103232"
// SWAP64-NOT: "value":
// SWAP64:     "value": "1"
// SWAP64-NOT: "value":
// SWAP64:     "value": "1"
// SWAP64-NOT: "value":
// SWAP64:     "strict":
// SWAP64:     "verdict": "PROVEN_TERMINATING"{{$}}

// Two branches, (x - 1, y + 1) and (x + 1, y - 3): 2x + y. Forty candidates come before it in
// the order (over x, y and the constant 0 they are compared with: the 24 whose magnitudes sum
// to 2 at most, and 16 of sum 3), more than the search has rounds: it is reached because the
// samples of the two branches rule most of them out unasked.
// RUN: %termreport %t8.mlir --function=t3_two_step --max-cegis-rounds=0 | FileCheck %s --check-prefix=TWO
// TWO:     "origin": "T3 bounded affine, least coefficients"
// TWO:     "rank": [
// TWO:     "value": "512"
// TWO-NOT: "value":
// TWO:     "value": "2"
// TWO-NOT: "value":
// TWO:     "value": "1"
// TWO-NOT: "value":
// TWO:     "strict":
// TWO:     "verdict": "PROVEN_TERMINATING"{{$}}

// The same loop continuing below zero (x + 42 > 0 && y + 42 > 0): 2x + y is negative there,
// and it is the constant that keeps the rank in range. No rank over x and y stays in range
// without one.
// RUN: %termreport %t8.mlir --function=t3_low --max-cegis-rounds=0 | FileCheck %s --check-prefix=LOW
// LOW:     "origin": "T3 bounded affine, least coefficients"
// LOW:     "rank": [
// LOW:     "value": "512"
// LOW-NOT: "value":
// LOW:     "value": "2"
// LOW-NOT: "value":
// LOW:     "value": "1"
// LOW-NOT: "value":
// LOW:     "strict":
// LOW:     "verdict": "PROVEN_TERMINATING"{{$}}

// With neither search of an affine rank, no rank: the other families have none for these.
// RUN: %termreport %t8.mlir --function=t3_swap --max-cegis-rounds=0 --max-least-rounds=0 | FileCheck %s --check-prefix=NEITHER
// RUN: %termreport %t8.mlir --function=t3_two_step --max-cegis-rounds=0 --max-least-rounds=0 | FileCheck %s --check-prefix=NEITHER
// RUN: %termreport %t8.mlir --function=t3_low --max-cegis-rounds=0 --max-least-rounds=0 | FileCheck %s --check-prefix=NEITHER
// NEITHER: "reason": "no rank for the SCC at L{{[0-9]+}}"
// NEITHER: "verdict": "UNKNOWN"

// As the tool is run, both searches on: proved, whichever of the two finds the rank.
// RUN: %termreport %t8.mlir --function=t3_swap | FileCheck %s --check-prefix=PROVEN
// RUN: %termreport %t8.mlir --function=t3_two_step | FileCheck %s --check-prefix=PROVEN
// RUN: %termreport %t8.mlir --function=t3_low | FileCheck %s --check-prefix=PROVEN
// PROVEN: "verdict": "PROVEN_TERMINATING"{{$}}
func.func private @unused()
