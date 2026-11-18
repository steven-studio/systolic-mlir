// RUN: %nestreport %s --outline-only | FileCheck %s
// Stage 1 of docs/GENERALIZED_GEMM_SCOPE.md: the copy of a nest that the analyses run on. It
// is a function of its own, in a module of its own, and the input is not changed.
//
// What the nest reads and does not define is an argument of the copy: the two arrays it
// uses, in the function's order; the function's scalar arguments; the value loaded before
// the nest and the value computed before it. Only the constant is copied in. The load
// before the nest is not repeated in the copy: at the nest's entry the cell may hold
// another value, and here it does. The third array, used before the nest only, is no
// input of it.
// CHECK: // function inputs
// CHECK-NEXT: // nest 0 (scf.for, statement 5)
// CHECK-NEXT: module {
// CHECK-NEXT: func.func @inputs__nest0(%[[A:.*]]: memref<8xi32>, %[[C:.*]]: memref<8xi32>, %[[N:.*]]: index, %[[W:.*]]: i32, %[[T:.*]]: index, %[[M:.*]]: index) {
// CHECK-NEXT: %[[ONE:.*]] = arith.constant 1 : index
// CHECK-NEXT: scf.for %[[I:.*]] = %[[T]] to %[[M]] step %[[ONE]] {
// CHECK-NEXT: %[[X:.*]] = memref.load %[[A]][%[[I]]] : memref<8xi32>
// CHECK-NEXT: %[[Y:.*]] = arith.muli %[[X]], %[[W]] : i32
// CHECK-NEXT: %[[Z:.*]] = memref.load %[[C]][%[[I]]] : memref<8xi32>
// CHECK-NEXT: %[[S:.*]] = arith.addi %[[Z]], %[[Y]] : i32
// CHECK-NEXT: memref.store %[[S]], %[[C]][%[[N]]] : memref<8xi32>
// CHECK-NEXT: }
// CHECK-NEXT: return
// CHECK-NEXT: }
// CHECK-NEXT: }
//
// Two nests of one function: two copies, each with its own arrays. An argument's
// llvm.noalias goes with it. The allocated array is an argument of the copy like any
// other, with no attribute: that it is apart from the others is handed over by name.
// CHECK: // function two
// CHECK-NEXT: // nest 0 (affine.for, statement 1)
// CHECK-NEXT: module {
// CHECK-NEXT: func.func @two__nest0(%[[A:.*]]: memref<4xi32> {llvm.noalias}, %[[T:.*]]: memref<4xi32>) {
// CHECK-NEXT: affine.for %[[I:.*]] = 0 to 4 {
// CHECK-NEXT: %[[X:.*]] = affine.load %[[A]][%[[I]]] : memref<4xi32>
// CHECK-NEXT: affine.store %[[X]], %[[T]][%[[I]]] : memref<4xi32>
// CHECK-NEXT: }
// CHECK-NEXT: return
// CHECK: // nest 1 (affine.for, statement 2)
// CHECK-NEXT: module {
// CHECK-NEXT: func.func @two__nest1(%[[B:.*]]: memref<4xi32>, %[[T:.*]]: memref<4xi32>) {
// CHECK-NEXT: affine.for %[[J:.*]] = 0 to 4 {
// CHECK-NEXT: %[[Y:.*]] = affine.load %[[T]][%[[J]]] : memref<4xi32>
// CHECK-NEXT: affine.store %[[Y]], %[[B]][%[[J]]] : memref<4xi32>
//
// What has no faithful copy is refused in words.
// CHECK: // function carried
// CHECK-NEXT: // nest 0 (scf.for, statement 4): no copy: the loop yields a value that is used after it (result 0 of scf.for): the nest's effect is not on memory alone
// CHECK: // function viewed
// CHECK-NEXT: // nest 0 (affine.for, statement 1): no copy: the nest uses a memref that is the result of memref.cast, which is no root of the function
// CHECK: // function branches: not outlined: the function's control flow is by branches (its body has more than one block): only loops written as operations are outlined
func.func @inputs(%a: memref<8xi32>, %c: memref<8xi32>, %p: memref<2xindex>, %n: index, %w: i32) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %m = arith.addi %n, %c1 : index
  %t = memref.load %p[%c0] : memref<2xindex>
  memref.store %c1, %p[%c0] : memref<2xindex>
  scf.for %i = %t to %m step %c1 {
    %x = memref.load %a[%i] : memref<8xi32>
    %y = arith.muli %x, %w : i32
    %z = memref.load %c[%i] : memref<8xi32>
    %s = arith.addi %z, %y : i32
    memref.store %s, %c[%n] : memref<8xi32>
  }
  return
}
func.func @two(%a: memref<4xi32> {llvm.noalias}, %b: memref<4xi32>) {
  %t = memref.alloc() : memref<4xi32>
  affine.for %i = 0 to 4 {
    %x = affine.load %a[%i] : memref<4xi32>
    affine.store %x, %t[%i] : memref<4xi32>
  }
  affine.for %j = 0 to 4 {
    %y = affine.load %t[%j] : memref<4xi32>
    affine.store %y, %b[%j] : memref<4xi32>
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
func.func @viewed(%a: memref<4xi32>, %b: memref<4xi32>) {
  %v = memref.cast %a : memref<4xi32> to memref<?xi32>
  affine.for %i = 0 to 4 {
    %x = affine.load %v[%i] : memref<?xi32>
    affine.store %x, %b[%i] : memref<4xi32>
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
