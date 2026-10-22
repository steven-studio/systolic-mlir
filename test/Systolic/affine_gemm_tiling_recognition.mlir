// RUN: systolic-opt --test-affine-matmul-discovery -split-input-file %s \
// RUN:   -o /dev/null 2>&1 | FileCheck %s
//
// S5.1 (docs/AFFINE_GEMM_MAPPING.md section 26.1): the loop tiling a program has ALREADY
// written is recognized and reported -- tile loops, point loops, tile sizes TM / TN / TK, the
// written loop order, the boundary -- and nothing is decided from it (recognition, mapping and
// lowering are unchanged; coverage is not proven at this stage). The expected metadata below was
// computed from each nest's definition by the generator, not taken from the compiler's output:
// for extent E and tile size S the last tile is E - (ceil(E / S) - 1) * S (17 = 8 + 8 + 1).
// Untiled nests and loops that only look like a tiling report no tiling, or "not a tiling" with
// the reason.

// m_only_32: 32x32x32, TM = 8, divisible bounds
// CHECK-LABEL: candidate count
// CHECK: recognition result: success
// CHECK: loop tiling (as written; recorded, not a decision): 1 tiled coordinate(s)
// CHECK-NEXT: loop order: 0=tile(I) 1=point(I) 2=untiled(J) 3=untiled(K)
// CHECK-NEXT: I: tile loop 0, point loop 1, TM = 8, bounds form, boundary divisible (last tile 8), I in [0, 32)
// CHECK-NEXT: tiled dimensions: M; reduction tiling: none
// CHECK-NEXT: coverage of the iteration space: not proven at this stage
func.func @m_only_32(%A: memref<32x32xf32> {llvm.noalias}, %B: memref<32x32xf32> {llvm.noalias}, %C: memref<32x32xf32> {llvm.noalias}) {
  affine.for %ii = 0 to 32 step 8 {
    affine.for %i = affine_map<(d0) -> (d0)>(%ii) to affine_map<(d0) -> (d0 + 8)>(%ii) {
      affine.for %j = 0 to 32 {
        affine.for %k = 0 to 32 {
          %a = affine.load %A[%i, %k] : memref<32x32xf32>
          %b = affine.load %B[%k, %j] : memref<32x32xf32>
          %c = affine.load %C[%i, %j] : memref<32x32xf32>
          %p = arith.mulf %a, %b fastmath<reassoc> : f32
          %s = arith.addf %c, %p fastmath<reassoc> : f32
          affine.store %s, %C[%i, %j] : memref<32x32xf32>
        }
      }
    }
  }
  return
}

// -----

// n_only_32: 32x32x32, TN = 8, divisible bounds
// CHECK-LABEL: candidate count
// CHECK: recognition result: success
// CHECK: loop tiling (as written; recorded, not a decision): 1 tiled coordinate(s)
// CHECK-NEXT: loop order: 0=untiled(I) 1=tile(J) 2=point(J) 3=untiled(K)
// CHECK-NEXT: J: tile loop 1, point loop 2, TN = 8, bounds form, boundary divisible (last tile 8), J in [0, 32)
// CHECK-NEXT: tiled dimensions: N; reduction tiling: none
// CHECK-NEXT: coverage of the iteration space: not proven at this stage
func.func @n_only_32(%A: memref<32x32xf32> {llvm.noalias}, %B: memref<32x32xf32> {llvm.noalias}, %C: memref<32x32xf32> {llvm.noalias}) {
  affine.for %i = 0 to 32 {
    affine.for %jj = 0 to 32 step 8 {
      affine.for %j = affine_map<(d0) -> (d0)>(%jj) to affine_map<(d0) -> (d0 + 8)>(%jj) {
        affine.for %k = 0 to 32 {
          %a = affine.load %A[%i, %k] : memref<32x32xf32>
          %b = affine.load %B[%k, %j] : memref<32x32xf32>
          %c = affine.load %C[%i, %j] : memref<32x32xf32>
          %p = arith.mulf %a, %b fastmath<reassoc> : f32
          %s = arith.addf %c, %p fastmath<reassoc> : f32
          affine.store %s, %C[%i, %j] : memref<32x32xf32>
        }
      }
    }
  }
  return
}

// -----

// k_only_32: 32x32x32, TK = 16, divisible bounds
// CHECK-LABEL: candidate count
// CHECK: recognition result: success
// CHECK: loop tiling (as written; recorded, not a decision): 1 tiled coordinate(s)
// CHECK-NEXT: loop order: 0=untiled(I) 1=untiled(J) 2=tile(K) 3=point(K)
// CHECK-NEXT: K: tile loop 2, point loop 3, TK = 16, bounds form, boundary divisible (last tile 16), K in [0, 32)
// CHECK-NEXT: tiled dimensions: none; reduction tiling: K
// CHECK-NEXT: coverage of the iteration space: not proven at this stage
func.func @k_only_32(%A: memref<32x32xf32> {llvm.noalias}, %B: memref<32x32xf32> {llvm.noalias}, %C: memref<32x32xf32> {llvm.noalias}) {
  affine.for %i = 0 to 32 {
    affine.for %j = 0 to 32 {
      affine.for %kk = 0 to 32 step 16 {
        affine.for %k = affine_map<(d0) -> (d0)>(%kk) to affine_map<(d0) -> (d0 + 16)>(%kk) {
          %a = affine.load %A[%i, %k] : memref<32x32xf32>
          %b = affine.load %B[%k, %j] : memref<32x32xf32>
          %c = affine.load %C[%i, %j] : memref<32x32xf32>
          %p = arith.mulf %a, %b fastmath<reassoc> : f32
          %s = arith.addf %c, %p fastmath<reassoc> : f32
          affine.store %s, %C[%i, %j] : memref<32x32xf32>
        }
      }
    }
  }
  return
}

// -----

// mn_64: 64x64x64, TM = 8, TN = 8, divisible bounds
// CHECK-LABEL: candidate count
// CHECK: recognition result: success
// CHECK: loop tiling (as written; recorded, not a decision): 2 tiled coordinate(s)
// CHECK-NEXT: loop order: 0=tile(I) 1=tile(J) 2=point(I) 3=point(J) 4=untiled(K)
// CHECK-NEXT: I: tile loop 0, point loop 2, TM = 8, bounds form, boundary divisible (last tile 8), I in [0, 64)
// CHECK-NEXT: J: tile loop 1, point loop 3, TN = 8, bounds form, boundary divisible (last tile 8), J in [0, 64)
// CHECK-NEXT: tiled dimensions: M N; reduction tiling: none
// CHECK-NEXT: coverage of the iteration space: not proven at this stage
func.func @mn_64(%A: memref<64x64xf32> {llvm.noalias}, %B: memref<64x64xf32> {llvm.noalias}, %C: memref<64x64xf32> {llvm.noalias}) {
  affine.for %ii = 0 to 64 step 8 {
    affine.for %jj = 0 to 64 step 8 {
      affine.for %i = affine_map<(d0) -> (d0)>(%ii) to affine_map<(d0) -> (d0 + 8)>(%ii) {
        affine.for %j = affine_map<(d0) -> (d0)>(%jj) to affine_map<(d0) -> (d0 + 8)>(%jj) {
          affine.for %k = 0 to 64 {
            %a = affine.load %A[%i, %k] : memref<64x64xf32>
            %b = affine.load %B[%k, %j] : memref<64x64xf32>
            %c = affine.load %C[%i, %j] : memref<64x64xf32>
            %p = arith.mulf %a, %b fastmath<reassoc> : f32
            %s = arith.addf %c, %p fastmath<reassoc> : f32
            affine.store %s, %C[%i, %j] : memref<64x64xf32>
          }
        }
      }
    }
  }
  return
}

