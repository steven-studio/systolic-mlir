// RUN: systolic-opt --affine-matmul-to-systolic="general-affine=true report=true" %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=REPLACE
// RUN: systolic-opt --affine-matmul-to-systolic="general-affine=true assume-no-alias=true" %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ASSUME
//
// Phase 5.4.3d-3b-5: without a declared fleet the fast path lowers nothing,
// and a nest the symbolic recognizer proves is replaced, in place, by its
// exact guarded MAC nest (affine.for over the logical box, scf.if exact
// Presburger guard, memref loads, mulf/addf in the source order, memref
// store). A nest whose buffers are not provably disjoint stays as written
// unless the production assume-no-alias option is given. Operations around
// a replaced nest keep their order; a nest the emitters refuse after the
// preflight accepted it stays as written (the transaction leaves no partial
// IR).
//
// The remarks (report mode) name the decision for every nest; they precede
// the printed module.
// REPLACE-DAG: remark: affine GEMM replaced by its exact guarded MAC nest
// REPLACE-DAG: remark: guarded MAC replacement not applied: UNKNOWN: A may alias C
// REPLACE-DAG: remark: guarded MAC replacement not applied: UNKNOWN: fast-math flags
// REPLACE-DAG: remark: guarded MAC replacement not applied: REFUSED: guarded memory loads emission [VALID_BUT_LOWERING_UNSUPPORTED]
module {
  // Skewed accesses: C[2i+j][j] += A[2i+j][k] * B[k][j], a non-unimodular
  // coordinate map (det 2) with one congruence.
  // REPLACE-LABEL: func.func @stretch(
  // REPLACE:      affine.for %{{.*}} = 0 to 10 {
  // REPLACE:        affine.for %{{.*}} = 0 to 4 {
  // REPLACE:          affine.for %{{.*}} = 0 to 4 {
  // REPLACE:            scf.if
  // REPLACE:              memref.load %arg1
  // REPLACE:              memref.load %arg2
  // REPLACE:              memref.load %arg0
  // REPLACE:              arith.mulf
  // REPLACE:              arith.addf
  // REPLACE:              memref.store {{.*}}, %arg0
  // REPLACE-NOT:        affine.load
  // REPLACE:      return
  func.func @stretch(%C: memref<16x4xf32> {llvm.noalias}, %A: memref<16x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}) {
    affine.for %i = 0 to 4 { affine.for %j = 0 to 4 { affine.for %k = 0 to 4 {
      %c = affine.load %C[2 * %i + %j, %j] : memref<16x4xf32>
      %a = affine.load %A[2 * %i + %j, %k] : memref<16x4xf32>
      %b = affine.load %B[%k, %j] : memref<4x4xf32>
      %p = arith.mulf %a, %b : f32
      %s = arith.addf %c, %p : f32
      affine.store %s, %C[2 * %i + %j, %j] : memref<16x4xf32>
    } } }
    func.return
  }

  // Two nests in one function, with memory operations before, between and
  // after them: each nest is replaced in its own position, the surrounding
  // operations stay where they are, in their order.
  // REPLACE-LABEL: func.func @siblings(
  // REPLACE:        memref.store %cst, %arg0[%c0, %c0]
  // REPLACE-NEXT:   affine.for
  // REPLACE:          scf.if
  // REPLACE:            memref.store {{.*}}, %arg0[
  // REPLACE:        memref.load %arg0[%c0, %c0]
  // REPLACE-NEXT:   memref.store {{.*}}, %arg3[%c0, %c0]
  // REPLACE-NEXT:   affine.for
  // REPLACE:          scf.if
  // REPLACE:            memref.store {{.*}}, %arg3[
  // REPLACE:        memref.store %cst, %arg3[%c0, %c0]
  // REPLACE-NEXT:   return
  // REPLACE-NOT:    affine.load
  func.func @siblings(%C: memref<4x4xf32> {llvm.noalias}, %A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}, %D: memref<4x4xf32> {llvm.noalias}) {
    %c0 = arith.constant 0 : index
    %one = arith.constant 1.0 : f32
    memref.store %one, %C[%c0, %c0] : memref<4x4xf32>
    affine.for %i = 0 to 4 { affine.for %j = 0 to 4 { affine.for %k = 0 to 4 {
      %c = affine.load %C[%i, %j] : memref<4x4xf32>
      %a = affine.load %A[%i, %k] : memref<4x4xf32>
      %b = affine.load %B[%k, %j] : memref<4x4xf32>
      %p = arith.mulf %a, %b : f32
      %s = arith.addf %c, %p : f32
      affine.store %s, %C[%i, %j] : memref<4x4xf32>
    } } }
    %mid = memref.load %C[%c0, %c0] : memref<4x4xf32>
    memref.store %mid, %D[%c0, %c0] : memref<4x4xf32>
    affine.for %i = 0 to 4 { affine.for %j = 0 to 4 { affine.for %k = 0 to 4 {
      %c = affine.load %D[%i, %j] : memref<4x4xf32>
      %a = affine.load %A[%i, %k] : memref<4x4xf32>
      %b = affine.load %B[%k, %j] : memref<4x4xf32>
      %p = arith.mulf %a, %b : f32
      %s = arith.addf %c, %p : f32
      affine.store %s, %D[%i, %j] : memref<4x4xf32>
    } } }
    memref.store %one, %D[%c0, %c0] : memref<4x4xf32>
    func.return
  }

  // The same statement on plain arguments: A and B may alias C.
  // REPLACE-LABEL: func.func @may_alias(
  // REPLACE:        affine.load %arg0
  // REPLACE:        affine.store
  // ASSUME-LABEL: func.func @may_alias(
  // ASSUME:         scf.if
  // ASSUME:         memref.store
  // ASSUME-NOT:     affine.load
  // ASSUME:         return
  func.func @may_alias(%C: memref<16x4xf32>, %A: memref<16x4xf32>, %B: memref<4x4xf32>) {
    affine.for %i = 0 to 4 { affine.for %j = 0 to 4 { affine.for %k = 0 to 4 {
      %c = affine.load %C[2 * %i + %j, %j] : memref<16x4xf32>
      %a = affine.load %A[2 * %i + %j, %k] : memref<16x4xf32>
      %b = affine.load %B[%k, %j] : memref<4x4xf32>
      %p = arith.mulf %a, %b : f32
      %s = arith.addf %c, %p : f32
      affine.store %s, %C[2 * %i + %j, %j] : memref<16x4xf32>
    } } }
    func.return
  }

  // fast-math flags are not mirrored, and no other path takes the nest (a
  // K bound depending on i: not centrally symmetric for the fast path,
  // partial reduction fibers for the certified device lowering, Phase 3b-7
  // section 28): the nest stays as written.
  // REPLACE-LABEL: func.func @relaxed(
  // REPLACE:        affine.load %arg0
  // REPLACE:        arith.mulf {{.*}} fastmath<reassoc>
  func.func @relaxed(%C: memref<4x4xf32> {llvm.noalias}, %A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}) {
    affine.for %i = 0 to 4 { affine.for %j = 0 to 4 { affine.for %k = 0 to affine_map<(d0) -> (d0 + 1)>(%i) {
      %c = affine.load %C[%i, %j] : memref<4x4xf32>
      %a = affine.load %A[%i, %k] : memref<4x4xf32>
      %b = affine.load %B[%k, %j] : memref<4x4xf32>
      %p = arith.mulf %a, %b fastmath<reassoc> : f32
      %s = arith.addf %c, %p fastmath<reassoc> : f32
      affine.store %s, %C[%i, %j] : memref<4x4xf32>
    } } }
    func.return
  }

  // A repetition loop around the nest: the preflight accepts it (the
  // repetition is a batch coordinate), but the emitted schedule would order
  // the batch innermost and change the order of the additions into each C
  // element; the loads emitter refuses at commit and the source stays as
  // written.
  // REPLACE-LABEL: func.func @repeated(
  // REPLACE:        affine.for %{{.*}} = 0 to 2 {
  // REPLACE:          affine.load %arg0
  // REPLACE:          affine.store
  // REPLACE-NOT:      scf.if
  func.func @repeated(%C: memref<4x4xf32> {llvm.noalias}, %A: memref<4x4xf32> {llvm.noalias}, %B: memref<4x4xf32> {llvm.noalias}) {
    affine.for %p = 0 to 2 {
      affine.for %i = 0 to 4 { affine.for %j = 0 to 4 { affine.for %k = 0 to 4 {
        %c = affine.load %C[%i, %j] : memref<4x4xf32>
        %a = affine.load %A[%i, %k] : memref<4x4xf32>
        %b = affine.load %B[%k, %j] : memref<4x4xf32>
        %p0 = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p0 : f32
        affine.store %s, %C[%i, %j] : memref<4x4xf32>
      } } }
    }
    func.return
  }
}
