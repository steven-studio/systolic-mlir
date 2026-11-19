// RUN: %domreport %s --function=one_bit_level --lower-structured --index-bit-width=16 --scope=0:1:3 --rlimit=2000000 --timeout-ms=3000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 0 and 3 are separate,roots 1 and 2 are separate,roots 1 and 3 are separate,roots 2 and 3 are separate" | FileCheck %s --check-prefix=BIT
// RUN: %domreport %s --function=stepped --lower-structured --index-bit-width=8 --scope=0:1:5 --max-level=0 --candidates-per-group=1 --rlimit=2000000 --timeout-ms=3000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 1 and 2 are separate" | FileCheck %s --check-prefix=STEP
// RUN: %domreport %s --function=stepped --lower-structured --index-bit-width=32 --scope=0:1:5 --max-level=0 --candidates-per-group=1 --rlimit=2000000 --timeout-ms=3000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 1 and 2 are separate" | FileCheck %s --check-prefix=STEP
// The flat proposer's congruence atoms, y mod 2, 3 and 4, exist only where the level's
// width holds the modulus. Here the search proposes a one-bit comparison as a coordinate,
// and the proposer once built APInt(1, 2) for it, which aborted a debug build before any
// report was printed; now it reports, and refuses the families as it did before.
// BIT: "gate": "OPEN"
// BIT: "search": "CANDIDATE_FOUND"
// BIT: "authorizes_lowering": false
// BIT: "proposal_reason": "no guard from the program's conditions is exact at the samples"
// A step of two at eight and thirty-two bits: the congruence i mod 2 = 0 is what makes
// the box exact, and the proposal verifies on both routes.
// STEP: "domain_symbolic": {
// STEP: "reason": "D1 to D3 discharged over the scope"
// STEP: "status": "VERIFIED"
// STEP: "request": {
// STEP: "op": "urem"
// STEP: "theorem13": {
// STEP: "status": "VERIFIED"
// STEP-NEXT: "tier": "SYMBOLIC"
func.func @one_bit_level(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %d: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  scf.for %i = %c0 to %n step %c1 {
    %x = memref.load %a[%i] : memref<8xi32>
    %y = memref.load %b[%i] : memref<8xi32>
    %z = memref.load %c[%i] : memref<8xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<8xi32>
  }
  scf.for %j = %c0 to %n step %c1 {
    %x2 = memref.load %a[%j] : memref<8xi32>
    %y2 = memref.load %d[%j] : memref<8xi32>
    %z2 = memref.load %c[%j] : memref<8xi32>
    %m2 = arith.muli %x2, %y2 : i32
    %s2 = arith.addi %z2, %m2 : i32
    memref.store %s2, %c[%j] : memref<8xi32>
  }
  return
}
func.func @stepped(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  scf.for %i = %c0 to %n step %c2 {
    %x = memref.load %a[%i] : memref<8xi32>
    %y = memref.load %b[%i] : memref<8xi32>
    %z = memref.load %c[%i] : memref<8xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<8xi32>
  }
  return
}
