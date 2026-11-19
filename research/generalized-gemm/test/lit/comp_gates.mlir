// RUN: %compreport %s --function=gemm --lower-structured --index-bit-width=8 --scope=0:1:2 --no-search --rlimit=400000 --timeout-ms=2000 | FileCheck %s --check-prefix=NOALIAS
// RUN: %compreport %s --function=restrict --lower-structured --index-bit-width=8 --scope=0:1:2 --no-search --rlimit=400000 --timeout-ms=2000 | FileCheck %s --check-prefix=NOALIAS
// RUN: %compreport %s --function=gemm --lower-structured --index-bit-width=8 --scope=0:1:2 --no-search --rlimit=400000 --timeout-ms=0 --authorize="roots 0 and 2 are separate,roots 1 and 2 are separate" | FileCheck %s --check-prefix=ZERO
// RUN: %compreport %s --function=gemm --lower-structured --index-bit-width=8 --scope=0:1:2 --no-search --rlimit=400000 --timeout-ms=2000 --no-certificate --authorize="roots 0 and 2 are separate,roots 1 and 2 are separate" | FileCheck %s --check-prefix=NOPROOF
// RUN: %compreport %s --function=accumulator --lower-structured --index-bit-width=8 --no-search --rlimit=400000 --timeout-ms=2000 | FileCheck %s --check-prefix=SCALAR
// RUN: %compreport %s --function=returned --lower-structured --index-bit-width=8 --scope=0:1:2 --no-search --rlimit=400000 --timeout-ms=2000 --authorize="roots 0 and 2 are separate,roots 1 and 2 are separate" | FileCheck %s --check-prefix=RETURN
// NOALIAS: "reason": "not authorized: roots 0 and 2 are separate"
// NOALIAS: "status": "UNKNOWN"
// NOALIAS-NOT: "status": "VERIFIED"
// ZERO: "reason": "a solver limit of zero is no limit"
// ZERO-NOT: "tier": "SYMBOLIC"
// NOPROOF: "gate": "UNKNOWN"
// NOPROOF-NOT: "tier": "SYMBOLIC"
// SCALAR: "gate": "UNSUPPORTED"
// SCALAR: "reason": "a scalar accumulator has no exact output view"
// RETURN: "reason": "UNOBSERVABLE: a return value depends on memory"
// RETURN: "reason": "F2 return"
// RETURN-NOT: "status": "VERIFIED"
func.func @gemm(%a: memref<4xi32>, %b: memref<4xi32>, %c: memref<4xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  scf.for %i = %c0 to %n step %c1 {
    %x = memref.load %a[%i] : memref<4xi32>
    %y = memref.load %b[%i] : memref<4xi32>
    %z = memref.load %c[%i] : memref<4xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<4xi32>
  }
  return
}
func.func @restrict(%a: memref<4xi32> {llvm.noalias}, %b: memref<4xi32> {llvm.noalias}, %c: memref<4xi32> {llvm.noalias}, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  scf.for %i = %c0 to %n step %c1 {
    %x = memref.load %a[%i] : memref<4xi32>
    %y = memref.load %b[%i] : memref<4xi32>
    %z = memref.load %c[%i] : memref<4xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<4xi32>
  }
  return
}
func.func @returned(%a: memref<4xi32>, %b: memref<4xi32>, %c: memref<4xi32>, %n: index) -> i32 {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  scf.for %i = %c0 to %n step %c1 {
    %x = memref.load %a[%i] : memref<4xi32>
    %y = memref.load %b[%i] : memref<4xi32>
    %z = memref.load %c[%i] : memref<4xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<4xi32>
  }
  %r = memref.load %c[%c0] : memref<4xi32>
  return %r : i32
}
func.func @accumulator(%a: memref<4xi32>, %b: memref<4xi32>, %c: memref<1xi32>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c4 = arith.constant 4 : index
  %zero = arith.constant 0 : i32
  %t = scf.for %k = %c0 to %c4 step %c1 iter_args(%v = %zero) -> (i32) {
    %x = memref.load %a[%k] : memref<4xi32>
    %y = memref.load %b[%k] : memref<4xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %v, %m : i32
    scf.yield %s : i32
  }
  memref.store %t, %c[%c0] : memref<1xi32>
  return
}
