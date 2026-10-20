// RUN: systolic-opt --systolic-declare-fpga-fleet --affine-matmul-to-systolic=allow-fp-reassociation=true \
// RUN:   --lower-systolic-matmul-tile-to-fpga="async=true" %s | FileCheck %s --check-prefix=ASYNC
// RUN: systolic-opt --systolic-declare-fpga-fleet --affine-matmul-to-systolic=allow-fp-reassociation=true \
// RUN:   --lower-systolic-matmul-tile-to-fpga="async=true" %s | FileCheck %s --check-prefix=NONEG
// RUN: systolic-opt --systolic-declare-fpga-fleet --affine-matmul-to-systolic=allow-fp-reassociation=true \
// RUN:   --lower-systolic-matmul-tile-to-fpga %s | FileCheck %s --check-prefix=SYNC

// C views the asynchronous runtime cannot address in place. fpga_tile_submit
// adds a tile at C[i * ldc + j] and refuses ldc < n (rc -11), so an output
// block is submitted straight into C's buffer only when C is row-major with a
// static row stride between its row length and INT32_MAX. Rows read
// backwards (C[11 - i][j]: strided<[-10, 1]>, also at a run-time offset) and
// a single row whose stride is smaller than its length (the 1 x 3 view of a
// 3 x 1 array) are staged instead: one contiguous M x N buffer per GEMM,
// filled from the view right after the GEMM's begin, every block submitted
// into it with ldc = N, and copied back by the front end's
// materialize_in_destination after the GEMM's one wait. No submit takes its
// ldc from such a view (no negative constant, no run-time row stride of
// it). The synchronous lowering stages every block through memref.copy and
// is unchanged (SYNC). @offset_c, a forward view at a static offset, keeps
// the direct path.

// NONEG-NOT: arith.constant -{{[0-9]+}} : i32

// ASYNC-LABEL: func.func @rev_rows
// ASYNC:       %[[CV:.*]] = memref.subview %{{.*}}[11, 0] [12, 10] [-1, 1] : memref<12x10xf32> to memref<12x10xf32, strided<[-10, 1], offset: 110>>
// ASYNC:       %[[ST:.*]] = memref.alloc() : memref<12x10xf32>
// ASYNC-NOT:   memref.extract_aligned_pointer_as_index %[[CV]]
// ASYNC:       memref.extract_aligned_pointer_as_index %[[ST]]
// ASYNC:       llvm.call @fpga_gemm_begin
// ASYNC-NEXT:  memref.copy %[[CV]], %[[ST]]
// ASYNC:       llvm.call @fpga_tile_submit
// ASYNC:       llvm.call @fpga_gemm_wait
// ASYNC-NOT:   llvm.call @fpga_gemm_begin
// ASYNC:       bufferization.materialize_in_destination %{{.*}} in restrict writable %[[CV]]
// SYNC-LABEL:  func.func @rev_rows
// SYNC:        %[[SCV:.*]] = memref.subview %{{.*}}[11, 0] [12, 10] [-1, 1]
// SYNC:        memref.subview %[[SCV]][0, 0] [8, 8] [1, 1] : memref<12x10xf32, strided<[-10, 1], offset: 110>> to memref<8x8xf32, strided<[-10, 1], offset: 110>>
// SYNC:        memref.copy
// SYNC:        llvm.call @fpga_matmul_tiled_auto_scheduled
func.func @rev_rows(%A: memref<12x20xf32> {llvm.noalias}, %B: memref<20x10xf32> {llvm.noalias},
                    %C: memref<12x10xf32> {llvm.noalias}) {
  affine.for %i = 0 to 12 {
    affine.for %j = 0 to 10 {
      affine.for %k = 0 to 20 {
        %a = affine.load %A[%i, %k] : memref<12x20xf32>
        %b = affine.load %B[%k, %j] : memref<20x10xf32>
        %c = affine.load %C[11 - %i, %j] : memref<12x10xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[11 - %i, %j] : memref<12x10xf32>
      }
    }
  }
  return
}

