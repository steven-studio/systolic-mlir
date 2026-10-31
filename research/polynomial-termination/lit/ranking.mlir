// RUN: %mlir-opt %inputs/loops.mlir --verify-each -o /dev/null
// RUN: %report %inputs/loops.mlir --function=inc_s | %FileCheck %s --check-prefix=RANK
// RUN: %report %inputs/loops.mlir --function=reversed_polarity | %FileCheck %s --check-prefix=RANK
// RUN: %report %inputs/loops.mlir --function=step_two --parameter-range=1:-128:126 | %FileCheck %s --check-prefix=RANK
// RANK: "kind":"linear-ranking"
// RANK-SAME: "ranking_bit_width":10
// RANK-SAME: "ranking_domain":"mathematical naturals on continuing states; exiting successor may be negative"
// RANK-SAME: "status":"PROVEN_TERMINATING"
