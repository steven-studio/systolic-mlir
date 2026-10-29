// RUN: %mlir-opt %inputs/accepted.mlir --verify-each -o /dev/null
// RUN: %cfg-report %inputs/accepted.mlir --function=sum_squares --evaluate=10 | %FileCheck %s --check-prefix=SQUARES
// RUN: %cfg-report %inputs/accepted.mlir --function=sum_squares_next_i --evaluate=10 | %FileCheck %s --check-prefix=NEXT
// RUN: %cfg-report %inputs/accepted.mlir --function=forwarded_noncanonical --evaluate=10 | %FileCheck %s --check-prefix=FORWARD
// SQUARES: "binomial_coefficients":["0/1","0/1","1/1","2/1"]
// SQUARES-SAME: "status":"PROVEN"
// SQUARES-SAME: "values":["10","285"]
// NEXT: "values":["10","385"]
// FORWARD: "header_block":4
// FORWARD-SAME: "values":["10","285"]
