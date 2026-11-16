// RUN: %nestreport %s --function=symbolic --index-bit-width=16 --scope=3:1:2 --max-level=1 --candidates-per-group=1 --rlimit=2000000 --timeout-ms=20000 --authorize="roots 0 and 2 are separate,roots 1 and 2 are separate" | FileCheck %s --check-prefix=SYMBOLIC
// RUN: %nestreport %s --function=square --index-bit-width=16 --max-level=1 --candidates-per-group=1 --rlimit=2000000 --timeout-ms=20000 --authorize="roots 0 and 2 are separate,roots 1 and 2 are separate" | FileCheck %s --check-prefix=SQUARE
// RUN: %nestreport %s --function=fresh --index-bit-width=16 --max-level=1 --candidates-per-group=1 --rlimit=2000000 --timeout-ms=20000 | FileCheck %s --check-prefix=FRESH
// RUN: %nestreport %s --function=carried --index-bit-width=16 | FileCheck %s --check-prefix=CARRIED
// RUN: %nestreport %s --function=calls --index-bit-width=16 | FileCheck %s --check-prefix=CALLS
// RUN: %nestreport %s --function=branches --index-bit-width=16 | FileCheck %s --check-prefix=BRANCHES
// Stage 1 of docs/GENERALIZED_GEMM_SCOPE.md, case D: what a record refuses, in words. (A may
// overlap C is in nest_product.mlir.)
//
// A bound that is an argument of the function. The ordered fold is verified for every
// value of the argument in scope, and M is no constant: no box for a partitioner. The
// refusal names the argument, and the scope is listed as an assumption.
// SYMBOLIC: "assumptions": [
// SYMBOLIC-NEXT: "argument 3 of the function is in [1, 2]: the scope of every claim",
// SYMBOLIC: "box": true,
// SYMBOLIC: "extents": null,
// SYMBOLIC: "axis": "I",
// SYMBOLIC: "extent": null,
// SYMBOLIC: "upper": "(param:bv16 0); it reads parameter 0 (argument 3 of the function)",
// SYMBOLIC: "ordered_fold_verified": true,
// SYMBOLIC: "definition": "argument 3",
// SYMBOLIC: "source": "argument",
// SYMBOLIC: "refusals": [
// SYMBOLIC-NEXT: "no constant extent: the upper bound of level 0 (axis I) is (param:bv16 0); it reads parameter 0 (argument 3 of the function)"
// SYMBOLIC-NEXT: ],
// SYMBOLIC: "status": "VERIFIED_NOT_MAPPED"
//
// A view that is not affine: A[i * i, k]. The fold is verified and the view is proved,
// and it is no view of Definition 4.5.
// SQUARE: "extents": {
// SQUARE: "A": {
// SQUARE-NEXT: "affine": false,
// SQUARE: "affine": false,
// SQUARE: "proved": true,
// SQUARE: "ordered_fold_verified": true,
// SQUARE: "refusals": [
// SQUARE-NEXT: "dimension 0 of the view of A is not affine: (mul:bv16
// SQUARE-NEXT: ],
// SQUARE: "status": "VERIFIED_NOT_MAPPED"
//
// A root allocated in the function before the nest. It is apart from the operands by a
// proof, which is handed to the nest's claim: nothing is authorized, and the record has
// no assumption about roots. The nest that sets it to zero and the nest that copies it
// out have no product, and say so; the three meet on the allocated root, which is listed.
// FRESH: "dependences": [
// FRESH: "earlier": 0,
// FRESH: "earlier_root": 3,
// FRESH-NEXT: "later": 1,
// FRESH: "relation": "same root"
// FRESH: "nests": [
// FRESH: "gemm": null,
// FRESH: "nest": 0,
// FRESH: "refusals": [
// FRESH-NEXT: "the nest has no multiply-accumulate site"
// FRESH: "status": "NO_GEMM"
// FRESH: "assumptions": [
// FRESH-NEXT: "index values are bit vectors of width 16"
// FRESH-NEXT: ],
// FRESH: "extents": {
// FRESH-NEXT: "K": 4,
// FRESH-NEXT: "M": 3,
// FRESH-NEXT: "N": 3
// FRESH: "C": {
// FRESH: "origin": "allocation",
// FRESH-NEXT: "root": 3,
// FRESH: "nest": 1,
// FRESH: "ordered_fold_verified": true,
// FRESH: "refusals": [],
// FRESH: "apart": true,
// FRESH-NEXT: "basis": "fresh allocation",
// FRESH-NEXT: "kind": "proof",
// FRESH-NEXT: "name": "roots 0 and 3 are separate",
// FRESH: "used": true
// FRESH: "apart": true,
// FRESH-NEXT: "basis": "fresh allocation",
// FRESH-NEXT: "kind": "proof",
// FRESH-NEXT: "name": "roots 1 and 3 are separate",
// FRESH: "used": true
// FRESH: "status": "VERIFIED"
// FRESH: "nest": 2,
// FRESH: "status": "NO_GEMM"
// FRESH: "order": [
// FRESH: "kind": "allocation",
// FRESH: "origin": "allocation",
//
// A loop that carries a value out: its effect is not on memory alone, and it has no copy.
// CARRIED: "authorizes_lowering": false,
// CARRIED: "gemm": null,
// CARRIED: "status": "NOT_IMPORTED",
// CARRIED: "ordered_fold_verified": false,
// CARRIED: "reason": "the loop yields a value that is used after it (result 0 of scf.for): the nest's effect is not on memory alone",
// CARRIED: "status": "UNSUPPORTED"
// CARRIED: "refusals": [
// CARRIED-NEXT: "the nest has no faithful copy: the loop yields a value that is used after it
// CARRIED: "status": "UNSUPPORTED"
//
// A nest that calls a function has no copy, and may meet any other nest.
// CALLS: "dependences": [
// CALLS: "relation": "unknown effects"
// CALLS: "reason": "the nest calls a function (func.call), and a callee is not outlined",
// CALLS: "status": "UNSUPPORTED"
//
// A function whose loops are branches has no nest to name.
// BRANCHES: "function": "branches",
// BRANCHES-NEXT: "nests": [],
// BRANCHES: "reason": "the function's control flow is by branches (its body has more than one block): only loops written as operations are outlined",
// BRANCHES: "status": "UNSUPPORTED"
func.func @symbolic(%a: memref<8x8xi32>, %b: memref<8x8xi32>, %c: memref<8x8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c3 = arith.constant 3 : index
  scf.for %i = %c0 to %n step %c1 {
    scf.for %j = %c0 to %c3 step %c1 {
      scf.for %k = %c0 to %c3 step %c1 {
        %x = memref.load %a[%i, %k] : memref<8x8xi32>
        %y = memref.load %b[%k, %j] : memref<8x8xi32>
        %z = memref.load %c[%i, %j] : memref<8x8xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        memref.store %s, %c[%i, %j] : memref<8x8xi32>
      }
    }
  }
  return
}
func.func @square(%a: memref<8x4xi32>, %b: memref<4x3xi32>, %c: memref<3x3xi32>) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 3 {
      affine.for %k = 0 to 4 {
        %q = arith.muli %i, %i : index
        %x = memref.load %a[%q, %k] : memref<8x4xi32>
        %y = affine.load %b[%k, %j] : memref<4x3xi32>
        %z = affine.load %c[%i, %j] : memref<3x3xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        affine.store %s, %c[%i, %j] : memref<3x3xi32>
      }
    }
  }
  return
}
func.func @fresh(%a: memref<3x4xi32>, %b: memref<4x3xi32>, %out: memref<3x3xi32>) {
  %c = memref.alloc() : memref<3x3xi32>
  %zero = arith.constant 0 : i32
  affine.for %i0 = 0 to 3 {
    affine.for %j0 = 0 to 3 {
      affine.store %zero, %c[%i0, %j0] : memref<3x3xi32>
    }
  }
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 3 {
      affine.for %k = 0 to 4 {
        %x = affine.load %a[%i, %k] : memref<3x4xi32>
        %y = affine.load %b[%k, %j] : memref<4x3xi32>
        %z = affine.load %c[%i, %j] : memref<3x3xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        affine.store %s, %c[%i, %j] : memref<3x3xi32>
      }
    }
  }
  affine.for %i2 = 0 to 3 {
    affine.for %j2 = 0 to 3 {
      %v = affine.load %c[%i2, %j2] : memref<3x3xi32>
      affine.store %v, %out[%i2, %j2] : memref<3x3xi32>
    }
  }
  return
}
func.func @carried(%a: memref<4xi32>, %b: memref<4xi32>) -> i32 {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c4 = arith.constant 4 : index
  %zero = arith.constant 0 : i32
  %sum = scf.for %k = %c0 to %c4 step %c1 iter_args(%acc = %zero) -> (i32) {
    %x = memref.load %a[%k] : memref<4xi32>
    %y = memref.load %b[%k] : memref<4xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %acc, %m : i32
    scf.yield %s : i32
  }
  return %sum : i32
}
func.func private @callee(%a: memref<4xi32>)
func.func @calls(%a: memref<4xi32>, %b: memref<4xi32>) {
  affine.for %i = 0 to 4 {
    func.call @callee(%a) : (memref<4xi32>) -> ()
  }
  affine.for %j = 0 to 4 {
    %x = affine.load %a[%j] : memref<4xi32>
    affine.store %x, %b[%j] : memref<4xi32>
  }
  return
}
func.func @branches(%a: memref<4xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  cf.br ^head(%c0 : index)
^head(%i: index):
  %more = arith.cmpi ult, %i, %n : index
  cf.cond_br %more, ^body, ^exit
^body:
  %c1 = arith.constant 1 : index
  %next = arith.addi %i, %c1 : index
  cf.br ^head(%next : index)
^exit:
  return
}