// -----

// mnk_32: 32x32x32, TM = 8, TN = 8, TK = 16, divisible bounds
// CHECK-LABEL: candidate count
// CHECK: recognition result: success
// CHECK: loop tiling (as written; recorded, not a decision): 3 tiled coordinate(s)
// CHECK-NEXT: loop order: 0=tile(I) 1=tile(J) 2=tile(K) 3=point(I) 4=point(J) 5=point(K)
// CHECK-NEXT: I: tile loop 0, point loop 3, TM = 8, bounds form, boundary divisible (last tile 8), I in [0, 32)
// CHECK-NEXT: J: tile loop 1, point loop 4, TN = 8, bounds form, boundary divisible (last tile 8), J in [0, 32)
// CHECK-NEXT: K: tile loop 2, point loop 5, TK = 16, bounds form, boundary divisible (last tile 16), K in [0, 32)
// CHECK-NEXT: tiled dimensions: M N; reduction tiling: K
// CHECK-NEXT: coverage of the iteration space: not proven at this stage
func.func @mnk_32(%A: memref<32x32xf32> {llvm.noalias}, %B: memref<32x32xf32> {llvm.noalias}, %C: memref<32x32xf32> {llvm.noalias}) {
  affine.for %ii = 0 to 32 step 8 {
    affine.for %jj = 0 to 32 step 8 {
      affine.for %kk = 0 to 32 step 16 {
        affine.for %i = affine_map<(d0) -> (d0)>(%ii) to affine_map<(d0) -> (d0 + 8)>(%ii) {
          affine.for %j = affine_map<(d0) -> (d0)>(%jj) to affine_map<(d0) -> (d0 + 8)>(%jj) {
            affine.for %k = affine_map<(d0) -> (d0)>(%kk) to affine_map<(d0) -> (d0 + 16)>(%kk) {
              %a = affine.load %A[%i, %k] : memref<32x32xf32>
              %b = affine.load %B[%k, %j] : memref<32x32xf32>
              %c = affine.load %C[%i, %j] : memref<32x32xf32>
              %p = arith.mulf %a, %b fastmath<reassoc> : f32
              %s = arith.addf %c, %p fastmath<reassoc> : f32
              affine.store %s, %C[%i, %j] : memref<32x32xf32>
            }
          }
        }
      }
    }
  }
  return
}

// -----

// mnk_64: 64x64x64, TM = 8, TN = 4, TK = 32, divisible bounds
// CHECK-LABEL: candidate count
// CHECK: recognition result: success
// CHECK: loop tiling (as written; recorded, not a decision): 3 tiled coordinate(s)
// CHECK-NEXT: loop order: 0=tile(I) 1=tile(J) 2=tile(K) 3=point(I) 4=point(J) 5=point(K)
// CHECK-NEXT: I: tile loop 0, point loop 3, TM = 8, bounds form, boundary divisible (last tile 8), I in [0, 64)
// CHECK-NEXT: J: tile loop 1, point loop 4, TN = 4, bounds form, boundary divisible (last tile 4), J in [0, 64)
// CHECK-NEXT: K: tile loop 2, point loop 5, TK = 32, bounds form, boundary divisible (last tile 32), K in [0, 64)
// CHECK-NEXT: tiled dimensions: M N; reduction tiling: K
// CHECK-NEXT: coverage of the iteration space: not proven at this stage
func.func @mnk_64(%A: memref<64x64xf32> {llvm.noalias}, %B: memref<64x64xf32> {llvm.noalias}, %C: memref<64x64xf32> {llvm.noalias}) {
  affine.for %ii = 0 to 64 step 8 {
    affine.for %jj = 0 to 64 step 4 {
      affine.for %kk = 0 to 64 step 32 {
        affine.for %i = affine_map<(d0) -> (d0)>(%ii) to affine_map<(d0) -> (d0 + 8)>(%ii) {
          affine.for %j = affine_map<(d0) -> (d0)>(%jj) to affine_map<(d0) -> (d0 + 4)>(%jj) {
            affine.for %k = affine_map<(d0) -> (d0)>(%kk) to affine_map<(d0) -> (d0 + 32)>(%kk) {
              %a = affine.load %A[%i, %k] : memref<64x64xf32>
              %b = affine.load %B[%k, %j] : memref<64x64xf32>
              %c = affine.load %C[%i, %j] : memref<64x64xf32>
              %p = arith.mulf %a, %b fastmath<reassoc> : f32
              %s = arith.addf %c, %p fastmath<reassoc> : f32
              affine.store %s, %C[%i, %j] : memref<64x64xf32>
            }
          }
        }
      }
    }
  }
  return
}

// -----

// nk_split_32: 32x32x32, TN = 8, divisible bounds
// CHECK-LABEL: candidate count
// CHECK: recognition result: success
// CHECK: loop tiling (as written; recorded, not a decision): 1 tiled coordinate(s)
// CHECK-NEXT: loop order: 0=untiled(I) 1=tile(J) 2=untiled(K) 3=point(J)
// CHECK-NEXT: J: tile loop 1, point loop 3, TN = 8, bounds form, boundary divisible (last tile 8), J in [0, 32)
// CHECK-NEXT: tiled dimensions: N; reduction tiling: none
// CHECK-NEXT: coverage of the iteration space: not proven at this stage
func.func @nk_split_32(%A: memref<32x32xf32> {llvm.noalias}, %B: memref<32x32xf32> {llvm.noalias}, %C: memref<32x32xf32> {llvm.noalias}) {
  affine.for %i = 0 to 32 {
    affine.for %jj = 0 to 32 step 8 {
      affine.for %k = 0 to 32 {
        affine.for %j = affine_map<(d0) -> (d0)>(%jj) to affine_map<(d0) -> (d0 + 8)>(%jj) {
          %a = affine.load %A[%i, %k] : memref<32x32xf32>
          %b = affine.load %B[%k, %j] : memref<32x32xf32>
          %c = affine.load %C[%i, %j] : memref<32x32xf32>
          %p = arith.mulf %a, %b fastmath<reassoc> : f32
          %s = arith.addf %c, %p fastmath<reassoc> : f32
          affine.store %s, %C[%i, %j] : memref<32x32xf32>
        }
      }
    }
  }
  return
}

// -----

