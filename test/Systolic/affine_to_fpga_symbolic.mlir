// RUN: systolic-opt --systolic-declare-fpga-fleet --affine-matmul-to-systolic %s \
// RUN:   | FileCheck %s --check-prefix=VIEW
// RUN: systolic-opt --systolic-declare-fpga-fleet --affine-matmul-to-systolic \
// RUN:   --lower-systolic-matmul-tile-to-fpga %s | FileCheck %s --check-prefix=SYNC
// RUN: systolic-opt --systolic-declare-fpga-fleet --affine-matmul-to-systolic \
// RUN:   --lower-systolic-matmul-tile-to-fpga="async=true" %s | FileCheck %s --check-prefix=ASYNC
// RUN: systolic-opt --systolic-declare-fpga-fleet --affine-matmul-to-systolic \
// RUN:   --lower-systolic-matmul-tile-to-fpga="async=true" --systolic-runtime-to-llvm %s \
// RUN:   | FileCheck %s --check-prefix=LLVM

// A GEMM on views placed by the function's parameters: C[r0 + i][j], 8 rows
// at r0 of C and the whole 8x16 A. The affine-to-systolic pass takes the
// view of C as a subview whose row offset is the parameter (a dynamic
// offset); the synchronous lowering stages each output block out of that
// view and back into it (memref.copy handles the layout); the asynchronous
// lowering reads the view's descriptor at run time
// (memref.extract_strided_metadata) and hands every tile a pointer computed
// from its offset and row stride, so the parameter's actual value is what
// the runtime writes to -- nothing is fixed at compile time, nothing is
// truncated. One begin, 5 submits (an 8x8 block on the 8x8 array, four 4x4
// blocks on the 4x4s), one wait after the last submit.

// VIEW-LABEL: func.func @rowoff
// VIEW:       %[[R:.*]] = arith.index_cast %{{.*}} : i32 to index
// VIEW:       %[[CV:.*]] = memref.subview %{{.*}}[%[[R]], 0] [8, 16] [1, 1] : memref<?x16xf32> to memref<8x16xf32, strided<[16, 1], offset: ?>>
// VIEW:       bufferization.to_tensor %[[CV]] restrict writable
// VIEW-COUNT-5: systolic.matmul_tile
// VIEW:       bufferization.materialize_in_destination %{{.*}} in restrict writable %[[CV]]

// SYNC-LABEL: func.func @rowoff
// SYNC:       %[[CV:.*]] = memref.subview %{{.*}}[%{{.*}}, 0] [8, 16] [1, 1] : memref<?x16xf32> to memref<8x16xf32, strided<[16, 1], offset: ?>>
// SYNC:       memref.subview %[[CV]][0, 0] [8, 8] [1, 1] : memref<8x16xf32, strided<[16, 1], offset: ?>> to memref<8x8xf32, strided<[16, 1], offset: ?>>
// SYNC:       memref.copy
// SYNC-COUNT-5: llvm.call @fpga_matmul_tiled_auto_scheduled
// SYNC-NOT:   llvm.call @fpga_matmul_tiled_auto_scheduled

// ASYNC-LABEL: func.func @rowoff
// ASYNC:       %[[CV:.*]] = memref.subview %{{.*}}[%{{.*}}, 0] [8, 16] [1, 1] : memref<?x16xf32> to memref<8x16xf32, strided<[16, 1], offset: ?>>
// ASYNC:       %[[BASE:.*]], %[[OFF:.*]], %{{.*}}:2, %[[STR:.*]]:2 = memref.extract_strided_metadata %[[CV]]
// ASYNC:       %[[P:.*]] = memref.extract_aligned_pointer_as_index %[[BASE]]
// ASYNC:       %[[BYTES:.*]] = arith.muli %[[OFF]], %{{.*}} : index
// ASYNC:       %[[ADDR:.*]] = arith.addi %[[P]], %[[BYTES]] : index
// ASYNC:       %[[I64:.*]] = arith.index_cast %[[ADDR]] : index to i64
// ASYNC:       %[[CP:.*]] = llvm.inttoptr %[[I64]] : i64 to !llvm.ptr
// ASYNC:       %[[LDC:.*]] = arith.index_cast %[[STR]]#0 : index to i32
// ASYNC:       %[[G:.*]] = llvm.call @fpga_gemm_begin
// ASYNC:       llvm.call @fpga_tile_submit(%[[G]], %{{.*}}, %{{.*}}, %{{.*}}, %{{.*}}, %{{.*}}, %{{.*}}, %{{.*}}, %{{.*}}, %[[CP]], %[[LDC]],
// ASYNC-COUNT-4: llvm.call @fpga_tile_submit(%[[G]],
// ASYNC-NOT:   llvm.call @fpga_gemm_wait
// ASYNC:       llvm.call @fpga_gemm_wait(%[[G]])
// ASYNC-NOT:   llvm.call @fpga_tile_submit

// LLVM-LABEL: llvm.func @rowoff
// LLVM:       llvm.call @fpga_gemm_begin
// LLVM-COUNT-5: llvm.call @fpga_tile_submit
// LLVM:       llvm.call @fpga_gemm_wait
// LLVM-NOT:   bufferization.
// LLVM-NOT:   memref.

func.func @rowoff(%A: memref<?x16xf32> {llvm.noalias},
                  %B: memref<?x16xf32> {llvm.noalias},
                  %C: memref<?x16xf32> {llvm.noalias}, %r0: i32) {
  %r = arith.index_cast %r0 : i32 to index
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 16 {
      affine.for %k = 0 to 16 {
        %a = affine.load %A[%i, %k] : memref<?x16xf32>
        %b = affine.load %B[%k, %j] : memref<?x16xf32>
        %c = affine.load %C[%i + symbol(%r), %j] : memref<?x16xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i + symbol(%r), %j] : memref<?x16xf32>
      }
    }
  }
  return
}
