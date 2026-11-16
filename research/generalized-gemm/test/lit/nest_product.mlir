// RUN: %nestreport %s --function=mm --index-bit-width=16 --max-level=1 --candidates-per-group=1 --rlimit=2000000 --timeout-ms=20000 --authorize="roots 0 and 2 are separate,roots 1 and 2 are separate" | FileCheck %s --check-prefix=MM
// RUN: %nestreport %s --function=strict --index-bit-width=16 --max-level=1 --candidates-per-group=1 --rlimit=2000000 --timeout-ms=20000 --authorize="roots 0 and 2 are separate,roots 1 and 2 are separate" | FileCheck %s --check-prefix=STRICT
// RUN: %nestreport %s --function=mmi --index-bit-width=16 --max-level=1 --candidates-per-group=1 --rlimit=2000000 --timeout-ms=20000 --authorize="roots 0 and 2 are separate,roots 1 and 2 are separate" | FileCheck %s --check-prefix=MMI
// RUN: %nestreport %s --function=mmi --index-bit-width=16 --max-level=1 --candidates-per-group=1 --rlimit=2000000 --timeout-ms=20000 --authorize="roots 1 and 2 are separate" | FileCheck %s --check-prefix=ALIAS
// Stage 1 of docs/GENERALIZED_GEMM_SCOPE.md, case A: one product over a constant box. The
// record gives what a partitioner reads: M, N and K, which loop is I, J and K, each
// operand's root and view, and the update. test/fuzz/nest_affine_compare.py compares these
// fields, for every function of this file, with what the production affine recognizer
// reports for the same text.
//
// In binary32 with reassoc on the accumulation. The ordered fold is verified, which is one
// statement; the source permits another order by its flag, which is another, and no proof:
// the float reorder fact is refuted by a memory at which reversing a cell changes its bits.
// MM: "function": "mm",
// MM-NEXT: "nests": [
// MM: "authorizes_lowering": false,
// MM: "accumulator_first": true,
// MM-NEXT: "element": "f32",
// MM-NEXT: "fold": "ORDERED_FLOAT",
// MM-NEXT: "multiply": "fmul",
// MM: "reduction": "fadd",
// MM: "starts_from": "the value of the cell at the nest's entry",
// MM-NEXT: "statement": "C[h] := C[h] + A[f] * B[g]"
// MM: "axes": {
// MM-NEXT: "I": [
// MM-NEXT: 0
// MM: "J": [
// MM-NEXT: 1
// MM: "K": [
// MM-NEXT: 2
// MM: "box": true,
// MM: "extents": {
// MM-NEXT: "K": 5,
// MM-NEXT: "M": 4,
// MM-NEXT: "N": 3
// MM: "other_writes": 0
// MM: "guard": "true",
// MM: "reduction_flags": [
// MM-NEXT: "reassoc"
// MM: "reorder": {
// MM-NEXT: "float": {
// MM: "status": "REFUTED",
// MM: "theorem13": {
// MM: "logical_tier": "SYMBOLIC",
// MM: "status": "VERIFIED",
// MM-NEXT: "tier": "SYMBOLIC"
// MM: "A": {
// MM: "form": "level0",
// MM: "form": "level2",
// MM: "proved": true,
// MM: "root": 0,
// MM: "B": {
// MM: "form": "level2",
// MM: "form": "level1",
// MM: "root": 1,
// MM: "C": {
// MM: "form": "level0",
// MM: "form": "level1",
// MM: "root": 2,
// MM: "ordered_fold_verified": true,
// MM: "reassociation": {
// MM-NEXT: "basis": "source flag",
// MM: "permitted": true,
// MM-NEXT: "source_flag_reassoc": true
// MM: "reassociation_permitted": true,
// MM-NEXT: "refusals": [],
// MM: "kind": "assumption",
// MM-NEXT: "name": "roots 0 and 2 are separate",
// MM: "used": true
// MM: "status": "VERIFIED"
// MM: "status": "OUTLINED"
//
// The same nest with no flag: the same verified fold, and no permission to reorder it.
// STRICT: "extents": {
// STRICT-NEXT: "K": 5,
// STRICT-NEXT: "M": 4,
// STRICT-NEXT: "N": 3
// STRICT: "reduction_flags": [],
// STRICT: "ordered_fold_verified": true,
// STRICT: "reassociation": {
// STRICT-NEXT: "basis": "none",
// STRICT: "permitted": false,
// STRICT-NEXT: "source_flag_reassoc": false
// STRICT: "reassociation_permitted": false,
// STRICT-NEXT: "refusals": [],
// STRICT: "statement": 0,
// STRICT-NEXT: "status": "VERIFIED"
//
// In 32-bit integers: a wrapping fold, and the order may change by a proof. There is no
// flag to read.
// MMI: "element": "bv32",
// MMI-NEXT: "fold": "MODULAR_INTEGER",
// MMI: "extents": {
// MMI-NEXT: "K": 5,
// MMI-NEXT: "M": 4,
// MMI-NEXT: "N": 3
// MMI: "holds_for": "every run",
// MMI: "ordered_fold_verified": true,
// MMI: "reassociation": {
// MMI-NEXT: "basis": "proof",
// MMI-NEXT: "detail": "wrapping addition with no flag, unobserved, at the computation's tier",
// MMI-NEXT: "permitted": true,
// MMI-NEXT: "source_flag_reassoc": null
// MMI: "refusals": [],
// MMI: "statement": 0,
// MMI-NEXT: "status": "VERIFIED"
//
// Case D: with no name for A against C, A may overlap C. Nothing is assumed: the fold is
// not verified, and the record says which roots are not known apart and what would.
// ALIAS: "authorizes_lowering": false,
// ALIAS: "ordered_fold_verified": false,
// ALIAS: "reassociation_permitted": false,
// ALIAS-NEXT: "refusals": [
// ALIAS-NEXT: "the ordered fold is not verified: Theorem 13 is
// ALIAS-NEXT: "A (root 0) may overlap C (root 2): the two are not known apart. A fresh allocation proves it; the name \"roots 0 and 2 are separate\" assumes it"
// ALIAS: "apart": false,
// ALIAS-NEXT: "basis": "none: the two may overlap",
// ALIAS-NEXT: "kind": "unknown",
// ALIAS-NEXT: "name": "roots 0 and 1 are separate",
// ALIAS: "apart": false,
// ALIAS-NEXT: "basis": "none: the two may overlap",
// ALIAS-NEXT: "kind": "unknown",
// ALIAS-NEXT: "name": "roots 0 and 2 are separate",
// ALIAS: "statement": 0,
// ALIAS-NEXT: "status": "NOT_VERIFIED"
func.func @mm(%a: memref<4x5xf32>, %b: memref<5x3xf32>, %c: memref<4x3xf32>) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 3 {
      affine.for %k = 0 to 5 {
        %x = affine.load %a[%i, %k] : memref<4x5xf32>
        %y = affine.load %b[%k, %j] : memref<5x3xf32>
        %z = affine.load %c[%i, %j] : memref<4x3xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        affine.store %s, %c[%i, %j] : memref<4x3xf32>
      }
    }
  }
  return
}
func.func @strict(%a: memref<4x5xf32>, %b: memref<5x3xf32>, %c: memref<4x3xf32>) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 3 {
      affine.for %k = 0 to 5 {
        %x = affine.load %a[%i, %k] : memref<4x5xf32>
        %y = affine.load %b[%k, %j] : memref<5x3xf32>
        %z = affine.load %c[%i, %j] : memref<4x3xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m : f32
        affine.store %s, %c[%i, %j] : memref<4x3xf32>
      }
    }
  }
  return
}
func.func @mmi(%a: memref<4x5xi32>, %b: memref<5x3xi32>, %c: memref<4x3xi32>) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 3 {
      affine.for %k = 0 to 5 {
        %x = affine.load %a[%i, %k] : memref<4x5xi32>
        %y = affine.load %b[%k, %j] : memref<5x3xi32>
        %z = affine.load %c[%i, %j] : memref<4x3xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        affine.store %s, %c[%i, %j] : memref<4x3xi32>
      }
    }
  }
  return
}
func.func @offsets(%a: memref<8x8xf32>, %b: memref<8x8xf32>, %c: memref<8x8xf32>) {
  affine.for %k = 2 to 4 {
    affine.for %i = 1 to 3 {
      affine.for %j = 0 to 3 {
        %x = affine.load %a[%i + 1, %k] : memref<8x8xf32>
        %y = affine.load %b[%j, %k * 2] : memref<8x8xf32>
        %z = affine.load %c[%i, %j + 2] : memref<8x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        affine.store %s, %c[%i, %j + 2] : memref<8x8xf32>
      }
    }
  }
  return
}
func.func @offsetsi(%a: memref<8x8xi32>, %b: memref<8x8xi32>, %c: memref<8x8xi32>) {
  affine.for %k = 2 to 4 {
    affine.for %i = 1 to 3 {
      affine.for %j = 0 to 3 {
        %x = affine.load %a[%i + 1, %k] : memref<8x8xi32>
        %y = affine.load %b[%j, %k * 2] : memref<8x8xi32>
        %z = affine.load %c[%i, %j + 2] : memref<8x8xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        affine.store %s, %c[%i, %j + 2] : memref<8x8xi32>
      }
    }
  }
  return
}
func.func @allocated(%a: memref<4x5xf32>, %b: memref<5x3xf32>) {
  %c = memref.alloc() : memref<4x3xf32>
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 3 {
      affine.for %k = 0 to 5 {
        %x = affine.load %a[%i, %k] : memref<4x5xf32>
        %y = affine.load %b[%k, %j] : memref<5x3xf32>
        %z = affine.load %c[%i, %j] : memref<4x3xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        affine.store %s, %c[%i, %j] : memref<4x3xf32>
      }
    }
  }
  return
}
func.func @restrict(%a: memref<4x5xf32>, %b: memref<5x3xf32>, %c: memref<4x3xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 3 {
      affine.for %k = 0 to 5 {
        %x = affine.load %a[%i, %k] : memref<4x5xf32>
        %y = affine.load %b[%k, %j] : memref<5x3xf32>
        %z = affine.load %c[%i, %j] : memref<4x3xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        affine.store %s, %c[%i, %j] : memref<4x3xf32>
      }
    }
  }
  return
}
