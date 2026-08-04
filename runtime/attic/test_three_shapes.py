"""
把三組新的 matmul shape (5x5x5, 8x4x6, 3x7x5) 跑過真實 FPGA,
跟 double-precision reference 比對,印出 max ULP error 跟 tile 數。

用法(在 ~/systolic-mlir/runtime/ 底下,run_shape 編好之後):
    python3 test_three_shapes.py

需要 reference.py 跟 ulp.py 在同一個資料夾(或 PYTHONPATH 內)。
"""
import subprocess
import numpy as np

from reference import reference_matmul, make_test_case
from ulp import max_ulp_error

RUN_SHAPE_BIN = "./run_shape"   # 跟 run_shape 編譯出來的位置一致就好


def run_on_fpga(A_f32: np.ndarray, B_f32: np.ndarray, M, K, N) -> np.ndarray:
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
        ((5, 5, 5), 101),
        ((8, 4, 6), 102),
        ((3, 7, 5), 103),
    ]

    print(f"{'shape':<12} {'tiles':>6} {'max ULP error':>14}")
    for (M, K, N), seed in shapes:
        A64, B64 = make_test_case(M, K, N, seed)
        C64 = reference_matmul(A64, B64)
        C_hw = run_on_fpga(A64.astype(np.float32), B64.astype(np.float32), M, K, N)
        err = max_ulp_error(C_hw, C64)
        t = tile_count(M, K, N)
        print(f"{M}x{K}x{N:<7} {t:>6} {err:>14}")
