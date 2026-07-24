"""
Runs run_conv2d_shape against every row of sweep_results_template.csv,
generates random test data per row, computes a double-precision reference
(with the same zero-padding), calls the hardware, and fills in
measured_tiles / max_error_ulp / result -- the same shape of workflow as
test_three_shapes.py, just driven from the CSV instead of a hardcoded list.

Usage (run from runtime/, after building run_conv2d_shape and after
`sweep_out/` has been copied in from gen_conv2d_sweep.py):
    python3 run_conv2d_sweep.py --sweep-dir sweep_out

Requires reference.py and ulp.py to already be in the same folder (or on
PYTHONPATH), same as test_three_shapes.py already assumes.
"""
import argparse
import csv
import hashlib
import subprocess
import numpy as np

from ulp import max_ulp_error

RUN_CONV2D_BIN = "./run_conv2d_shape"


def deterministic_seed(name):
    """Python's built-in hash() is randomized per-process since 3.3 (security
    feature) -- NOT safe for reproducible seeding across separate script
    invocations. hashlib gives the same digest every time, on every machine,
    so re-running the sweep (or debug_single_row.py on one row from it)
    reproduces the exact same test data."""
    return int(hashlib.md5(name.encode()).hexdigest(), 16) % (2**32)


def reference_conv2d_padded(X, K, strideH, strideW, dilH, dilW,
                             padTop, padBottom, padLeft, padRight):
    """X: (N,H,W,Cin) float64, K: (Kh,Kw,Cin,Cout) float64 -> (N,Hout,Wout,Cout) float64.
    Zero-pads exactly like fpga_conv2d_im2col_padded_auto's runtime logic,
    computed in double precision as the ground truth.

    Rounds X and K to float32 first (matching what actually gets sent
    to hardware -- run_one() below writes X.astype(np.float32) to the
    wire), then upcasts back to float64. Without this, the reference
    and the hardware are comparing against different input values,
    which bakes in a spurious mismatch unrelated to any real hardware
    imprecision."""
    X = X.astype(np.float32).astype(np.float64)
    K = K.astype(np.float32).astype(np.float64)
    N, H, W, Cin = X.shape
    Kh, Kw, _, Cout = K.shape
    Xp = np.pad(X, ((0, 0), (padTop, padBottom), (padLeft, padRight), (0, 0)))
    Hp, Wp = Xp.shape[1], Xp.shape[2]
    effKh = dilH * (Kh - 1) + 1
    effKw = dilW * (Kw - 1) + 1
    Hout = (Hp - effKh) // strideH + 1
    Wout = (Wp - effKw) // strideW + 1
    Y = np.zeros((N, Hout, Wout, Cout), dtype=np.float64)
    for n in range(N):
        for oy in range(Hout):
            for ox in range(Wout):
                for ky in range(Kh):
                    for kx in range(Kw):
                        iy = oy * strideH + ky * dilH
                        ix = ox * strideW + kx * dilW
                        Y[n, oy, ox, :] += Xp[n, iy, ix, :] @ K[ky, kx, :, :]
    return Y


def run_one(row, bin_path):
    N, H, W, Cin = int(row["N"]), int(row["H"]), int(row["W"]), int(row["Cin"])
    Kh, Kw, Cout = int(row["Kh"]), int(row["Kw"]), int(row["Cout"])
    sH, sW = int(row["strideH"]), int(row["strideW"])
    dH, dW = int(row["dilationH"]), int(row["dilationW"])
    pT, pB = int(row["padTop"]), int(row["padBottom"])
    pL, pR = int(row["padLeft"]), int(row["padRight"])

    # Matches reference.py's make_test_case range (uniform -10,10) used for
    # Table 1's matmul results, so this sweep is apples-to-apples
    # comparable rather than testing a different operand magnitude scale.
    rng = np.random.default_rng(deterministic_seed(row["name"]))
    X64 = rng.uniform(-10, 10, size=(N, H, W, Cin))
    K64 = rng.uniform(-10, 10, size=(Kh, Kw, Cin, Cout))

    X64.astype(np.float32).tofile("X.bin")
    K64.astype(np.float32).tofile("K.bin")

    args = [bin_path, str(N), str(H), str(W), str(Cin), str(Kh), str(Kw),
            str(Cout), str(sH), str(sW), str(dH), str(dW),
            str(pT), str(pB), str(pL), str(pR), "X.bin", "K.bin", "Y.bin"]
    result = subprocess.run(args, capture_output=True, text=True)
    if result.returncode != 0:
        return None, result.stderr.strip() or "nonzero exit"

    Y_ref = reference_conv2d_padded(X64, K64, sH, sW, dH, dW, pT, pB, pL, pR)
    Hout, Wout = Y_ref.shape[1], Y_ref.shape[2]
    Y_hw = np.fromfile("Y.bin", dtype=np.float32).reshape(N, Hout, Wout, Cout)
    err = max_ulp_error(Y_hw, Y_ref)
    return err, None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--sweep-dir", default="sweep_out")
    ap.add_argument("--bin", default=RUN_CONV2D_BIN)
    args = ap.parse_args()

    csv_path = f"{args.sweep_dir}/sweep_results_template.csv"
    with open(csv_path) as f:
        rows = list(csv.DictReader(f))

    fieldnames = list(rows[0].keys())

    for row in rows:
        err, error_msg = run_one(row, args.bin)
        if error_msg is not None:
            row["result"] = f"FAIL ({error_msg})"
            print(f"{row['name']:<20} FAIL: {error_msg}")
        else:
            row["measured_tiles"] = ""  # tile count isn't observable from this
                                         # driver directly; leave blank unless
                                         # you instrument fpga_matmul_tiled_auto
                                         # to log a counter, same as noted for
                                         # the matmul sweep.
            row["max_error_ulp"] = f"{err:.1f}"
            row["result"] = "PASS" if err < 100 else "FAIL"  # adjust threshold
                                                              # against Table 1's
                                                              # observed 7-30 ULP
                                                              # range once you
                                                              # have real numbers
            print(f"{row['name']:<20} max_error_ulp={err:.1f}  {row['result']}")

        # Write the CSV after EVERY row, not just at the end -- if this
        # script is interrupted (Ctrl-C, crash, or a hardware hiccup that
        # requires re-flashing mid-run), everything completed so far is
        # already saved to disk rather than lost.
        with open(csv_path, "w", newline="") as f:
            writer = csv.DictWriter(f, fieldnames=fieldnames)
            writer.writeheader()
            writer.writerows(rows)

    print(f"\nUpdated {csv_path} in place.")


if __name__ == "__main__":
    main()