// m_only_17: 17x19x23, TM = 8, min boundaries
// CHECK-LABEL: candidate count
// CHECK: recognition result: success
// CHECK: loop tiling (as written; recorded, not a decision): 1 tiled coordinate(s)
// CHECK-NEXT: loop order: 0=tile(I) 1=point(I) 2=untiled(J) 3=untiled(K)
// CHECK-NEXT: I: tile loop 0, point loop 1, TM = 8, bounds form, boundary min-clipped (last tile 1), I in [0, 17)
// CHECK-NEXT: tiled dimensions: M; reduction tiling: none
// CHECK-NEXT: coverage of the iteration space: not proven at this stage
func.func @m_only_17(%A: memref<17x23xf32> {llvm.noalias}, %B: memref<23x19xf32> {llvm.noalias}, %C: memref<17x19xf32> {llvm.noalias}) {
  affine.for %ii = 0 to 17 step 8 {
    affine.for %i = affine_map<(d0) -> (d0)>(%ii) to min affine_map<(d0) -> (d0 + 8, 17)>(%ii) {
      affine.for %j = 0 to 19 {
        affine.for %k = 0 to 23 {
          %a = affine.load %A[%i, %k] : memref<17x23xf32>
          %b = affine.load %B[%k, %j] : memref<23x19xf32>
          %c = affine.load %C[%i, %j] : memref<17x19xf32>
          %p = arith.mulf %a, %b fastmath<reassoc> : f32
          %s = arith.addf %c, %p fastmath<reassoc> : f32
          affine.store %s, %C[%i, %j] : memref<17x19xf32>
        }
      }
    }
  }
  return
}

// -----

// mn_17_19_23: 17x19x23, TM = 8, TN = 8, min boundaries
// CHECK-LABEL: candidate count
// CHECK: recognition result: success
// CHECK: loop tiling (as written; recorded, not a decision): 2 tiled coordinate(s)
// CHECK-NEXT: loop order: 0=tile(I) 1=tile(J) 2=point(I) 3=point(J) 4=untiled(K)
// CHECK-NEXT: I: tile loop 0, point loop 2, TM = 8, bounds form, boundary min-clipped (last tile 1), I in [0, 17)
// CHECK-NEXT: J: tile loop 1, point loop 3, TN = 8, bounds form, boundary min-clipped (last tile 3), J in [0, 19)
// CHECK-NEXT: tiled dimensions: M N; reduction tiling: none
// CHECK-NEXT: coverage of the iteration space: not proven at this stage
func.func @mn_17_19_23(%A: memref<17x23xf32> {llvm.noalias}, %B: memref<23x19xf32> {llvm.noalias}, %C: memref<17x19xf32> {llvm.noalias}) {
  affine.for %ii = 0 to 17 step 8 {
    affine.for %jj = 0 to 19 step 8 {
      affine.for %i = affine_map<(d0) -> (d0)>(%ii) to min affine_map<(d0) -> (d0 + 8, 17)>(%ii) {
        affine.for %j = affine_map<(d0) -> (d0)>(%jj) to min affine_map<(d0) -> (d0 + 8, 19)>(%jj) {
          affine.for %k = 0 to 23 {
            %a = affine.load %A[%i, %k] : memref<17x23xf32>
            %b = affine.load %B[%k, %j] : memref<23x19xf32>
            %c = affine.load %C[%i, %j] : memref<17x19xf32>
            %p = arith.mulf %a, %b fastmath<reassoc> : f32
            %s = arith.addf %c, %p fastmath<reassoc> : f32
            affine.store %s, %C[%i, %j] : memref<17x19xf32>
          }
        }
      }
    }
  }
  return
}

// -----

// mnk_17_19_23: 17x19x23, TM = 8, TN = 8, TK = 16, min boundaries
// CHECK-LABEL: candidate count
// CHECK: recognition result: success
// CHECK: loop tiling (as written; recorded, not a decision): 3 tiled coordinate(s)
// CHECK-NEXT: loop order: 0=tile(I) 1=tile(J) 2=tile(K) 3=point(I) 4=point(J) 5=point(K)
// CHECK-NEXT: I: tile loop 0, point loop 3, TM = 8, bounds form, boundary min-clipped (last tile 1), I in [0, 17)
// CHECK-NEXT: J: tile loop 1, point loop 4, TN = 8, bounds form, boundary min-clipped (last tile 3), J in [0, 19)
// CHECK-NEXT: K: tile loop 2, point loop 5, TK = 16, bounds form, boundary min-clipped (last tile 7), K in [0, 23)
// CHECK-NEXT: tiled dimensions: M N; reduction tiling: K
// CHECK-NEXT: coverage of the iteration space: not proven at this stage
func.func @mnk_17_19_23(%A: memref<17x23xf32> {llvm.noalias}, %B: memref<23x19xf32> {llvm.noalias}, %C: memref<17x19xf32> {llvm.noalias}) {
  affine.for %ii = 0 to 17 step 8 {
    affine.for %jj = 0 to 19 step 8 {
      affine.for %kk = 0 to 23 step 16 {
        affine.for %i = affine_map<(d0) -> (d0)>(%ii) to min affine_map<(d0) -> (d0 + 8, 17)>(%ii) {
          affine.for %j = affine_map<(d0) -> (d0)>(%jj) to min affine_map<(d0) -> (d0 + 8, 19)>(%jj) {
            affine.for %k = affine_map<(d0) -> (d0)>(%kk) to min affine_map<(d0) -> (d0 + 16, 23)>(%kk) {
              %a = affine.load %A[%i, %k] : memref<17x23xf32>
              %b = affine.load %B[%k, %j] : memref<23x19xf32>
              %c = affine.load %C[%i, %j] : memref<17x19xf32>
              %p = arith.mulf %a, %b fastmath<reassoc> : f32
              %s = arith.addf %c, %p fastmath<reassoc> : f32
              affine.store %s, %C[%i, %j] : memref<17x19xf32>
            }
          }
        }
      }
    }
  }
  return
}

// -----

