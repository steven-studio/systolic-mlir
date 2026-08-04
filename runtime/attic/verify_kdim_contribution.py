"""
Tests whether reduction depth (Kdim = Kh*Kw*Cin, or the more
hardware-relevant K_tiles = ceil(Kdim/4), the number of chained
4x4x4 tile dispatches accumulated in-place) adds explanatory power
for relative error BEYOND what cancellation_ratio alone explains.

Kdim/K_tiles is constant within a single conv2d configuration (every
output element of one row shares the same Kdim), so this can only be
tested ACROSS rows, comparing rows with different Kdim/K_tiles values.
With 4 rows there are at most 3-4 distinct Kdim/K_tiles values, so
resolution is coarse -- treat this as a first, cheap look, not a
definitive test.

Method: pool all elements from all analyzed rows (tagging each element
with its own row's Kdim/K_tiles), then fit two log-log linear models:

    log(rel_err) ~ log(cancellation_ratio)                    [model 1]
    log(rel_err) ~ log(cancellation_ratio) + log(K_tiles)      [model 2]

and report R^2 for both, plus the R^2 improvement from adding K_tiles.
A meaningful improvement (self-test in this project's chat history
showed ~0.07 R^2 improvement for a deliberately-constructed case where
Kdim genuinely mattered, vs 0.0000 for a case where it did not) is
evidence K_tiles carries independent information; near-zero improvement
is evidence it does not, given this data.

Also reports the partial correlation of model-1's residuals against
log(K_tiles) directly, as a second, more direct way to see the same
thing (residual correlation near 0 <=> no independent contribution).

This script draws no conclusion on its own -- report the R^2 values,
the residual correlation, and the plot, and let the reader (and the
paper) judge.

Usage:
    python3 verify_kdim_contribution.py --sweep-dir sweep_out \
        --rows conv_sweep_004 conv_sweep_029 conv_sweep_042 conv_sweep_032 \
        --bin ./run_conv2d_shape
"""
import argparse
import csv
import hashlib
import subprocess
import sys

import numpy as np

try:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    HAVE_MPL = True
except ImportError:
    HAVE_MPL = False


def deterministic_seed(name):
    return int(hashlib.md5(name.encode()).hexdigest(), 16) % (2**32)


def reference_and_cancellation(X, K, strideH, strideW, dilH, dilW,
                                padTop, padBottom, padLeft, padRight):
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
    sum_abs = np.zeros((N, Hout, Wout, Cout), dtype=np.float64)

    for n in range(N):
        for oy in range(Hout):
            for ox in range(Wout):
                for ky in range(Kh):
                    for kx in range(Kw):
                        iy = oy * strideH + ky * dilH
                        ix = ox * strideW + kx * dilW
                        x_vec = Xp[n, iy, ix, :]
                        k_mat = K[ky, kx, :, :]
                        terms = x_vec[:, None] * k_mat
                        Y[n, oy, ox, :] += terms.sum(axis=0)
                        sum_abs[n, oy, ox, :] += np.abs(terms).sum(axis=0)
    return Y, sum_abs, Hout, Wout


