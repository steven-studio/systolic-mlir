"""
Reruns ONE named row from sweep_out (e.g. conv_sweep_015) and prints a
full element-by-element comparison against the double-precision
reference, instead of just the aggregate max-ULP number -- to tell
apart two very different failure shapes:

  - One or a few wildly-wrong values, everything else close: points to
    an indexing / out-of-bounds bug (e.g. a boundary tile reading or
    writing past where it should for a Cin/Cout that isn't a multiple
    of 4), not a precision issue.
  - Every value elevated by a similar, moderate amount: consistent
    with genuine accumulated floating-point imprecision compounding
    across many tile invocations (the phenomenon already documented
    for the matmul baseline).

Usage:
    python3 debug_single_row.py --sweep-dir sweep_out --row conv_sweep_015
"""
import argparse
import csv
import hashlib
import subprocess
import numpy as np

RUN_CONV2D_BIN = "./run_conv2d_shape"


def deterministic_seed(name):
    """Python's built-in hash() is randomized per-process since 3.3 (security
    feature) -- NOT safe for reproducible seeding across separate script
    invocations. hashlib gives the same digest every time, on every machine."""
    return int(hashlib.md5(name.encode()).hexdigest(), 16) % (2**32)


def reference_conv2d_padded(X, K, strideH, strideW, dilH, dilW,
                             padTop, padBottom, padLeft, padRight):
    """Rounds X and K to float32 first (matching what actually gets sent
    to hardware via X_debug.bin/K_debug.bin below), then upcasts back to
    float64, before computing the double-precision reference. Without
    this, the reference and the hardware are comparing against
    different input values, which bakes in a spurious mismatch
    unrelated to any real hardware imprecision."""
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
    return Y, Hout, Wout


def ulp_diff(a, b):
    """Per-element ULP distance between two float32 arrays."""
    ai = a.astype(np.float32).view(np.int32).astype(np.int64)
    bi = b.astype(np.float32).view(np.int32).astype(np.int64)
    ai = np.where(ai < 0, np.int64(0x80000000) - ai, ai)
    bi = np.where(bi < 0, np.int64(0x80000000) - bi, bi)
    return np.abs(ai - bi)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--sweep-dir", default="sweep_out")
    ap.add_argument("--row", required=True)
    ap.add_argument("--bin", default=RUN_CONV2D_BIN)
    args = ap.parse_args()

    with open(f"{args.sweep_dir}/sweep_results_template.csv") as f:
        rows = {r["name"]: r for r in csv.DictReader(f)}
    row = rows[args.row]

    N, H, W, Cin = int(row["N"]), int(row["H"]), int(row["W"]), int(row["Cin"])
    Kh, Kw, Cout = int(row["Kh"]), int(row["Kw"]), int(row["Cout"])
    sH, sW = int(row["strideH"]), int(row["strideW"])
    dH, dW = int(row["dilationH"]), int(row["dilationW"])
    pT, pB = int(row["padTop"]), int(row["padBottom"])
    pL, pR = int(row["padLeft"]), int(row["padRight"])

    print(f"{args.row}: N={N} H={H} W={W} Cin={Cin} Kh={Kh} Kw={Kw} Cout={Cout} "
          f"stride=({sH},{sW}) dil=({dH},{dW}) pad=({pT},{pB},{pL},{pR})")

    rng = np.random.default_rng(deterministic_seed(args.row))
    X64 = rng.uniform(-10, 10, size=(N, H, W, Cin))
    K64 = rng.uniform(-10, 10, size=(Kh, Kw, Cin, Cout))
    X64.astype(np.float32).tofile("X_debug.bin")
    K64.astype(np.float32).tofile("K_debug.bin")

    args_list = [args.bin, str(N), str(H), str(W), str(Cin), str(Kh), str(Kw),
                 str(Cout), str(sH), str(sW), str(dH), str(dW),
                 str(pT), str(pB), str(pL), str(pR),
                 "X_debug.bin", "K_debug.bin", "Y_debug.bin"]
    result = subprocess.run(args_list, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"run_conv2d_shape failed: {result.stderr}")
        return

    Y_ref, Hout, Wout = reference_conv2d_padded(X64, K64, sH, sW, dH, dW, pT, pB, pL, pR)
    Y_hw = np.fromfile("Y_debug.bin", dtype=np.float32).reshape(N, Hout, Wout, Cout)

    diffs = ulp_diff(Y_hw, Y_ref)
    print(f"\nShape: N={N} Hout={Hout} Wout={Wout} Cout={Cout} "
          f"({Y_hw.size} total output elements)")
    print(f"Max ULP error: {diffs.max()}  |  Median: {int(np.median(diffs))}  |  "
          f"Mean: {diffs.mean():.1f}")

    flat_hw = Y_hw.flatten()
    flat_ref = Y_ref.flatten()
    flat_diff = diffs.flatten()
    order = np.argsort(-flat_diff)

    print("\nTop 10 worst elements (index -> hw vs ref, ULP diff):")
    for idx in order[:10]:
        n_i = idx // (Hout * Wout * Cout)
        rem = idx % (Hout * Wout * Cout)
        oy_i = rem // (Wout * Cout)
        rem2 = rem % (Wout * Cout)
        ox_i = rem2 // Cout
        co_i = rem2 % Cout
        print(f"  [n={n_i},oy={oy_i},ox={ox_i},cout={co_i}]  "
              f"hw={flat_hw[idx]:.6g}  ref={flat_ref[idx]:.6g}  "
              f"ulp={flat_diff[idx]}")

    n_bad = int(np.sum(flat_diff > 100))
    print(f"\n{n_bad}/{flat_hw.size} elements have >100 ULP error.")
    if n_bad < flat_hw.size * 0.1:
        print("-> A SMALL MINORITY of elements are wildly wrong. This looks like a "
              "LOCALIZED bug (specific output positions / channels), not broad "
              "accumulated imprecision. Check tile-boundary or Cout%4 handling.")
    else:
        print("-> Errors are widespread across most elements. More consistent with "
              "broad accumulated precision loss, though 20000+ ULP is still far "
              "beyond what pure precision loss alone would explain.")


if __name__ == "__main__":
    main()