// mnk_31_33_35: 31x33x35, TM = 8, TN = 8, TK = 16, min boundaries
// CHECK-LABEL: candidate count
// CHECK: recognition result: success
// CHECK: loop tiling (as written; recorded, not a decision): 3 tiled coordinate(s)
// CHECK-NEXT: loop order: 0=tile(I) 1=tile(J) 2=tile(K) 3=point(I) 4=point(J) 5=point(K)
// CHECK-NEXT: I: tile loop 0, point loop 3, TM = 8, bounds form, boundary min-clipped (last tile 7), I in [0, 31)
// CHECK-NEXT: J: tile loop 1, point loop 4, TN = 8, bounds form, boundary min-clipped (last tile 1), J in [0, 33)
// CHECK-NEXT: K: tile loop 2, point loop 5, TK = 16, bounds form, boundary min-clipped (last tile 3), K in [0, 35)
// CHECK-NEXT: tiled dimensions: M N; reduction tiling: K
// CHECK-NEXT: coverage of the iteration space: not proven at this stage
func.func @mnk_31_33_35(%A: memref<31x35xf32> {llvm.noalias}, %B: memref<35x33xf32> {llvm.noalias}, %C: memref<31x33xf32> {llvm.noalias}) {
  affine.for %ii = 0 to 31 step 8 {
    affine.for %jj = 0 to 33 step 8 {
      affine.for %kk = 0 to 35 step 16 {
        affine.for %i = affine_map<(d0) -> (d0)>(%ii) to min affine_map<(d0) -> (d0 + 8, 31)>(%ii) {
          affine.for %j = affine_map<(d0) -> (d0)>(%jj) to min affine_map<(d0) -> (d0 + 8, 33)>(%jj) {
            affine.for %k = affine_map<(d0) -> (d0)>(%kk) to min affine_map<(d0) -> (d0 + 16, 35)>(%kk) {
              %a = affine.load %A[%i, %k] : memref<31x35xf32>
              %b = affine.load %B[%k, %j] : memref<35x33xf32>
              %c = affine.load %C[%i, %j] : memref<31x33xf32>
              %p = arith.mulf %a, %b fastmath<reassoc> : f32
              %s = arith.addf %c, %p fastmath<reassoc> : f32
              affine.store %s, %C[%i, %j] : memref<31x33xf32>
            }
          }
        }
      }
    }
  }
  return
}

// -----

// mnk_63_65_67: 63x65x67, TM = 8, TN = 8, TK = 64, min boundaries
// CHECK-LABEL: candidate count
// CHECK: recognition result: success
// CHECK: loop tiling (as written; recorded, not a decision): 3 tiled coordinate(s)
// CHECK-NEXT: loop order: 0=tile(I) 1=tile(J) 2=tile(K) 3=point(I) 4=point(J) 5=point(K)
// CHECK-NEXT: I: tile loop 0, point loop 3, TM = 8, bounds form, boundary min-clipped (last tile 7), I in [0, 63)
// CHECK-NEXT: J: tile loop 1, point loop 4, TN = 8, bounds form, boundary min-clipped (last tile 1), J in [0, 65)
// CHECK-NEXT: K: tile loop 2, point loop 5, TK = 64, bounds form, boundary min-clipped (last tile 3), K in [0, 67)
// CHECK-NEXT: tiled dimensions: M N; reduction tiling: K
// CHECK-NEXT: coverage of the iteration space: not proven at this stage
func.func @mnk_63_65_67(%A: memref<63x67xf32> {llvm.noalias}, %B: memref<67x65xf32> {llvm.noalias}, %C: memref<63x65xf32> {llvm.noalias}) {
  affine.for %ii = 0 to 63 step 8 {
    affine.for %jj = 0 to 65 step 8 {
      affine.for %kk = 0 to 67 step 64 {
        affine.for %i = affine_map<(d0) -> (d0)>(%ii) to min affine_map<(d0) -> (d0 + 8, 63)>(%ii) {
          affine.for %j = affine_map<(d0) -> (d0)>(%jj) to min affine_map<(d0) -> (d0 + 8, 65)>(%jj) {
            affine.for %k = affine_map<(d0) -> (d0)>(%kk) to min affine_map<(d0) -> (d0 + 64, 67)>(%kk) {
              %a = affine.load %A[%i, %k] : memref<63x67xf32>
              %b = affine.load %B[%k, %j] : memref<67x65xf32>
              %c = affine.load %C[%i, %j] : memref<63x65xf32>
              %p = arith.mulf %a, %b fastmath<reassoc> : f32
              %s = arith.addf %c, %p fastmath<reassoc> : f32
              affine.store %s, %C[%i, %j] : memref<63x65xf32>
            }
          }
        }
      }
    }
  }
  return
}

// -----

// mn_4x8: 17x19x23, TM = 4, TN = 8, min boundaries
// CHECK-LABEL: candidate count
// CHECK: recognition result: success
// CHECK: loop tiling (as written; recorded, not a decision): 2 tiled coordinate(s)
// CHECK-NEXT: loop order: 0=tile(I) 1=tile(J) 2=point(I) 3=point(J) 4=untiled(K)
// CHECK-NEXT: I: tile loop 0, point loop 2, TM = 4, bounds form, boundary min-clipped (last tile 1), I in [0, 17)
// CHECK-NEXT: J: tile loop 1, point loop 3, TN = 8, bounds form, boundary min-clipped (last tile 3), J in [0, 19)
// CHECK-NEXT: tiled dimensions: M N; reduction tiling: none
// CHECK-NEXT: coverage of the iteration space: not proven at this stage
func.func @mn_4x8(%A: memref<17x23xf32> {llvm.noalias}, %B: memref<23x19xf32> {llvm.noalias}, %C: memref<17x19xf32> {llvm.noalias}) {
  affine.for %ii = 0 to 17 step 4 {
    affine.for %jj = 0 to 19 step 8 {
      affine.for %i = affine_map<(d0) -> (d0)>(%ii) to min affine_map<(d0) -> (d0 + 4, 17)>(%ii) {
        affine.for %j = affine_map<(d0) -> (d0)>(%jj) to min affine_map<(d0) -> (d0 + 8, 19)>(%jj) {
          affine.for %k = 0 to 23 {
            %a = affine.load %A[%i, %k] : memref<17x23xf32>
            %b = affine.load %B[%k, %j] : memref<23x19xf32>
            %c = affine.load %C[%i, %j] : memref<17x19xf32>
            %p = arith.mulf %a, %b fastmath<reassoc> : f32
            %s = arith.addf %c, %p fastmath<reassoc> : f32
            affine.store %s, %C[%i, %j] : memref<17x19xf32>
          }
        }
      }
    }
  }
  return
}

// -----

// mn_8x4: 17x19x23, TM = 8, TN = 4, min boundaries
// CHECK-LABEL: candidate count
// CHECK: recognition result: success
// CHECK: loop tiling (as written; recorded, not a decision): 2 tiled coordinate(s)
// CHECK-NEXT: loop order: 0=tile(I) 1=tile(J) 2=point(I) 3=point(J) 4=untiled(K)
// CHECK-NEXT: I: tile loop 0, point loop 2, TM = 8, bounds form, boundary min-clipped (last tile 1), I in [0, 17)
// CHECK-NEXT: J: tile loop 1, point loop 3, TN = 4, bounds form, boundary min-clipped (last tile 3), J in [0, 19)
// CHECK-NEXT: tiled dimensions: M N; reduction tiling: none
// CHECK-NEXT: coverage of the iteration space: not proven at this stage
func.func @mn_8x4(%A: memref<17x23xf32> {llvm.noalias}, %B: memref<23x19xf32> {llvm.noalias}, %C: memref<17x19xf32> {llvm.noalias}) {
  affine.for %ii = 0 to 17 step 8 {
    affine.for %jj = 0 to 19 step 4 {
      affine.for %i = affine_map<(d0) -> (d0)>(%ii) to min affine_map<(d0) -> (d0 + 8, 17)>(%ii) {
        affine.for %j = affine_map<(d0) -> (d0)>(%jj) to min affine_map<(d0) -> (d0 + 4, 19)>(%jj) {
          affine.for %k = 0 to 23 {
            %a = affine.load %A[%i, %k] : memref<17x23xf32>
            %b = affine.load %B[%k, %j] : memref<23x19xf32>
            %c = affine.load %C[%i, %j] : memref<17x19xf32>
            %p = arith.mulf %a, %b fastmath<reassoc> : f32
            %s = arith.addf %c, %p fastmath<reassoc> : f32
            affine.store %s, %C[%i, %j] : memref<17x19xf32>
          }
        }
      }
    }
  }
  return
}

