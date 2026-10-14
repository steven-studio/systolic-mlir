// REQUIRES: cfront-clang
//
// The C GEMM through the C front end and into the existing affine pipeline,
// one RUN line per stage, every stage on the LLVM/MLIR 24 tools:
//
// RUN: %cfront %s -DM=16 -DN=16 -DK=16 -o %t.mlir
// RUN: FileCheck %s --check-prefix=IR --input-file=%t.mlir
// RUN: systolic-opt %t.mlir -o /dev/null
// RUN: systolic-opt --systolic-declare-fpga-fleet --test-affine-matmul-discovery %t.mlir -o /dev/null 2>&1 \
// RUN:   | FileCheck %s --check-prefix=PART
// RUN: systolic-opt --systolic-declare-fpga-fleet --affine-matmul-to-systolic %t.mlir \
// RUN:   | FileCheck %s --check-prefix=SYS
// RUN: systolic-opt --systolic-declare-fpga-fleet --affine-matmul-to-systolic \
// RUN:   --lower-systolic-matmul-tile-to-fpga --systolic-runtime-to-llvm %t.mlir \
// RUN:   | FileCheck %s --check-prefix=LLVM
// RUN: %cfront %s -DM=20 -DN=12 -DK=40 -o %t.edge.mlir
// RUN: systolic-opt --systolic-declare-fpga-fleet --affine-matmul-to-systolic %t.edge.mlir \
// RUN:   | FileCheck %s --check-prefix=EDGE

void gemm(float A[restrict M][K], float B[restrict K][N],
          float C[restrict M][N]) {
  for (int i = 0; i < M; ++i)
    for (int j = 0; j < N; ++j)
      for (int k = 0; k < K; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

// What the front end emits: C's adjusted parameter types (float (*restrict)[K])
// as memref<?xKxf32> {llvm.noalias}, the loops as affine.for over their
// static bounds, and C[i][j] += A[i][k] * B[k][j] as C defines it:
// C[i][j] = C[i][j] + (A[i][k] * B[k][j]), C[i][j] read once.
//
// IR:      func.func @gemm(%A: memref<?x16xf32> {llvm.noalias} loc("gemm.c":20:11), %B: memref<?x16xf32> {llvm.noalias} loc("gemm.c":20:{{[0-9]+}}), %C: memref<?x16xf32> {llvm.noalias} loc("gemm.c":21:11))
// IR-NEXT:   affine.for %i = 0 to 16 {
// IR-NEXT:     affine.for %j = 0 to 16 {
// IR-NEXT:       affine.for %k = 0 to 16 {
// IR-NEXT:         %[[C:.*]] = affine.load %C[%i, %j] : memref<?x16xf32> loc("gemm.c":25:9)
// IR-NEXT:         %[[A:.*]] = affine.load %A[%i, %k] : memref<?x16xf32> loc("gemm.c":25:20)
// IR-NEXT:         %[[B:.*]] = affine.load %B[%k, %j] : memref<?x16xf32> loc("gemm.c":25:30)
// IR-NEXT:         %[[P:.*]] = arith.mulf %[[A]], %[[B]] : f32
// IR-NEXT:         %[[S:.*]] = arith.addf %[[C]], %[[P]] : f32
// IR-NEXT:         affine.store %[[S]], %C[%i, %j] : memref<?x16xf32> loc("gemm.c":25:9)
// IR-NEXT:       } loc("gemm.c":24:7)
// IR-NEXT:     } loc("gemm.c":23:5)
// IR-NEXT:   } loc("gemm.c":22:3)
// IR-NEXT:   func.return
//
// The existing recognizer and partitioner on that IR.
//
// PART:      recognition result: success
// PART:      row extent: static 16
// PART:      column extent: static 16
// PART:      reduction extent: static 16
// PART:      dense logical GEMM: yes, I/J/K on loops 0/1/2
// PART:      fleet partition: 10 regions, exact
//
// SYS-LABEL: func.func @gemm
// SYS-NOT:   affine.for
// SYS-COUNT-2: systolic.matmul_tile {{.*}} m = 8 n = 8 k = 16 on @acc_8x8_0
// SYS-COUNT-8: systolic.matmul_tile {{.*}} m = 4 n = 4 k = 16 on @acc_4x4_
// SYS-NOT:   systolic.matmul_tile
// SYS:       bufferization.materialize_in_destination
//
// LLVM:      llvm.func @gemm(
// LLVM-COUNT-10: llvm.call @fpga_matmul_tiled_auto_scheduled
// LLVM-NOT:  llvm.call @fpga_matmul_tiled_auto_scheduled
// LLVM-NOT:  affine.
//
// Edge tiles and K-folding from the same source: 20x12x40 is 27 invocations.
//
// EDGE-COUNT-27: systolic.matmul_tile {{.*}} on @acc_
// EDGE-NOT:  systolic.matmul_tile
