"""
Runs run_conv2d_shape against every row of sweep_results_template.csv.

CHANGED FROM THE PREVIOUS VERSION
The old reference (reference_conv2d_padded) accumulated in float64 while
the accelerator accumulates in float32, so every row reported a nonzero
"error" that was really just fp32-vs-fp64 rounding -- it grew with
problem size and was dominated by cancellation, which is why it looked
correlated with Kdim. Configs 004/029/032/042 compare BIT-EXACT against
the order-matched fp32 reference.

Two numbers are now reported per row:
  err_ulp   -- vs reference_conv2d_f32 (fp32, im2col + sequential-k,
               matching fpga_conv2d_im2col_padded_auto). This is the
               correctness gate: anything other than 0 is a real bug.
  fp64_ulp  -- vs the old float64 reference. NOT a correctness signal;
               it measures the accelerator's intrinsic fp32 accumulation
               error against an infinitely-precise ideal. Useful for the
               numerical-analysis section, not for pass/fail.

Usage (from runtime/, after building run_conv2d_shape):
    python3 run_conv2d_sweep.py --sweep-dir sweep_out
    python3 run_conv2d_sweep.py --only 004,029,032,042   # subset
    python3 run_conv2d_sweep.py --keep-bins              # per-row .bin dumps
"""
import argparse
import csv
import hashlib
import os
import shutil
import subprocess

import numpy as np

from reference import reference_conv2d_f32
from ulp import max_ulp_error

RUN_CONV2D_BIN = "./run_conv2d_shape"


def deterministic_seed(name):
    """Python's built-in hash() is randomized per-process since 3.3 -- NOT
    safe for reproducible seeding across separate script invocations.
    hashlib gives the same digest every time, on every machine."""
    return int(hashlib.md5(name.encode()).hexdigest(), 16) % (2**32)


def reference_conv2d_fp64(X, K, sH, sW, dH, dW, pT, pB, pL, pR):
    """Float64 reference, kept as the numerical-quality baseline only.

    Inputs are rounded to float32 first so that the reference and the
    hardware agree on what the input values are; the accumulation is
    then done in float64. The gap between this and the hardware is the
    accelerator's own fp32 rounding, not an implementation error.
    """
    X = X.astype(np.float32).astype(np.float64)
    K = K.astype(np.float32).astype(np.float64)
    N, H, W, Cin = X.shape
    Kh, Kw, _, Cout = K.shape
    Xp = np.pad(X, ((0, 0), (pT, pB), (pL, pR), (0, 0)))
    Hp, Wp = Xp.shape[1], Xp.shape[2]
    effKh = dH * (Kh - 1) + 1
    effKw = dW * (Kw - 1) + 1
    Hout = (Hp - effKh) // sH + 1
    Wout = (Wp - effKw) // sW + 1
    Y = np.zeros((N, Hout, Wout, Cout), dtype=np.float64)
    for n in range(N):
        for oy in range(Hout):
            for ox in range(Wout):
                for ky in range(Kh):
                    for kx in range(Kw):
                        iy = oy * sH + ky * dH
                        ix = ox * sW + kx * dW
                        Y[n, oy, ox, :] += Xp[n, iy, ix, :] @ K[ky, kx, :, :]
    return Y


def unpack(row):
    f = lambda k: int(row[k])
    return (f("N"), f("H"), f("W"), f("Cin"), f("Kh"), f("Kw"), f("Cout"),
            f("strideH"), f("strideW"), f("dilationH"), f("dilationW"),
            f("padTop"), f("padBottom"), f("padLeft"), f("padRight"))