// -----

// mk_17_19_23: 17x19x23, TM = 8, TK = 16, min boundaries
// CHECK-LABEL: candidate count
// CHECK: recognition result: success
// CHECK: loop tiling (as written; recorded, not a decision): 2 tiled coordinate(s)
// CHECK-NEXT: loop order: 0=tile(I) 1=tile(K) 2=point(I) 3=untiled(J) 4=point(K)
// CHECK-NEXT: I: tile loop 0, point loop 2, TM = 8, bounds form, boundary min-clipped (last tile 1), I in [0, 17)
// CHECK-NEXT: K: tile loop 1, point loop 4, TK = 16, bounds form, boundary min-clipped (last tile 7), K in [0, 23)
// CHECK-NEXT: tiled dimensions: M; reduction tiling: K
// CHECK-NEXT: coverage of the iteration space: not proven at this stage
func.func @mk_17_19_23(%A: memref<17x23xf32> {llvm.noalias}, %B: memref<23x19xf32> {llvm.noalias}, %C: memref<17x19xf32> {llvm.noalias}) {
  affine.for %ii = 0 to 17 step 8 {
    affine.for %kk = 0 to 23 step 16 {
      affine.for %i = affine_map<(d0) -> (d0)>(%ii) to min affine_map<(d0) -> (d0 + 8, 17)>(%ii) {
        affine.for %j = 0 to 19 {
          affine.for %k = affine_map<(d0) -> (d0)>(%kk) to min affine_map<(d0) -> (d0 + 16, 23)>(%kk) {
            %a = affine.load %A[%i, %k] : memref<17x23xf32>
            %b = affine.load %B[%k, %j] : memref<23x19xf32>
            %c = affine.load %C[%i, %j] : memref<17x19xf32>
            %p = arith.mulf %a, %b fastmath<reassoc> : f32
            %s = arith.addf %c, %p fastmath<reassoc> : f32
            affine.store %s, %C[%i, %j] : memref<17x19xf32>
          }
        }
      }
    }
  }
  return
}

// -----

// nk_17_19_23: 17x19x23, TN = 4, TK = 32, min boundaries
// CHECK-LABEL: candidate count
// CHECK: recognition result: success
// CHECK: loop tiling (as written; recorded, not a decision): 2 tiled coordinate(s)
// CHECK-NEXT: loop order: 0=tile(J) 1=tile(K) 2=untiled(I) 3=point(J) 4=point(K)
// CHECK-NEXT: J: tile loop 0, point loop 3, TN = 4, bounds form, boundary min-clipped (last tile 3), J in [0, 19)
// CHECK-NEXT: K: tile loop 1, point loop 4, TK = 32, bounds form, boundary min-clipped (last tile 23), K in [0, 23)
// CHECK-NEXT: tiled dimensions: N; reduction tiling: K
// CHECK-NEXT: coverage of the iteration space: not proven at this stage
func.func @nk_17_19_23(%A: memref<17x23xf32> {llvm.noalias}, %B: memref<23x19xf32> {llvm.noalias}, %C: memref<17x19xf32> {llvm.noalias}) {
  affine.for %jj = 0 to 19 step 4 {
    affine.for %kk = 0 to 23 step 32 {
      affine.for %i = 0 to 17 {
        affine.for %j = affine_map<(d0) -> (d0)>(%jj) to min affine_map<(d0) -> (d0 + 4, 19)>(%jj) {
          affine.for %k = affine_map<(d0) -> (d0)>(%kk) to min affine_map<(d0) -> (d0 + 32, 23)>(%kk) {
            %a = affine.load %A[%i, %k] : memref<17x23xf32>
            %b = affine.load %B[%k, %j] : memref<23x19xf32>
            %c = affine.load %C[%i, %j] : memref<17x19xf32>
            %p = arith.mulf %a, %b fastmath<reassoc> : f32
            %s = arith.addf %c, %p fastmath<reassoc> : f32
            affine.store %s, %C[%i, %j] : memref<17x19xf32>
          }
        }
      }
    }
  }
  return
}

// -----

// kk_outermost: 17x19x23, TK = 16, TM = 8, TN = 8, min boundaries
// CHECK-LABEL: candidate count
// CHECK: recognition result: success
// CHECK: loop tiling (as written; recorded, not a decision): 3 tiled coordinate(s)
// CHECK-NEXT: loop order: 0=tile(K) 1=tile(I) 2=tile(J) 3=point(I) 4=point(J) 5=point(K)
// CHECK-NEXT: K: tile loop 0, point loop 5, TK = 16, bounds form, boundary min-clipped (last tile 7), K in [0, 23)
// CHECK-NEXT: I: tile loop 1, point loop 3, TM = 8, bounds form, boundary min-clipped (last tile 1), I in [0, 17)
// CHECK-NEXT: J: tile loop 2, point loop 4, TN = 8, bounds form, boundary min-clipped (last tile 3), J in [0, 19)
// CHECK-NEXT: tiled dimensions: M N; reduction tiling: K
// CHECK-NEXT: coverage of the iteration space: not proven at this stage
func.func @kk_outermost(%A: memref<17x23xf32> {llvm.noalias}, %B: memref<23x19xf32> {llvm.noalias}, %C: memref<17x19xf32> {llvm.noalias}) {
  affine.for %kk = 0 to 23 step 16 {
    affine.for %ii = 0 to 17 step 8 {
      affine.for %jj = 0 to 19 step 8 {
        affine.for %i = affine_map<(d0) -> (d0)>(%ii) to min affine_map<(d0) -> (d0 + 8, 17)>(%ii) {
          affine.for %j = affine_map<(d0) -> (d0)>(%jj) to min affine_map<(d0) -> (d0 + 8, 19)>(%jj) {
            affine.for %k = affine_map<(d0) -> (d0)>(%kk) to min affine_map<(d0) -> (d0 + 16, 23)>(%kk) {
              %a = affine.load %A[%i, %k] : memref<17x23xf32>
              %b = affine.load %B[%k, %j] : memref<23x19xf32>
              %c = affine.load %C[%i, %j] : memref<17x19xf32>
              %p = arith.mulf %a, %b fastmath<reassoc> : f32
              %s = arith.addf %c, %p fastmath<reassoc> : f32
              affine.store %s, %C[%i, %j] : memref<17x19xf32>
            }
          }
        }
      }
    }
  }
  return
}

