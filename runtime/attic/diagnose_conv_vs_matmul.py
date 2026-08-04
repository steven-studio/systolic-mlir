"""
Isolates whether conv2d's suspiciously low (1-2 ULP) error is a property
of the (M, K, N) shape itself, or specific to the conv2d/im2col code path.

For a chosen row of sweep_results_template.csv, this derives the
equivalent flattened matmul shape:
    M = Hout * Wout * N   (im2col's output-row count)
    K = Kh * Kw * Cin     (im2col's reduction/column count)
    N_ = Cout
and runs THAT shape directly through the existing, already-trusted
run_shape binary (the same one behind test_three_shapes.py / Table 1),
using the same magnitude range as reference.py (-10, 10).

If this raw-matmul test also comes back ~1-2 ULP: the low error is a
property of this (M,K,N) shape/magnitude combination on the real
hardware, not a conv2d-specific bug -- worth understanding why, but not
a correctness problem in the conv2d path.

If this raw-matmul test comes back ~7-30 ULP (like Table 1): the conv2d
path itself (im2col assembly, or the reference_conv2d_padded computation
in run_conv2d_sweep.py) is doing something wrong, and the low conv2d
numbers should NOT be trusted or written into the paper as-is.

Usage:
    python3 diagnose_conv_vs_matmul.py --sweep-dir sweep_out --row conv_sweep_020
"""
import argparse
import csv
import subprocess
import numpy as np

from reference import reference_matmul
from ulp import max_ulp_error

RUN_SHAPE_BIN = "./run_shape"


def conv_out_dim(dim, k, stride, dilation):
    eff_k = dilation * (k - 1) + 1
    return (dim - eff_k) // stride + 1


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--sweep-dir", default="sweep_out")
    ap.add_argument("--row", required=True,
                     help="name of the sweep row to derive the shape from, "
                          "e.g. conv_sweep_020")
    ap.add_argument("--seed", type=int, default=999)
    args = ap.parse_args()

    csv_path = f"{args.sweep_dir}/sweep_results_template.csv"
    with open(csv_path) as f:
        rows = {r["name"]: r for r in csv.DictReader(f)}

    if args.row not in rows:
        print(f"row {args.row} not found in {csv_path}")
        return
    row = rows[args.row]

    N, H, W, Cin = int(row["N"]), int(row["H"]), int(row["W"]), int(row["Cin"])
    Kh, Kw, Cout = int(row["Kh"]), int(row["Kw"]), int(row["Cout"])
    sH, sW = int(row["strideH"]), int(row["strideW"])
    dH, dW = int(row["dilationH"]), int(row["dilationW"])
    pT, pB = int(row["padTop"]), int(row["padBottom"])
    pL, pR = int(row["padLeft"]), int(row["padRight"])

    Hpadded, Wpadded = H + pT + pB, W + pL + pR
    Hout = conv_out_dim(Hpadded, Kh, sH, dH)
    Wout = conv_out_dim(Wpadded, Kw, sW, dW)

    M = Hout * Wout * N
    K = Kh * Kw * Cin
    Ncols = Cout

    print(f"Row {args.row}: conv shape N={N} H={H} W={W} Cin={Cin} "
          f"Kh={Kh} Kw={Kw} Cout={Cout} -> equivalent matmul M={M} K={K} N={Ncols}")
    print(f"conv2d measured (from earlier run): "
          f"max_error_ulp={row.get('max_error_ulp', '?')} tiles={row.get('predicted_tiles', '?')}")

    # Same magnitude range as reference.py's make_test_case, used for
    # Table 1's results -- deliberately NOT re-deriving via conv2d/im2col
    # at all, to isolate the (M,K,N) shape itself.
    rng = np.random.default_rng(args.seed)
    A64 = rng.uniform(-10, 10, size=(M, K)).astype(np.float64)
    B64 = rng.uniform(-10, 10, size=(K, Ncols)).astype(np.float64)
    C64 = reference_matmul(A64, B64)

    A64.astype(np.float32).tofile("A_diag.bin")
    B64.astype(np.float32).tofile("B_diag.bin")

    subprocess.run([RUN_SHAPE_BIN, str(M), str(K), str(Ncols),
                     "A_diag.bin", "B_diag.bin", "C_diag.bin"], check=True)
    C_hw = np.fromfile("C_diag.bin", dtype=np.float32).reshape(M, Ncols)

    err = max_ulp_error(C_hw, C64)

    def ceil_div(a, b):
        return -(-a // b)
    tiles = ceil_div(M, 4) * ceil_div(K, 4) * ceil_div(Ncols, 4)

    print(f"\nDirect matmul test at the SAME (M,K,N)=({M},{K},{Ncols}), "
          f"same magnitude range, bypassing conv2d/im2col entirely:")
    print(f"  tiles={tiles}  max_error_ulp={err:.1f}")
    print()
    if err > 5:
        print("-> Comparable to Table 1's 7-30 ULP range. The shape itself "
              "explains the magnitude of error; conv2d's low numbers need "
              "re-checking (likely a bug in the im2col path or in "
              "reference_conv2d_padded).")
    else:
        print("-> Also low (~1-2 ULP), same as conv2d's result. This "
              "(M,K,N) shape/magnitude combination is genuinely more "
              "accurate on this hardware than Table 1's shapes -- worth "
              "investigating why, but NOT a conv2d-specific bug.")


if __name__ == "__main__":
    main()
