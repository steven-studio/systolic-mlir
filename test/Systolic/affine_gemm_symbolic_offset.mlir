// RUN: systolic-opt --test-affine-matmul-discovery %s -o /dev/null 2>&1 \
// RUN:   | FileCheck %s
//
// A GEMM whose accesses add the nest's symbols (the function's integer
// parameters, as the C front end translates C[r0 + i][c0 + j]): the
// symbol terms are separated from the loop part before recognition, which
// then sees the plain GEMM, and are kept as the operands' run-time offsets.
//
// The discovery reports the candidates it could not recognize first
// (@symbol_times_counter below: a symbol multiplied by a loop counter is
// not an offset), then the recognized GEMM.
// CHECK: candidate count: 2
// CHECK: recognition result: failure
// CHECK: unique logical matmul count: 1
// CHECK: recognition result: success
// CHECK: T[0] = [1 0 0]
// CHECK: T[1] = [0 1 0]
// CHECK: T[2] = [0 0 1]
// CHECK: c = [0 0 0]
// CHECK: symbolic offsets A: ()[s0] -> (s0, 0) over 1 symbol(s)
// CHECK: symbolic offsets B: ()[s0] -> (0, s0) over 1 symbol(s)
// CHECK: symbolic offsets C: ()[s0, s1] -> (s0, s1) over 2 symbol(s)
// CHECK: row extent: static 8
// CHECK: column extent: static 8
// CHECK: reduction extent: static 16
// CHECK: dense logical GEMM: yes, I/J/K on loops 0/1/2

func.func @symbolic(%A: memref<?x16xf32> {llvm.noalias},
                    %B: memref<?x32xf32> {llvm.noalias},
                    %C: memref<?x32xf32> {llvm.noalias},
                    %r0: i32, %c0: i32) {
  %r = arith.index_cast %r0 : i32 to index
  %c = arith.index_cast %c0 : i32 to index
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 16 {
        %a = affine.load %A[%i + symbol(%r), %k] : memref<?x16xf32>
        %b = affine.load %B[%k, %j + symbol(%c)] : memref<?x32xf32>
        %cc = affine.load %C[%i + symbol(%r), %j + symbol(%c)] : memref<?x32xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %cc, %p : f32
        affine.store %s, %C[%i + symbol(%r), %j + symbol(%c)] : memref<?x32xf32>
      }
    }
  }
  return
}

// A symbol multiplied by a loop counter is not an offset: not recognized
// (the "failure" above).
func.func @symbol_times_counter(%A: memref<?x16xf32> {llvm.noalias},
                                %B: memref<?x16xf32> {llvm.noalias},
                                %C: memref<?x16xf32> {llvm.noalias},
                                %s: i32) {
  %n = arith.index_cast %s : i32 to index
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 16 {
      affine.for %k = 0 to 16 {
        %a = affine.load %A[%i, %k] : memref<?x16xf32>
        %b = affine.load %B[%k, %j] : memref<?x16xf32>
        %cc = affine.load %C[%i * symbol(%n), %j] : memref<?x16xf32>
        %p = arith.mulf %a, %b : f32
        %sum = arith.addf %cc, %p : f32
        affine.store %sum, %C[%i * symbol(%n), %j] : memref<?x16xf32>
      }
    }
  }
  return
}
