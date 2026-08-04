"""
Tests, rather than assumes, the catastrophic-cancellation hypothesis for
why relative error varies so much across conv2d output positions.

Evidence chain so far (per debug session):
    large ULP        <- large relative error      [PROVEN: ratio
                                                     ULP/(rel_err/eps32)
                                                     stable at ~1.0-2.0
                                                     across 80 elements]
    large rel. error  <- catastrophic cancellation [NOT YET PROVEN --
                                                     this script tests it]

For each output position (n, oy, ox, cout), the true convolution sum is

    Y[n,oy,ox,cout] = sum over (ky,kx,cin) of X_padded[n,iy,ix,cin] * K[ky,kx,cin,cout]

This script recomputes that same sum but ALSO keeps sum(|term_i|) for
the same set of terms, and defines

    cancellation_ratio = sum(|term_i|) / |sum(term_i)|

(e.g. terms 100, -99, 98, -97 sum to 2, but sum(|term|)=394, giving a
ratio of 197 -- almost total cancellation. Terms of similar total
magnitude that mostly agree in sign would give a ratio near 1.)

It then reports the Pearson correlation (on log-log scale, matching the
convention already used elsewhere in this project for Kdim vs ULP)
between cancellation_ratio and relative error, per row and pooled
across all rows, plus a scatter plot, plus explicit side-by-side ratio
values for the worst-error elements vs a random sample of "typical"
elements -- so the correlation can be inspected directly, not just
summarized as a single r value.

This script draws no conclusion on its own; report the r value and the
plot, and let the reader (and the paper) judge based on that -- per the
"is consistent with" / "is likely due to" framing rather than "is
caused by" until this evidence is in hand.

Usage:
    python3 verify_cancellation_hypothesis.py --sweep-dir sweep_out \
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
    """Returns (Y_ref, sum_abs_terms), both shape (N,Hout,Wout,Cout).

    Y_ref[n,oy,ox,cout]        = sum_i term_i
    sum_abs_terms[n,oy,ox,cout] = sum_i |term_i|

    where the sum is over all (ky,kx,cin) reduction positions and
    term_i = X_padded[n,iy,ix,cin] * K[ky,kx,cin,cout]. Same float32
    rounding-then-float64-accumulation convention as the rest of this
    project's reference computation.
    """
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
                        x_vec = Xp[n, iy, ix, :]          # (Cin,)
                        k_mat = K[ky, kx, :, :]            # (Cin, Cout)
                        # terms[cin, cout] = x_vec[cin] * k_mat[cin, cout]
                        terms = x_vec[:, None] * k_mat      # (Cin, Cout)
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
    return Y_hw.astype(np.float64), Y_ref, sum_abs


def pearson_loglog(x, y):
    """log-log Pearson correlation, matching the convention already used
    elsewhere in this project (Kdim vs ULP)."""
    mask = (x > 0) & (y > 0) & np.isfinite(x) & np.isfinite(y)
    lx, ly = np.log(x[mask]), np.log(y[mask])
    if len(lx) < 3:
        return float("nan"), mask.sum()
    r = np.corrcoef(lx, ly)[0, 1]
    return r, mask.sum()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--sweep-dir", default="sweep_out")
    ap.add_argument("--rows", nargs="+", required=True)
    ap.add_argument("--bin", default="./run_conv2d_shape")
    ap.add_argument("--n-typical-sample", type=int, default=10,
                     help="How many 'typical' (median-error) elements to print for side-by-side comparison")
    args = ap.parse_args()

    with open(f"{args.sweep_dir}/sweep_results_template.csv") as f:
        rows = {r["name"]: r for r in csv.DictReader(f)}

    all_ratio, all_rel_err, all_names = [], [], []

    for name in args.rows:
        if name not in rows:
            print(f"WARNING: {name} not found, skipping", file=sys.stderr)
            continue
        out = run_row(name, rows[name], args.bin)
        if out is None:
            continue
        Y_hw, Y_ref, sum_abs = out

        abs_ref = np.abs(Y_ref)
        # Guard against exact-zero reference (shouldn't happen in
        # practice with continuous random test data, but avoid div-by-zero).
        cancellation_ratio = sum_abs / np.maximum(abs_ref, 1e-300)

        abs_err = np.abs(Y_hw - Y_ref)
        with np.errstate(divide="ignore", invalid="ignore"):
            rel_err = np.where(abs_ref > 0, abs_err / abs_ref, np.nan)

        flat_ratio = cancellation_ratio.flatten()
        flat_rel = rel_err.flatten()
        flat_ref = Y_ref.flatten()

        r, n_used = pearson_loglog(flat_ratio, flat_rel)

        print(f"\n{'='*70}")
        print(f"ROW: {name}")
        print(f"{'='*70}")
        print(f"log-log Pearson r (cancellation_ratio vs relative_error) = {r:.4f}  "
              f"(n={n_used}/{flat_ratio.size} finite/positive pairs)")

        order = np.argsort(-flat_rel)
        worst_idx = order[np.isfinite(flat_rel[order])][:5]
        print(f"\nTop 5 worst-relative-error elements -- cancellation ratio vs rel. error:")
        for idx in worst_idx:
            print(f"  cancellation_ratio={flat_ratio[idx]:>10.2f}   rel_err={flat_rel[idx]:.4e}   "
                  f"ref={flat_ref[idx]:.4g}")

        finite_mask = np.isfinite(flat_rel)
        median_rel = np.median(flat_rel[finite_mask])
        typical_idx = np.argsort(np.abs(flat_rel - median_rel))[:args.n_typical_sample]
        print(f"\n{args.n_typical_sample} 'typical' (near-median relative error) elements "
              f"-- cancellation ratio vs rel. error, for comparison:")
        for idx in typical_idx:
            print(f"  cancellation_ratio={flat_ratio[idx]:>10.2f}   rel_err={flat_rel[idx]:.4e}   "
                  f"ref={flat_ref[idx]:.4g}")

        if HAVE_MPL:
            plt.figure(figsize=(5, 4))
            plt.scatter(flat_ratio, flat_rel, s=8, alpha=0.5)
            plt.xscale("log"); plt.yscale("log")
            plt.xlabel("cancellation ratio"); plt.ylabel("relative error")
            plt.title(f"{name}: cancellation ratio vs rel. error (r={r:.3f})")
            plt.tight_layout()
            plt.savefig(f"cancellation_{name}.png", dpi=120)
            plt.close()
            print(f"\nSaved plot: cancellation_{name}.png")

        all_ratio.append(flat_ratio)
        all_rel_err.append(flat_rel)
        all_names.append(name)

    if not all_ratio:
        print("No rows successfully analyzed.", file=sys.stderr)
        return

    pooled_ratio = np.concatenate(all_ratio)
    pooled_rel = np.concatenate(all_rel_err)
    r_pooled, n_pooled = pearson_loglog(pooled_ratio, pooled_rel)

    print(f"\n{'='*70}")
    print(f"POOLED across {len(all_names)} rows ({all_names})")
    print(f"log-log Pearson r (cancellation_ratio vs relative_error) = {r_pooled:.4f}  "
          f"(n={n_pooled})")
    print(f"{'='*70}")

    if HAVE_MPL:
        plt.figure(figsize=(6, 5))
        for name, ratio, rel in zip(all_names, all_ratio, all_rel_err):
            plt.scatter(ratio, rel, s=6, alpha=0.4, label=name)
        plt.xscale("log"); plt.yscale("log")
        plt.xlabel("cancellation ratio"); plt.ylabel("relative error")
        plt.title(f"Pooled: cancellation ratio vs rel. error (r={r_pooled:.3f})")
        plt.legend(fontsize=7)
        plt.tight_layout()
        plt.savefig("cancellation_pooled.png", dpi=120)
        plt.close()
        print("Saved pooled plot: cancellation_pooled.png")


if __name__ == "__main__":
    main()