// -----

// ji_order: 31x33x35, TN = 8, TM = 8, min boundaries
// CHECK-LABEL: candidate count
// CHECK: recognition result: success
// CHECK: loop tiling (as written; recorded, not a decision): 2 tiled coordinate(s)
// CHECK-NEXT: loop order: 0=tile(J) 1=tile(I) 2=point(J) 3=point(I) 4=untiled(K)
// CHECK-NEXT: J: tile loop 0, point loop 2, TN = 8, bounds form, boundary min-clipped (last tile 1), J in [0, 33)
// CHECK-NEXT: I: tile loop 1, point loop 3, TM = 8, bounds form, boundary min-clipped (last tile 7), I in [0, 31)
// CHECK-NEXT: tiled dimensions: M N; reduction tiling: none
// CHECK-NEXT: coverage of the iteration space: not proven at this stage
func.func @ji_order(%A: memref<31x35xf32> {llvm.noalias}, %B: memref<35x33xf32> {llvm.noalias}, %C: memref<31x33xf32> {llvm.noalias}) {
  affine.for %jj = 0 to 33 step 8 {
    affine.for %ii = 0 to 31 step 8 {
      affine.for %j = affine_map<(d0) -> (d0)>(%jj) to min affine_map<(d0) -> (d0 + 8, 33)>(%jj) {
        affine.for %i = affine_map<(d0) -> (d0)>(%ii) to min affine_map<(d0) -> (d0 + 8, 31)>(%ii) {
          affine.for %k = 0 to 35 {
            %a = affine.load %A[%i, %k] : memref<31x35xf32>
            %b = affine.load %B[%k, %j] : memref<35x33xf32>
            %c = affine.load %C[%i, %j] : memref<31x33xf32>
            %p = arith.mulf %a, %b fastmath<reassoc> : f32
            %s = arith.addf %c, %p fastmath<reassoc> : f32
            affine.store %s, %C[%i, %j] : memref<31x33xf32>
          }
        }
      }
    }
  }
  return
}

// -----

// counter_form: I = 8 * i0 + i1 (the dense numbering of docs section 3), J = 4 * j0 + j1, K = 8 * k0 + k1.
// CHECK-LABEL: candidate count
// CHECK: recognition result: success
// CHECK: loop tiling (as written; recorded, not a decision): 3 tiled coordinate(s)
// CHECK-NEXT: loop order: 0=tile(I) 1=tile(J) 2=tile(K) 3=point(I) 4=point(J) 5=point(K)
// CHECK-NEXT: I: tile loop 0, point loop 3, TM = 8, counter form (I = 8 * loop 0 + loop 3), boundary divisible (last tile 8), I in [0, 16)
// CHECK-NEXT: J: tile loop 1, point loop 4, TN = 4, counter form (J = 4 * loop 1 + loop 4), boundary divisible (last tile 4), J in [0, 16)
// CHECK-NEXT: K: tile loop 2, point loop 5, TK = 8, counter form (K = 8 * loop 2 + loop 5), boundary divisible (last tile 8), K in [0, 24)
// CHECK-NEXT: tiled dimensions: M N; reduction tiling: K
func.func @counter_form(%A: memref<16x24xf32> {llvm.noalias}, %B: memref<24x16xf32> {llvm.noalias}, %C: memref<16x16xf32> {llvm.noalias}) {
  affine.for %i0 = 0 to 2 { affine.for %j0 = 0 to 4 { affine.for %k0 = 0 to 3 {
  affine.for %i1 = 0 to 8 { affine.for %j1 = 0 to 4 { affine.for %k1 = 0 to 8 {
    %a = affine.load %A[8 * %i0 + %i1, 8 * %k0 + %k1] : memref<16x24xf32>
    %b = affine.load %B[8 * %k0 + %k1, 4 * %j0 + %j1] : memref<24x16xf32>
    %c = affine.load %C[8 * %i0 + %i1, 4 * %j0 + %j1] : memref<16x16xf32>
    %p = arith.mulf %a, %b fastmath<reassoc> : f32
    %s = arith.addf %c, %p fastmath<reassoc> : f32
    affine.store %s, %C[8 * %i0 + %i1, 4 * %j0 + %j1] : memref<16x16xf32>
  } } } } } }
  return
}

// -----

// untiled_17_19_23: an untiled GEMM: no tiling reported
// CHECK-LABEL: candidate count
// CHECK-NOT: loop tiling
func.func @untiled_17_19_23(%A: memref<17x23xf32> {llvm.noalias}, %B: memref<23x19xf32> {llvm.noalias}, %C: memref<17x19xf32> {llvm.noalias}) {
  affine.for %i = 0 to 17 {
    affine.for %j = 0 to 19 {
      affine.for %k = 0 to 23 {
        %a = affine.load %A[%i, %k] : memref<17x23xf32>
        %b = affine.load %B[%k, %j] : memref<23x19xf32>
        %c = affine.load %C[%i, %j] : memref<17x19xf32>
        %p = arith.mulf %a, %b fastmath<reassoc> : f32
        %s = arith.addf %c, %p fastmath<reassoc> : f32
        affine.store %s, %C[%i, %j] : memref<17x19xf32>
      }
    }
  }
  return
}

// -----

// stepped_access: a stepped loop whose counter an access reads (A[i] with i += 2): a step alone is no tiling
// CHECK-LABEL: candidate count
// CHECK-NOT: loop tiling
func.func @stepped_access(%A: memref<32x32xf32> {llvm.noalias}, %B: memref<32x32xf32> {llvm.noalias}, %C: memref<32x32xf32> {llvm.noalias}) {
  affine.for %i = 0 to 32 step 2 {
    affine.for %j = 0 to 32 {
      affine.for %k = 0 to 32 {
        %a = affine.load %A[%i, %k] : memref<32x32xf32>
        %b = affine.load %B[%k, %j] : memref<32x32xf32>
        %c = affine.load %C[%i, %j] : memref<32x32xf32>
        %p = arith.mulf %a, %b fastmath<reassoc> : f32
        %s = arith.addf %c, %p fastmath<reassoc> : f32
        affine.store %s, %C[%i, %j] : memref<32x32xf32>
      }
    }
  }
  return
}

// -----

// triangle: a loop bounded by an outer counter of step 1 (j from i): not shaped like a tiling
// CHECK-LABEL: candidate count
// CHECK-NOT: loop tiling
func.func @triangle(%A: memref<32x32xf32> {llvm.noalias}, %B: memref<32x32xf32> {llvm.noalias}, %C: memref<32x32xf32> {llvm.noalias}) {
  affine.for %i = 0 to 32 {
    affine.for %j = affine_map<(d0) -> (d0)>(%i) to 32 {
      affine.for %k = 0 to 32 {
        %a = affine.load %A[%i, %k] : memref<32x32xf32>
        %b = affine.load %B[%k, %j] : memref<32x32xf32>
        %c = affine.load %C[%i, %j] : memref<32x32xf32>
        %p = arith.mulf %a, %b fastmath<reassoc> : f32
        %s = arith.addf %c, %p fastmath<reassoc> : f32
        affine.store %s, %C[%i, %j] : memref<32x32xf32>
      }
    }
  }
  return
}