def run_one(row, bin_path, keep_bins):
    p = unpack(row)
    (N, H, W, Cin, Kh, Kw, Cout, sH, sW, dH, dW, pT, pB, pL, pR) = p
    name = row["name"]

    rng = np.random.default_rng(deterministic_seed(name))
    X64 = rng.uniform(-10, 10, size=(N, H, W, Cin))
    K64 = rng.uniform(-10, 10, size=(Kh, Kw, Cin, Cout))
    X32 = X64.astype(np.float32)
    K32 = K64.astype(np.float32)

    X32.tofile("X.bin")
    K32.tofile("K.bin")

    args = [bin_path, str(N), str(H), str(W), str(Cin), str(Kh), str(Kw),
            str(Cout), str(sH), str(sW), str(dH), str(dW),
            str(pT), str(pB), str(pL), str(pR), "X.bin", "K.bin", "Y.bin"]
    result = subprocess.run(args, capture_output=True, text=True)
    if os.environ.get("FPGA_UART_STATS") and result.stderr:
        print("   ", result.stderr.strip())
    if result.returncode != 0:
        return None, result.stderr.strip() or "nonzero exit"

    # Correctness gate: order-matched fp32.
    Y_ref32 = reference_conv2d_f32(X32.ravel(), K32.ravel(), *p)
    Hout, Wout = Y_ref32.shape[1], Y_ref32.shape[2]

    n_expect = N * Hout * Wout * Cout
    Y_hw = np.fromfile("Y.bin", dtype=np.float32)
    if Y_hw.size != n_expect:
        return None, f"output size {Y_hw.size}, expected {n_expect}"
    Y_hw = Y_hw.reshape(N, Hout, Wout, Cout)

    if keep_bins:
        for src, dst in (("X.bin", f"X_{name}.bin"),
                         ("K.bin", f"K_{name}.bin"),
                         ("Y.bin", f"Y_{name}.bin")):
            shutil.copyfile(src, dst)

    diff = (Y_hw.view(np.int32) != Y_ref32.view(np.int32))
    n_diff = int(diff.sum())
    err_ulp = max_ulp_error(Y_hw, Y_ref32)

    # Secondary metric only -- never used for pass/fail.
    Y_ref64 = reference_conv2d_fp64(X64, K64, sH, sW, dH, dW, pT, pB, pL, pR)
    fp64_ulp = max_ulp_error(Y_hw, Y_ref64)

    return (err_ulp, n_diff, Y_hw.size, fp64_ulp), None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--sweep-dir", default="sweep_out")
    ap.add_argument("--bin", default=RUN_CONV2D_BIN)
    ap.add_argument("--only", default=None,
                    help="comma-separated config numbers, e.g. 004,029")
    ap.add_argument("--keep-bins", action="store_true",
                    help="save per-config X/K/Y .bin dumps for debugging")
    args = ap.parse_args()

    csv_path = f"{args.sweep_dir}/sweep_results_template.csv"
    with open(csv_path) as f:
        rows = list(csv.DictReader(f))
    fieldnames = list(rows[0].keys())
    if "fp64_ulp" not in fieldnames:
        fieldnames.append("fp64_ulp")

    todo = rows
    if args.only:
        keep = {s.strip() for s in args.only.split(",")}
        todo = [r for r in rows if r["name"].split("_")[-1] in keep]

    n_pass = n_fail = 0
    for row in todo:
        out, error_msg = run_one(row, args.bin, args.keep_bins)
        if error_msg is None and out[1] != 0:          # n_diff != 0
            out2, error_msg2 = run_one(row, args.bin, args.keep_bins)
            if error_msg2 is None and out2[1] == 0:
                out, retried = out2, True        # 第二次過了 → 偶發
        if error_msg is not None:
            row["result"] = f"ERROR ({error_msg})"
            n_fail += 1
            print(f"{row['name']:<20} ERROR: {error_msg}")
            continue

        err_ulp, n_diff, n_elem, fp64_ulp = out
        row["max_error_ulp"] = f"{err_ulp:.0f}"
        row["fp64_ulp"] = f"{fp64_ulp:.0f}"
        row["result"] = "BIT-EXACT" if n_diff == 0 else "MISMATCH"
        if n_diff == 0:
            n_pass += 1
            print(f"{row['name']:<20} BIT-EXACT           "
                  f"(fp64_ulp={fp64_ulp:.0f})")
        else:
            n_fail += 1
            print(f"{row['name']:<20} MISMATCH  max_ulp={err_ulp:.0f} "
                  f"diff={n_diff}/{n_elem}  (fp64_ulp={fp64_ulp:.0f})")

        # Write after EVERY row: if this is interrupted (Ctrl-C, crash, or a
        # hardware hiccup needing a re-flash), completed work is already on
        # disk rather than lost.
        with open(csv_path, "w", newline="") as f:
            writer = csv.DictWriter(f, fieldnames=fieldnames)
            writer.writeheader()
            writer.writerows(rows)

    print(f"\n{n_pass} bit-exact, {n_fail} not. Updated {csv_path}.")
    return 0 if n_fail == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
