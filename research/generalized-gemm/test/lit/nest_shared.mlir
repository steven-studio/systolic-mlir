// RUN: %nestreport %s --function=shared --index-bit-width=16 --max-level=1 --candidates-per-group=1 --rlimit=2000000 --timeout-ms=20000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 0 and 3 are separate,roots 1 and 2 are separate,roots 1 and 3 are separate,roots 2 and 3 are separate" | FileCheck %s --check-prefix=SHARED
// RUN: %domreport %s --function=shared --lower-structured --segments --index-bit-width=16 --max-level=1 --candidates-per-group=1 --rlimit=2000000 --timeout-ms=20000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 0 and 3 are separate,roots 1 and 2 are separate,roots 1 and 3 are separate,roots 2 and 3 are separate" --groups="all" | FileCheck %s --check-prefix=WHOLE
// Stage 1 of docs/GENERALIZED_GEMM_SCOPE.md, case B. Two nests on one output, C += A * B and
// then C += A * D. Each has a record of its own, verified from any memory at its entry: the
// second nest's claim does not ask what the first wrote. The two meet on C, which is
// listed, and the order of the statements is kept. Nothing is merged, and no record
// authorizes a lowering.
// SHARED: "authorizes_lowering": false,
// SHARED: "dependences": [
// SHARED: "earlier": 0,
// SHARED: "earlier_root": 2,
// SHARED-NEXT: "later": 1,
// SHARED: "later_root": 2,
// SHARED-NEXT: "relation": "same root"
// SHARED: "function": "shared",
// SHARED-NEXT: "nests": [
// SHARED: "authorizes_lowering": false,
// SHARED: "statement": "C[h] := C[h] + A[f] * B[g]"
// SHARED: "extents": {
// SHARED-NEXT: "K": 2,
// SHARED-NEXT: "M": 2,
// SHARED-NEXT: "N": 3
// SHARED: "theorem13": {
// SHARED: "status": "VERIFIED",
// SHARED-NEXT: "tier": "SYMBOLIC"
// SHARED: "B": {
// SHARED: "root": 1,
// SHARED: "C": {
// SHARED: "root": 2,
// SHARED: "nest": 0,
// SHARED: "ordered_fold_verified": true,
// SHARED: "function": "shared__nest0",
// SHARED: "reassociation_permitted": true,
// SHARED-NEXT: "refusals": [],
// SHARED: "statement": 0,
// SHARED-NEXT: "status": "VERIFIED"
// SHARED: "authorizes_lowering": false,
// SHARED: "extents": {
// SHARED-NEXT: "K": 4,
// SHARED-NEXT: "M": 3,
// SHARED-NEXT: "N": 2
// SHARED: "B": {
// SHARED: "root": 3,
// SHARED: "C": {
// SHARED: "root": 2,
// SHARED: "nest": 1,
// SHARED: "ordered_fold_verified": true,
// SHARED: "function": "shared__nest1",
// SHARED: "refusals": [],
// The second copy numbers its roots 0, 1, 2; the names in the record are the function's.
// SHARED: "name": "roots 0 and 2 are separate",
// SHARED: "name": "roots 0 and 3 are separate",
// SHARED: "name": "roots 2 and 3 are separate",
// SHARED: "statement": 1,
// SHARED-NEXT: "status": "VERIFIED"
// SHARED: "schema": "generalized-gemm-nest/1",
// SHARED-NEXT: "status": "OUTLINED"
// The whole function, as the existing driver sees it today: the claim over both groups is
// verified, and each group's own claim is refuted, since the other nest writes the same
// cells. (The claim over the set is printed first.)
// WHOLE: "group_sets": [
// WHOLE: "reduction": {
// WHOLE: "refuted_by": "NOTHING"
// WHOLE: "status": "VERIFIED",
// WHOLE-NEXT: "tier": "SYMBOLIC"
// WHOLE: "groups": [
// WHOLE: "theorem13": {
// WHOLE: "status": "REFUTED",
// WHOLE: "theorem13": {
// WHOLE: "status": "REFUTED",
func.func @shared(%a: memref<4x4xi32>, %b: memref<4x4xi32>, %c: memref<4x4xi32>, %d: memref<4x4xi32>) {
  affine.for %i = 0 to 2 {
    affine.for %j = 0 to 3 {
      affine.for %k = 0 to 2 {
        %x = affine.load %a[%i, %k] : memref<4x4xi32>
        %y = affine.load %b[%k, %j] : memref<4x4xi32>
        %z = affine.load %c[%i, %j] : memref<4x4xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        affine.store %s, %c[%i, %j] : memref<4x4xi32>
      }
    }
  }
  affine.for %i2 = 0 to 3 {
    affine.for %j2 = 0 to 2 {
      affine.for %k2 = 0 to 4 {
        %x2 = affine.load %a[%i2, %k2] : memref<4x4xi32>
        %y2 = affine.load %d[%k2, %j2] : memref<4x4xi32>
        %z2 = affine.load %c[%i2, %j2] : memref<4x4xi32>
        %m2 = arith.muli %x2, %y2 : i32
        %s2 = arith.addi %z2, %m2 : i32
        affine.store %s2, %c[%i2, %j2] : memref<4x4xi32>
      }
    }
  }
  return
}