// -----

// width_9: the point loop runs 9 values per tile of step 8 (tiles overlap)
// CHECK-LABEL: candidate count
// CHECK: not a tiling: loop 0: its point loop 1 runs 9 values per tile, not the tile step 8
// CHECK-NOT: tile(
func.func @width_9(%A: memref<32x32xf32> {llvm.noalias}, %B: memref<32x32xf32> {llvm.noalias}, %C: memref<32x32xf32> {llvm.noalias}) {
  affine.for %ii = 0 to 32 step 8 {
    affine.for %i = affine_map<(d0) -> (d0)>(%ii) to min affine_map<(d0) -> (d0 + 9, 32)>(%ii) {
      affine.for %j = 0 to 32 {
        affine.for %k = 0 to 32 {
          %a = affine.load %A[%i, %k] : memref<32x32xf32>
          %b = affine.load %B[%k, %j] : memref<32x32xf32>
          %c = affine.load %C[%i, %j] : memref<32x32xf32>
          %p = arith.mulf %a, %b fastmath<reassoc> : f32
          %s = arith.addf %c, %p fastmath<reassoc> : f32
          affine.store %s, %C[%i, %j] : memref<32x32xf32>
        }
      }
    }
  }
  return
}

// -----

// width_7: the point loop runs 7 values per tile of step 8 (iterations missing)
// CHECK-LABEL: candidate count
// CHECK: not a tiling: loop 0: its point loop 1 runs 7 values per tile, not the tile step 8
func.func @width_7(%A: memref<32x32xf32> {llvm.noalias}, %B: memref<32x32xf32> {llvm.noalias}, %C: memref<32x32xf32> {llvm.noalias}) {
  affine.for %ii = 0 to 32 step 8 {
    affine.for %i = affine_map<(d0) -> (d0)>(%ii) to min affine_map<(d0) -> (d0 + 7, 32)>(%ii) {
      affine.for %j = 0 to 32 {
        affine.for %k = 0 to 32 {
          %a = affine.load %A[%i, %k] : memref<32x32xf32>
          %b = affine.load %B[%k, %j] : memref<32x32xf32>
          %c = affine.load %C[%i, %j] : memref<32x32xf32>
          %p = arith.mulf %a, %b fastmath<reassoc> : f32
          %s = arith.addf %c, %p fastmath<reassoc> : f32
          affine.store %s, %C[%i, %j] : memref<32x32xf32>
        }
      }
    }
  }
  return
}

// -----

// clip_outside: the min clips at 40, past the last tile [24, 32)
// CHECK-LABEL: candidate count
// CHECK: not a tiling: loop 0: the min of its point loop 1 clips at 40, outside the last tile [24, 32)
func.func @clip_outside(%A: memref<40x32xf32> {llvm.noalias}, %B: memref<32x32xf32> {llvm.noalias}, %C: memref<40x32xf32> {llvm.noalias}) {
  affine.for %ii = 0 to 32 step 8 {
    affine.for %i = affine_map<(d0) -> (d0)>(%ii) to min affine_map<(d0) -> (d0 + 8, 40)>(%ii) {
      affine.for %j = 0 to 32 {
        affine.for %k = 0 to 32 {
          %a = affine.load %A[%i, %k] : memref<40x32xf32>
          %b = affine.load %B[%k, %j] : memref<32x32xf32>
          %c = affine.load %C[%i, %j] : memref<40x32xf32>
          %p = arith.mulf %a, %b fastmath<reassoc> : f32
          %s = arith.addf %c, %p fastmath<reassoc> : f32
          affine.store %s, %C[%i, %j] : memref<40x32xf32>
        }
      }
    }
  }
  return
}

// -----

// tile_counter_read: the tile counter is also read by an access
// CHECK-LABEL: candidate count
// CHECK: not a tiling: loop 0: its counter is read by an access or another operation as well as by a loop bound
func.func @tile_counter_read(%A: memref<32x32xf32> {llvm.noalias}, %B: memref<32x64xf32> {llvm.noalias}, %C: memref<32x32xf32> {llvm.noalias}) {
  affine.for %ii = 0 to 32 step 8 {
    affine.for %i = affine_map<(d0) -> (d0)>(%ii) to affine_map<(d0) -> (d0 + 8)>(%ii) {
      affine.for %j = 0 to 32 {
        affine.for %k = 0 to 32 {
          %a = affine.load %A[%i, %k] : memref<32x32xf32>
          %b = affine.load %B[%k, %j + %ii] : memref<32x64xf32>
          %c = affine.load %C[%i, %j] : memref<32x32xf32>
          %p = arith.mulf %a, %b fastmath<reassoc> : f32
          %s = arith.addf %c, %p fastmath<reassoc> : f32
          affine.store %s, %C[%i, %j] : memref<32x32xf32>
        }
      }
    }
  }
  return
}

// -----

// two_point_loops: one tile counter bounds two loops
// CHECK-LABEL: candidate count
// CHECK: not a tiling: loop 0: its counter bounds more than one loop, or one end of a loop only
func.func @two_point_loops(%A: memref<32x32xf32> {llvm.noalias}, %B: memref<32x32xf32> {llvm.noalias}, %C: memref<32x32xf32> {llvm.noalias}) {
  affine.for %ii = 0 to 32 step 8 {
    affine.for %i = affine_map<(d0) -> (d0)>(%ii) to affine_map<(d0) -> (d0 + 8)>(%ii) {
      affine.for %j = affine_map<(d0) -> (d0)>(%ii) to affine_map<(d0) -> (d0 + 8)>(%ii) {
        affine.for %k = 0 to 32 {
          %a = affine.load %A[%i, %k] : memref<32x32xf32>
          %b = affine.load %B[%k, %j] : memref<32x32xf32>
          %c = affine.load %C[%i, %j] : memref<32x32xf32>
          %p = arith.mulf %a, %b fastmath<reassoc> : f32
          %s = arith.addf %c, %p fastmath<reassoc> : f32
          affine.store %s, %C[%i, %j] : memref<32x32xf32>
        }
      }
    }
  }
  return
}

// -----

// scaled_lower: the point loop starts at 2 * ii, not ii plus a constant
// CHECK-LABEL: candidate count
// CHECK: not a tiling: loop 0: the lower bound of its point loop 1 is not the tile counter plus a constant
func.func @scaled_lower(%A: memref<32x32xf32> {llvm.noalias}, %B: memref<32x32xf32> {llvm.noalias}, %C: memref<32x32xf32> {llvm.noalias}) {
  affine.for %ii = 0 to 16 step 8 {
    affine.for %i = affine_map<(d0) -> (d0 * 2)>(%ii) to affine_map<(d0) -> (d0 * 2 + 8)>(%ii) {
      affine.for %j = 0 to 32 {
        affine.for %k = 0 to 32 {
          %a = affine.load %A[%i, %k] : memref<32x32xf32>
          %b = affine.load %B[%k, %j] : memref<32x32xf32>
          %c = affine.load %C[%i, %j] : memref<32x32xf32>
          %p = arith.mulf %a, %b fastmath<reassoc> : f32
          %s = arith.addf %c, %p fastmath<reassoc> : f32
          affine.store %s, %C[%i, %j] : memref<32x32xf32>
        }
      }
    }
  }
  return
}