def run_row(name, row, bin_path):
    N, H, W, Cin = int(row["N"]), int(row["H"]), int(row["W"]), int(row["Cin"])
    Kh, Kw, Cout = int(row["Kh"]), int(row["Kw"]), int(row["Cout"])
    sH, sW = int(row["strideH"]), int(row["strideW"])
    dH, dW = int(row["dilationH"]), int(row["dilationW"])
    pT, pB = int(row["padTop"]), int(row["padBottom"])
    pL, pR = int(row["padLeft"]), int(row["padRight"])

    Kdim = Kh * Kw * Cin
    Ktiles = -(-Kdim // 4)  # ceil div 4

    rng = np.random.default_rng(deterministic_seed(name))
    X64 = rng.uniform(-10, 10, size=(N, H, W, Cin))
    K64 = rng.uniform(-10, 10, size=(Kh, Kw, Cin, Cout))

    Y_ref, sum_abs, Hout, Wout = reference_and_cancellation(
        X64, K64, sH, sW, dH, dW, pT, pB, pL, pR)

    X64.astype(np.float32).tofile(f"X_{name}.bin")
    K64.astype(np.float32).tofile(f"K_{name}.bin")

    args = [bin_path, str(N), str(H), str(W), str(Cin), str(Kh), str(Kw),
            str(Cout), str(sH), str(sW), str(dH), str(dW),
            str(pT), str(pB), str(pL), str(pR),
            f"X_{name}.bin", f"K_{name}.bin", f"Y_{name}.bin"]
    result = subprocess.run(args, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"[{name}] run_conv2d_shape FAILED: {result.stderr}", file=sys.stderr)
        return None

    Y_hw = np.fromfile(f"Y_{name}.bin", dtype=np.float32).reshape(N, Hout, Wout, Cout)
    return Y_hw.astype(np.float64), Y_ref, sum_abs, Kdim, Ktiles


def fit_and_r2(X, y):
    coef, *_ = np.linalg.lstsq(X, y, rcond=None)
    pred = X @ coef
    ss_res = np.sum((y - pred) ** 2)
    ss_tot = np.sum((y - y.mean()) ** 2)
    r2 = 1 - ss_res / ss_tot
    residuals = y - pred
    return r2, coef, residuals


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--sweep-dir", default="sweep_out")
    ap.add_argument("--rows", nargs="+", required=True)
    ap.add_argument("--bin", default="./run_conv2d_shape")
    args = ap.parse_args()

    with open(f"{args.sweep_dir}/sweep_results_template.csv") as f:
        rows = {r["name"]: r for r in csv.DictReader(f)}

    pooled_cancel, pooled_relerr, pooled_kdim, pooled_ktiles, pooled_names = [], [], [], [], []
    per_row_info = []

    for name in args.rows:
        if name not in rows:
            print(f"WARNING: {name} not found, skipping", file=sys.stderr)
            continue
        out = run_row(name, rows[name], args.bin)
        if out is None:
            continue
        Y_hw, Y_ref, sum_abs, Kdim, Ktiles = out

        abs_ref = np.abs(Y_ref)
        cancellation_ratio = sum_abs / np.maximum(abs_ref, 1e-300)
        abs_err = np.abs(Y_hw - Y_ref)
        with np.errstate(divide="ignore", invalid="ignore"):
            rel_err = np.where(abs_ref > 0, abs_err / abs_ref, np.nan)

        flat_cancel = cancellation_ratio.flatten()
        flat_rel = rel_err.flatten()

        mask = np.isfinite(flat_rel) & (flat_rel > 0) & (flat_cancel > 0)
        pooled_cancel.append(flat_cancel[mask])
        pooled_relerr.append(flat_rel[mask])
        pooled_kdim.append(np.full(mask.sum(), Kdim))
        pooled_ktiles.append(np.full(mask.sum(), Ktiles))
        pooled_names.extend([name] * mask.sum())
        per_row_info.append((name, Kdim, Ktiles, np.median(flat_rel[mask])))

    if not pooled_cancel:
        print("No rows successfully analyzed.", file=sys.stderr)
        return

    cancel = np.concatenate(pooled_cancel)
    relerr = np.concatenate(pooled_relerr)
    kdim = np.concatenate(pooled_kdim)
    ktiles = np.concatenate(pooled_ktiles)

    log_cancel = np.log(cancel)
    log_relerr = np.log(relerr)
    log_kdim = np.log(kdim)
    log_ktiles = np.log(ktiles)
    ones = np.ones_like(log_cancel)

    print("Per-row Kdim / K_tiles / median relative error:")
    for name, Kd, Kt, med in per_row_info:
        print(f"  {name:<18} Kdim={Kd:<5} K_tiles={Kt:<4} median_rel_err={med:.4e}")

    # Model 1: cancellation_ratio only
    X1 = np.column_stack([log_cancel, ones])
    r2_1, coef1, resid1 = fit_and_r2(X1, log_relerr)

    # Model 2a: cancellation_ratio + Kdim
    X2a = np.column_stack([log_cancel, log_kdim, ones])
    r2_2a, coef2a, _ = fit_and_r2(X2a, log_relerr)

    # Model 2b: cancellation_ratio + K_tiles (more hardware-relevant)
    X2b = np.column_stack([log_cancel, log_ktiles, ones])
    r2_2b, coef2b, _ = fit_and_r2(X2b, log_relerr)

    print(f"\n{'='*70}")
    print(f"POOLED regression results (n={len(relerr)} elements across {len(per_row_info)} rows)")
    print(f"{'='*70}")
    print(f"Model 1  [log(rel_err) ~ log(cancellation_ratio)]:")
    print(f"    R^2 = {r2_1:.4f}   coef(log cancellation_ratio) = {coef1[0]:.4f}")

    print(f"\nModel 2a [log(rel_err) ~ log(cancellation_ratio) + log(Kdim)]:")
    print(f"    R^2 = {r2_2a:.4f}   (improvement over Model 1: {r2_2a - r2_1:+.4f})")
    print(f"    coef(log cancellation_ratio) = {coef2a[0]:.4f}   coef(log Kdim) = {coef2a[1]:.4f}")

    print(f"\nModel 2b [log(rel_err) ~ log(cancellation_ratio) + log(K_tiles)]:")
    print(f"    R^2 = {r2_2b:.4f}   (improvement over Model 1: {r2_2b - r2_1:+.4f})")
    print(f"    coef(log cancellation_ratio) = {coef2b[0]:.4f}   coef(log K_tiles) = {coef2b[1]:.4f}")

    # Direct partial-residual check: correlate Model 1's residuals
    # against log(K_tiles) directly (a second, more transparent view of
    # the same question as the R^2 improvement above).
    r_resid_kdim = np.corrcoef(resid1, log_kdim)[0, 1]
    r_resid_ktiles = np.corrcoef(resid1, log_ktiles)[0, 1]
    print(f"\nPartial check: correlation of Model 1's residuals with log(Kdim)    = {r_resid_kdim:.4f}")
    print(f"Partial check: correlation of Model 1's residuals with log(K_tiles) = {r_resid_ktiles:.4f}")
    print("(near 0 => no independent contribution from reduction depth beyond")
    print(" cancellation_ratio, given this data; a value comparable in magnitude")
    print(" to the earlier cancellation-vs-rel_err correlation (~0.59) would")
    print(" indicate a real additional contribution)")

    if HAVE_MPL:
        fig, ax = plt.subplots(figsize=(6, 5))
        for name, Kd, Kt, _ in per_row_info:
            row_mask = np.array([n == name for n in pooled_names])
            ax.scatter(log_ktiles[row_mask], resid1[row_mask], s=8, alpha=0.5,
                       label=f"{name} (K_tiles={Kt})")
        ax.set_xlabel("log(K_tiles)")
        ax.set_ylabel("Model 1 residual (log rel_err unexplained by cancellation_ratio)")
        ax.set_title(f"Residuals vs log(K_tiles)  (r={r_resid_ktiles:.3f})")
        ax.legend(fontsize=7)
        plt.tight_layout()
        plt.savefig("kdim_residual_check.png", dpi=120)
        plt.close()
        print("\nSaved plot: kdim_residual_check.png")


if __name__ == "__main__":
    main()
