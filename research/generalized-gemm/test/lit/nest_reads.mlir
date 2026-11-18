// RUN: %nestreport %s --function=reads --index-bit-width=16 --max-level=1 --candidates-per-group=1 --rlimit=2000000 --timeout-ms=20000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 0 and 3 are separate,roots 1 and 2 are separate,roots 1 and 3 are separate,roots 2 and 3 are separate" | FileCheck %s --check-prefix=READS
// RUN: %domreport %s --function=reads --lower-structured --segments --index-bit-width=16 --max-level=1 --candidates-per-group=1 --rlimit=2000000 --timeout-ms=20000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 0 and 3 are separate,roots 1 and 2 are separate,roots 1 and 3 are separate,roots 2 and 3 are separate" --groups="all" | FileCheck %s --check-prefix=WHOLE
// Stage 1 of docs/GENERALIZED_GEMM_SCOPE.md, case C. The second nest reads, as an operand,
// what the first wrote. Each has a verified record of its own: the second one's operand A
// is root 2 as it stands at that nest's entry. The dependence is listed: the earlier nest
// writes the root the later one reads.
// READS: "dependences": [
// READS: "earlier": 0,
// READS-NEXT: "earlier_access": [
// READS-NEXT: "read",
// READS-NEXT: "write"
// READS: "earlier_root": 2,
// READS-NEXT: "later": 1,
// READS-NEXT: "later_access": [
// READS-NEXT: "read"
// READS-NEXT: ],
// READS-NEXT: "later_root": 2,
// READS-NEXT: "relation": "same root"
// READS: "nest": 0,
// READS: "ordered_fold_verified": true,
// READS: "refusals": [],
// READS: "statement": 0,
// READS-NEXT: "status": "VERIFIED"
// READS: "extents": {
// READS-NEXT: "K": 2,
// READS-NEXT: "M": 2,
// READS-NEXT: "N": 3
// READS: "A": {
// READS: "root": 2,
// READS: "B": {
// READS: "root": 1,
// READS: "C": {
// READS: "root": 3,
// READS: "nest": 1,
// READS: "ordered_fold_verified": true,
// READS: "refusals": [],
// READS: "statement": 1,
// READS-NEXT: "status": "VERIFIED"
// The whole function, as the existing driver sees it today: the claim over both groups is
// refuted, by a run in which the second nest's operand is not its initial value.
// WHOLE: "reduction": {
// WHOLE: "computation_finite": {
// WHOLE-NEXT: "reason": "the program and the replay differ
// WHOLE: "refuted_by": "COMPUTATION"
// WHOLE: "status": "REFUTED",
func.func @reads(%a: memref<4x4xi32>, %b: memref<4x4xi32>, %c: memref<4x4xi32>, %d: memref<4x4xi32>) {
  affine.for %i = 0 to 2 {
    affine.for %j = 0 to 2 {
      affine.for %k = 0 to 3 {
        %x = affine.load %a[%i, %k] : memref<4x4xi32>
        %y = affine.load %b[%k, %j] : memref<4x4xi32>
        %z = affine.load %c[%i, %j] : memref<4x4xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        affine.store %s, %c[%i, %j] : memref<4x4xi32>
      }
    }
  }
  affine.for %i2 = 0 to 2 {
    affine.for %j2 = 0 to 3 {
      affine.for %k2 = 0 to 2 {
        %x2 = affine.load %c[%i2, %k2] : memref<4x4xi32>
        %y2 = affine.load %b[%k2, %j2] : memref<4x4xi32>
        %z2 = affine.load %d[%i2, %j2] : memref<4x4xi32>
        %m2 = arith.muli %x2, %y2 : i32
        %s2 = arith.addi %z2, %m2 : i32
        affine.store %s2, %d[%i2, %j2] : memref<4x4xi32>
      }
    }
  }
  return
}