// -----

// tiled_copy: a tiled copy C[i][j] = A[i][j], no GEMM: the candidate is refused (its body is
// no multiply-accumulate); the loops ARE tiled, which is reported under "no recognized GEMM",
// with unknown coordinates -- never as a tiled GEMM.
// CHECK-LABEL: candidate count: 1
// CHECK: reason: the innermost body is not exactly one multiply-accumulate
// CHECK-NOT: recognition result: success
// CHECK: loop tiling of a nest with no recognized GEMM:
// CHECK-NEXT: loop tiling (as written; recorded, not a decision): 1 tiled coordinate(s)
// CHECK-NEXT: loop order: 0=tile(?) 1=point(?) 2=untiled(?)
// CHECK-NEXT: ?: tile loop 0, point loop 1, T? = 8, bounds form, boundary min-clipped (last tile 1), ? in [0, 17)
func.func @tiled_copy(%A: memref<17x19xf32>, %C: memref<17x19xf32>) {
  affine.for %ii = 0 to 17 step 8 {
    affine.for %i = affine_map<(d0) -> (d0)>(%ii) to min affine_map<(d0) -> (d0 + 8, 17)>(%ii) {
      affine.for %j = 0 to 19 {
        %a = affine.load %A[%i, %j] : memref<17x19xf32>
        affine.store %a, %C[%i, %j] : memref<17x19xf32>
      }
    }
  }
  return
}

// -----

// n_only_17_unrecognized: N-only tiling with a min boundary; no GEMM recognized in the nest
// today (later S5 stage), its tiling is reported with unknown coordinates.
// CHECK-LABEL: candidate count
// CHECK-NOT: recognition result: success
// CHECK: loop tiling of a nest with no recognized GEMM:
// CHECK-NEXT: loop tiling (as written; recorded, not a decision): 1 tiled coordinate(s)
// CHECK-NEXT: loop order: 0=untiled(?) 1=tile(?) 2=point(?) 3=untiled(?)
// CHECK-NEXT: ?: tile loop 1, point loop 2, T? = 8, bounds form, boundary min-clipped (last tile 3), ? in [0, 19)
// CHECK-NEXT: tiled dimensions: none; reduction tiling: none
func.func @n_only_17_unrecognized(%A: memref<17x23xf32> {llvm.noalias}, %B: memref<23x19xf32> {llvm.noalias}, %C: memref<17x19xf32> {llvm.noalias}) {
  affine.for %i = 0 to 17 {
    affine.for %jj = 0 to 19 step 8 {
      affine.for %j = affine_map<(d0) -> (d0)>(%jj) to min affine_map<(d0) -> (d0 + 8, 19)>(%jj) {
        affine.for %k = 0 to 23 {
          %a = affine.load %A[%i, %k] : memref<17x23xf32>
          %b = affine.load %B[%k, %j] : memref<23x19xf32>
          %c = affine.load %C[%i, %j] : memref<17x19xf32>
          %p = arith.mulf %a, %b fastmath<reassoc> : f32
          %s = arith.addf %c, %p fastmath<reassoc> : f32
          affine.store %s, %C[%i, %j] : memref<17x19xf32>
        }
      }
    }
  }
  return
}

// -----

// k_only_17_unrecognized: K-only tiling with a min boundary; as above.
// CHECK-LABEL: candidate count
// CHECK-NOT: recognition result: success
// CHECK: loop tiling of a nest with no recognized GEMM:
// CHECK-NEXT: loop tiling (as written; recorded, not a decision): 1 tiled coordinate(s)
// CHECK-NEXT: loop order: 0=untiled(?) 1=untiled(?) 2=tile(?) 3=point(?)
// CHECK-NEXT: ?: tile loop 2, point loop 3, T? = 16, bounds form, boundary min-clipped (last tile 7), ? in [0, 23)
func.func @k_only_17_unrecognized(%A: memref<17x23xf32> {llvm.noalias}, %B: memref<23x19xf32> {llvm.noalias}, %C: memref<17x19xf32> {llvm.noalias}) {
  affine.for %i = 0 to 17 {
    affine.for %j = 0 to 19 {
      affine.for %kk = 0 to 23 step 16 {
        affine.for %k = affine_map<(d0) -> (d0)>(%kk) to min affine_map<(d0) -> (d0 + 16, 23)>(%kk) {
          %a = affine.load %A[%i, %k] : memref<17x23xf32>
          %b = affine.load %B[%k, %j] : memref<23x19xf32>
          %c = affine.load %C[%i, %j] : memref<17x19xf32>
          %p = arith.mulf %a, %b fastmath<reassoc> : f32
          %s = arith.addf %c, %p fastmath<reassoc> : f32
          affine.store %s, %C[%i, %j] : memref<17x19xf32>
        }
      }
    }
  }
  return
}

// -----

// rank1_tiled: C[i + j] += A[i][k] * B[k][j] tiled 8 x 8 with min boundaries; no GEMM
// recognized in it today (rank-1 tiled nests: a later S5 stage); its tiling is reported.
// CHECK-LABEL: candidate count
// CHECK-NOT: recognition result: success
// CHECK: loop tiling of a nest with no recognized GEMM:
// CHECK-NEXT: loop tiling (as written; recorded, not a decision): 2 tiled coordinate(s)
// CHECK-NEXT: loop order: 0=tile(?) 1=tile(?) 2=point(?) 3=point(?) 4=untiled(?)
func.func @rank1_tiled(%A: memref<17x23xf32> {llvm.noalias}, %B: memref<23x19xf32> {llvm.noalias}, %C: memref<35xf32> {llvm.noalias}) {
  affine.for %ii = 0 to 17 step 8 {
    affine.for %jj = 0 to 19 step 8 {
      affine.for %i = affine_map<(d0) -> (d0)>(%ii) to min affine_map<(d0) -> (d0 + 8, 17)>(%ii) {
        affine.for %j = affine_map<(d0) -> (d0)>(%jj) to min affine_map<(d0) -> (d0 + 8, 19)>(%jj) {
          affine.for %k = 0 to 23 {
            %a = affine.load %A[%i, %k] : memref<17x23xf32>
            %b = affine.load %B[%k, %j] : memref<23x19xf32>
            %c = affine.load %C[%i + %j] : memref<35xf32>
            %p = arith.mulf %a, %b fastmath<reassoc> : f32
            %s = arith.addf %c, %p fastmath<reassoc> : f32
            affine.store %s, %C[%i + %j] : memref<35xf32>
          }
        }
      }
    }
  }
  return
}