// ASYNC-LABEL: func.func @rev_rows_at
// ASYNC:       %[[DV:.*]] = memref.subview %{{.*}}[%{{.*}}, 0] [8, 16] [-1, 1] : memref<?x16xf32> to memref<8x16xf32, strided<[-16, 1], offset: ?>>
// ASYNC:       %[[DST:.*]] = memref.alloc() : memref<8x16xf32>
// ASYNC-NOT:   memref.extract_strided_metadata %[[DV]]
// ASYNC:       llvm.call @fpga_gemm_begin
// ASYNC-NEXT:  memref.copy %[[DV]], %[[DST]]
// ASYNC:       llvm.call @fpga_tile_submit
// ASYNC:       llvm.call @fpga_gemm_wait
// ASYNC-NOT:   llvm.call @fpga_gemm_begin
// ASYNC:       bufferization.materialize_in_destination %{{.*}} in restrict writable %[[DV]]
func.func @rev_rows_at(%A: memref<8x16xf32> {llvm.noalias}, %B: memref<16x16xf32> {llvm.noalias},
                       %C: memref<?x16xf32> {llvm.noalias}, %r0: i32) {
  %r = arith.index_cast %r0 : i32 to index
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 16 {
      affine.for %k = 0 to 16 {
        %a = affine.load %A[%i, %k] : memref<8x16xf32>
        %b = affine.load %B[%k, %j] : memref<16x16xf32>
        %c = affine.load %C[symbol(%r) - %i, %j] : memref<?x16xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[symbol(%r) - %i, %j] : memref<?x16xf32>
      }
    }
  }
  return
}

// ASYNC-LABEL: func.func @one_row
// ASYNC:       %[[RV:.*]] = memref.transpose
// ASYNC:       %[[RST:.*]] = memref.alloc() : memref<1x3xf32>
// ASYNC-NOT:   memref.extract_aligned_pointer_as_index %[[RV]]
// ASYNC:       llvm.call @fpga_gemm_begin
// ASYNC-NEXT:  memref.copy %[[RV]], %[[RST]]
// ASYNC:       llvm.call @fpga_tile_submit
// ASYNC:       llvm.call @fpga_gemm_wait
func.func @one_row(%A: memref<1x20xf32> {llvm.noalias}, %B: memref<20x3xf32> {llvm.noalias},
                   %C: memref<3x1xf32> {llvm.noalias}) {
  affine.for %i = 0 to 1 {
    affine.for %j = 0 to 3 {
      affine.for %k = 0 to 20 {
        %a = affine.load %A[%i, %k] : memref<1x20xf32>
        %b = affine.load %B[%k, %j] : memref<20x3xf32>
        %c = affine.load %C[%j, %i] : memref<3x1xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%j, %i] : memref<3x1xf32>
      }
    }
  }
  return
}

// ASYNC-LABEL: func.func @offset_c
// ASYNC:       %[[OV:.*]] = memref.subview %{{.*}}[2, 3] [12, 10] [1, 1] : memref<15x14xf32> to memref<12x10xf32, strided<[14, 1], offset: 31>>
// ASYNC-NOT:   memref.alloc
// ASYNC:       memref.extract_aligned_pointer_as_index %[[OV]]
// ASYNC:       llvm.call @fpga_gemm_begin
// ASYNC-NOT:   llvm.call @fpga_gemm_begin
// ASYNC:       llvm.call @fpga_gemm_wait
// ASYNC-NOT:   llvm.call @fpga_gemm_begin
// ASYNC:       return
func.func @offset_c(%A: memref<12x20xf32> {llvm.noalias}, %B: memref<20x10xf32> {llvm.noalias},
                    %C: memref<15x14xf32> {llvm.noalias}) {
  affine.for %i = 0 to 12 {
    affine.for %j = 0 to 10 {
      affine.for %k = 0 to 20 {
        %a = affine.load %A[%i, %k] : memref<12x20xf32>
        %b = affine.load %B[%k, %j] : memref<20x10xf32>
        %c = affine.load %C[%i + 2, %j + 3] : memref<15x14xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i + 2, %j + 3] : memref<15x14xf32>
      }
    }
  }
  return
}

