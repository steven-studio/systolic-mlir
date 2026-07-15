"""
診斷用:把 4x4x4、6x6x6(論文已宣稱 <=1 ULP 的兩組)跟三組新 shape
全部用同一套方法重跑,並印出誤差最大那個位置的實際數值(不只是 ULP 數字),
方便判斷是「正常的浮點運算順序差異」還是「真的有 bug」。

用法:python3 diagnose_ulp.py
"""
import subprocess
import numpy as np

from reference import reference_matmul, make_test_case
from ulp import max_ulp_error, ulp_distance

RUN_SHAPE_BIN = "./run_shape"


def run_on_fpga(A_f32, B_f32, M, K, N):
    A_f32.tofile("A.bin")
    B_f32.tofile("B.bin")
    subprocess.run(
        [RUN_SHAPE_BIN, str(M), str(K), str(N), "A.bin", "B.bin", "C.bin"],
        check=True,
    )
    return np.fromfile("C.bin", dtype=np.float32).reshape(M, N)


def tile_count(M, K, N):
    def ceil_div(a, b):
        return -(-a // b)
    return ceil_div(M, 4) * ceil_div(K, 4) * ceil_div(N, 4)


if __name__ == "__main__":
    shapes = [
        ((4, 4, 4), 1),
        ((6, 6, 6), 7),
        ((5, 5, 5), 101),
        ((8, 4, 6), 102),
        ((3, 7, 5), 103),
    ]

    for (M, K, N), seed in shapes:
        A64, B64 = make_test_case(M, K, N, seed)
        C64 = reference_matmul(A64, B64)
        C_hw = run_on_fpga(A64.astype(np.float32), B64.astype(np.float32), M, K, N)

        ref_f32 = C64.astype(np.float32)
        diffs = np.array([[ulp_distance(C_hw[i, j], ref_f32[i, j]) for j in range(N)]
                           for i in range(M)])
        worst_idx = np.unravel_index(np.argmax(diffs), diffs.shape)
        i, j = worst_idx

        print(f"=== {M}x{K}x{N} (tiles={tile_count(M,K,N)}) ===")
        print(f"  max ULP error = {diffs.max()}  at C[{i}][{j}]")
        print(f"  hardware value   = {C_hw[i,j]!r}")
        print(f"  reference (f32)  = {ref_f32[i,j]!r}")
        print(f"  reference (f64)  = {C64[i,j]!r}")
        print(f"  abs diff         = {abs(float(C_hw[i,j]) - float(ref_f32[i,j])):.3e}")
        print()
