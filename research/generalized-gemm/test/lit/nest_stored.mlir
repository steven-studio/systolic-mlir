// RUN: %nestreport %s --function=stored --index-bit-width=16 --max-level=1 --candidates-per-group=1 --rlimit=2000000 --timeout-ms=20000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 0 and 3 are separate,roots 1 and 2 are separate,roots 1 and 3 are separate,roots 2 and 3 are separate" | FileCheck %s --check-prefix=STORED
// RUN: %domreport %s --function=stored --lower-structured --segments --index-bit-width=16 --max-level=1 --candidates-per-group=1 --rlimit=2000000 --timeout-ms=20000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 0 and 3 are separate,roots 1 and 2 are separate,roots 1 and 3 are separate,roots 2 and 3 are separate" --groups="all" | FileCheck %s --check-prefix=WHOLE
// Stage 1 of docs/GENERALIZED_GEMM_SCOPE.md, case C. A nest on C, a value of C stored into D,
// a nest on D. Each nest has a verified record of its own. The statements between them are
// in the function's order, where they stay: the second record is a claim from the memory
// at its nest's entry, whatever the store put there.
// STORED: "authorizes_lowering": false,
// STORED: "function": "stored",
// STORED-NEXT: "nests": [
// STORED: "extents": {
// STORED-NEXT: "K": 3,
// STORED-NEXT: "M": 2,
// STORED-NEXT: "N": 2
// STORED: "nest": 0,
// STORED: "ordered_fold_verified": true,
// STORED: "refusals": [],
// STORED: "statement": 2,
// STORED-NEXT: "status": "VERIFIED"
// STORED: "extents": {
// STORED-NEXT: "K": 2,
// STORED-NEXT: "M": 2,
// STORED-NEXT: "N": 3
// STORED: "C": {
// STORED: "root": 3,
// STORED: "nest": 1,
// STORED: "ordered_fold_verified": true,
// STORED: "refusals": [],
// STORED: "statement": 5,
// STORED-NEXT: "status": "VERIFIED"
// STORED: "order": [
// STORED: "kind": "nest",
// STORED: "nest": 0,
// STORED: "access": [
// STORED-NEXT: "read"
// STORED: "kind": "access",
// STORED: "operation": "memref.load",
// STORED-NEXT: "root": 2,
// STORED: "access": [
// STORED-NEXT: "write"
// STORED: "kind": "access",
// STORED: "operation": "memref.store",
// STORED-NEXT: "root": 3,
// STORED: "kind": "nest",
// STORED: "nest": 1,
// The whole function, as the existing driver sees it today: the claim over both groups is
// refuted, by a run.
// WHOLE: "reduction": {
// WHOLE: "refuted_by": "COMPUTATION"
// WHOLE: "status": "REFUTED",
func.func @stored(%a: memref<4x4xi32>, %b: memref<4x4xi32>, %c: memref<4x4xi32>, %d: memref<4x4xi32>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
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
  %v = memref.load %c[%c0, %c0] : memref<4x4xi32>
  memref.store %v, %d[%c1, %c1] : memref<4x4xi32>
  affine.for %i2 = 0 to 2 {
    affine.for %j2 = 0 to 3 {
      affine.for %k2 = 0 to 2 {
        %x2 = affine.load %a[%i2, %k2] : memref<4x4xi32>
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
