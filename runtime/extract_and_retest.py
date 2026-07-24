"""
Builds the EXACT Xcol/Kmat that fpga_conv2d_im2col_padded_auto would
compute for a given sweep row (same im2col unfold logic, reimplemented
here in Python), dumps them to X_extracted.bin / K_extracted.bin, then
calls run_shape directly (the SAME pure-matmul path already proven
stable at 599 ULP for a random-data test) on that EXACT data, repeated
N times.

If THIS shows instability too: the trigger is the DATA PATTERN itself
(im2col-shaped, likely containing repeated/overlapping values from
receptive-field overlap), not anything specific to
fpga_conv2d_im2col_padded_auto's code.

If THIS stays stable: the instability really is specific to something
in fpga_conv2d_im2col_padded_auto's own code (not just the data it
happens to produce), narrowing the search back to that function.

Usage:
    python3 extract_and_retest.py --sweep-dir sweep_out --row conv_sweep_001 --repeats 10
"""
import argparse
import csv
import hashlib
import subprocess
import numpy as np

RUN_SHAPE_BIN = "./run_shape"


def deterministic_seed(name):
    return int(hashlib.md5(name.encode()).hexdigest(), 16) % (2**32)


def build_xcol_kmat(X, K, strideH, strideW, dilH, dilW, padTop, padLeft, Hout, Wout):
    """Mirrors fpga_conv2d_im2col_padded_auto's im2col unfold exactly,
    for image index 0 only (matches an N=1 row)."""
    H, W, Cin = X.shape[1], X.shape[2], X.shape[3]
    Kh, Kw, _, Cout = K.shape
    Kdim = Kh * Kw * Cin
    M = Hout * Wout
    Xcol = np.zeros((M, Kdim), dtype=np.float32)
    for oy in range(Hout):
        for ox in range(Wout):
            row = oy * Wout + ox
            for ky in range(Kh):
                for kx in range(Kw):
                    iy = oy * strideH + ky * dilH - padTop
                    ix = ox * strideW + kx * dilW - padLeft
                    for c in range(Cin):
                        col = (ky * Kw + kx) * Cin + c
                        v = 0.0
                        if 0 <= iy < H and 0 <= ix < W:
                            v = X[0, iy, ix, c]
                        Xcol[row, col] = v
    Kmat = K.reshape(Kdim, Cout).astype(np.float32)
    return Xcol, Kmat, M, Kdim, Cout


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--sweep-dir", default="sweep_out")
    ap.add_argument("--row", required=True)
    ap.add_argument("--repeats", type=int, default=10)
    args = ap.parse_args()

    with open(f"{args.sweep_dir}/sweep_results_template.csv") as f:
        rows = {r["name"]: r for r in csv.DictReader(f)}
    row = rows[args.row]

    N, H, W, Cin = int(row["N"]), int(row["H"]), int(row["W"]), int(row["Cin"])
    Kh, Kw, Cout = int(row["Kh"]), int(row["Kw"]), int(row["Cout"])
    sH, sW = int(row["strideH"]), int(row["strideW"])
    dH, dW = int(row["dilationH"]), int(row["dilationW"])
    pT, pL = int(row["padTop"]), int(row["padLeft"])
    pB, pR = int(row["padBottom"]), int(row["padRight"])

    effKh = dH * (Kh - 1) + 1
    effKw = dW * (Kw - 1) + 1
    Hout = (H + pT + pB - effKh) // sH + 1
    Wout = (W + pL + pR - effKw) // sW + 1

    rng = np.random.default_rng(deterministic_seed(args.row))
    X64 = rng.uniform(-10, 10, size=(N, H, W, Cin))
    K64 = rng.uniform(-10, 10, size=(Kh, Kw, Cin, Cout))

    Xcol, Kmat, M, Kdim, Ncols = build_xcol_kmat(
        X64, K64, sH, sW, dH, dW, pT, pL, Hout, Wout)

    print(f"{args.row}: extracted Xcol shape ({M},{Kdim}), Kmat shape ({Kdim},{Ncols})")
    print(f"Running this EXACT im2col-shaped data through run_shape "
          f"{args.repeats} times...")

    Xcol.tofile("X_extracted.bin")
    Kmat.tofile("K_extracted.bin")

    from reference import reference_matmul
    from ulp import max_ulp_error
    C_ref = reference_matmul(Xcol.astype(np.float64), Kmat.astype(np.float64))

    results = []
    for i in range(args.repeats):
        subprocess.run([RUN_SHAPE_BIN, str(M), str(Kdim), str(Ncols),
                         "X_extracted.bin", "K_extracted.bin", "C_extracted.bin"],
                        check=True, capture_output=True)
        C_hw = np.fromfile("C_extracted.bin", dtype=np.float32).reshape(M, Ncols)
        err = max_ulp_error(C_hw, C_ref)
        results.append(err)
        print(f"  run {i+1}: max_ulp_error={err:.1f}")

    print(f"\nAll {args.repeats} results: {results}")
    if len(set(results)) == 1:
        print("-> IDENTICAL every time. The im2col-shaped DATA is not the "
              "trigger -- points back to something in "
              "fpga_conv2d_im2col_padded_auto's own code, not the data.")
    else:
        print("-> VARIES between runs, even via the pure matmul path. "
              "The instability tracks the DATA PATTERN itself (im2col-shaped, "
              "likely with repeated/overlapping values), not conv2d's code.")


if __name__ == "__main__":
    main()
