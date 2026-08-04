"""
Objective investigation of the conv2d ULP phenomenon. Does NOT presuppose
any explanation (in particular, does NOT presuppose the "near-zero
reference value inflates ULP" hypothesis) -- it computes the raw numbers
and leaves interpretation to whoever reads the output.

For each named sweep row, this script:
  1. Regenerates the exact same (deterministic-seed) test data used by
     run_conv2d_sweep.py.
  2. Computes a double-precision reference, rounding operands to float32
     first (matching what is actually sent to hardware) -- the same
     corrected methodology already used elsewhere in this project.
  3. Runs the real hardware binary (./run_conv2d_shape) to get the
     measured output.
  4. Computes, per output element: ULP, absolute error, relative error,
     and whether |reference| falls under each of several near-zero
     thresholds (1e-6, 1e-5, 1e-4).
  5. Prints the top-K elements by ULP with every requested column.
  6. Prints aggregate statistics: max/RMSE/MAE for absolute error, max
     relative error, max ULP, and the proportion of near-zero outputs at
     each threshold.
  7. Saves three scatter plots per row (|reference| vs ULP, absolute
     error vs ULP, relative error vs ULP), plus one pooled plot across
     all analyzed rows, to help visually judge whether ULP is
     concentrated at near-zero references or not.
  8. Recomputes max ULP after excluding elements with |reference| < eps,
     for eps in {1e-6, 1e-5, 1e-4} -- this is the key test of the
     near-zero hypothesis: if excluding near-zero elements collapses the
     max ULP down near the "normal" range, that supports the hypothesis;
     if large ULP values persist even after exclusion, that refutes it.

Usage:
    python3 investigate_conv2d_ulp.py --sweep-dir sweep_out \
        --rows conv_sweep_004 conv_sweep_029 conv_sweep_042 conv_sweep_032 \
        --bin ./run_conv2d_shape --topk 20
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


def reference_conv2d_padded(X, K, strideH, strideW, dilH, dilW,
                             padTop, padBottom, padLeft, padRight):
    """Rounds X, K to float32 first (matching what is actually sent to
    hardware), then upcasts to float64 before computing the reference."""
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
    ai = a.astype(np.float32).view(np.int32).astype(np.int64)
    bi = b.astype(np.float32).view(np.int32).astype(np.int64)
    ai = np.where(ai < 0, np.int64(0x80000000) - ai, ai)
    bi = np.where(bi < 0, np.int64(0x80000000) - bi, bi)
    return np.abs(ai - bi)


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

    Y_ref, Hout, Wout = reference_conv2d_padded(
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
    return Y_hw.astype(np.float64), Y_ref, (N, Hout, Wout, Cout)


def analyze_row(name, Y_hw, Y_ref, shape, topk, thresholds):
    N, Hout, Wout, Cout = shape
    ulp = ulp_diff(Y_hw, Y_ref)
    abs_err = np.abs(Y_hw - Y_ref)
    ref_abs = np.abs(Y_ref)
    with np.errstate(divide="ignore", invalid="ignore"):
        rel_err = np.where(ref_abs > 0, abs_err / ref_abs, np.inf)

    flat_ulp = ulp.flatten()
    flat_abs = abs_err.flatten()
    flat_rel = rel_err.flatten()
    flat_ref = Y_ref.flatten()
    flat_hw = Y_hw.flatten()

    order = np.argsort(-flat_ulp)

    print(f"\n{'='*70}")
    print(f"ROW: {name}  (shape N={N} Hout={Hout} Wout={Wout} Cout={Cout}, "
          f"{flat_ulp.size} total output elements)")
    print(f"{'='*70}")

    print(f"\nTop {topk} elements by ULP:")
    header = (f"{'idx(n,oy,ox,cout)':<22}{'ref':>14}{'hw':>14}"
              f"{'abs_err':>14}{'rel_err':>14}{'ULP':>10}  near-zero@(1e-6,1e-5,1e-4)")
    print(header)
    for idx in order[:topk]:
        n_i = idx // (Hout * Wout * Cout)
        rem = idx % (Hout * Wout * Cout)
        oy_i = rem // (Wout * Cout)
        rem2 = rem % (Wout * Cout)
        ox_i = rem2 // Cout
        co_i = rem2 % Cout
        r = flat_ref[idx]
        rel_str = f"{flat_rel[idx]:.3e}" if np.isfinite(flat_rel[idx]) else "inf(ref=0)"
        nz_flags = tuple(abs(r) < t for t in thresholds)
        idx_str = f"({n_i},{oy_i},{ox_i},{co_i})"
        print(f"{idx_str:<22}{r:>14.6g}{flat_hw[idx]:>14.6g}"
              f"{flat_abs[idx]:>14.6g}{rel_str:>14}{flat_ulp[idx]:>10.0f}  {nz_flags}")

    print(f"\nAggregate statistics ({name}):")
    print(f"  max ULP           = {flat_ulp.max():.0f}")
    print(f"  max abs error     = {flat_abs.max():.6g}")
    finite_rel = flat_rel[np.isfinite(flat_rel)]
    print(f"  max rel error     = {finite_rel.max():.6g}"
          f"  ({flat_rel.size - finite_rel.size} elements had ref==0, excluded from this max)")
    print(f"  RMSE (abs)        = {np.sqrt(np.mean(flat_abs**2)):.6g}")
    print(f"  MAE  (abs)        = {np.mean(flat_abs):.6g}")
    print(f"  |reference| scale: median={np.median(np.abs(flat_ref)):.4g}  "
          f"max={np.abs(flat_ref).max():.4g}  (for interpreting the near-zero "
          f"thresholds below relative to this row's actual output magnitude)")
    for t in thresholds:
        frac = np.mean(np.abs(flat_ref) < t)
        print(f"  fraction |ref| < {t:.0e}  = {frac:.4f} ({int(frac*flat_ref.size)}/{flat_ref.size})")

    print(f"\nMax ULP after excluding |ref| < eps (near-zero-exclusion test):")
    for t in thresholds:
        mask = np.abs(flat_ref) >= t
        if mask.sum() == 0:
            print(f"  eps={t:.0e}: no elements remain (all references below threshold)")
            continue
        print(f"  eps={t:.0e}: max ULP among remaining {mask.sum()}/{flat_ulp.size} elements "
              f"= {flat_ulp[mask].max():.0f}")

    # Relative-to-this-row's-own-scale near-zero test: a fixed absolute
    # threshold can miss a "proportionally near-zero" element in a row
    # whose typical output magnitude is unusually large (or catch too much
    # in a row whose typical magnitude is small) -- this repeats the same
    # exclusion test using a threshold defined as a fraction of THIS row's
    # own median |reference|, rather than a single absolute number shared
    # across all rows.
    median_ref = np.median(np.abs(flat_ref))
    rel_thresholds = [1e-5, 1e-4, 1e-3, 1e-2]
    print(f"\nMax ULP after excluding |ref| < (relative_eps * row median |ref| = "
          f"{median_ref:.4g}) -- relative-to-own-scale near-zero test:")
    for rt in rel_thresholds:
        eps = rt * median_ref
        mask = np.abs(flat_ref) >= eps
        if mask.sum() == 0:
            print(f"  relative_eps={rt:.0e} (abs eps={eps:.4g}): no elements remain")
            continue
        print(f"  relative_eps={rt:.0e} (abs eps={eps:.4g}): max ULP among remaining "
              f"{mask.sum()}/{flat_ulp.size} elements = {flat_ulp[mask].max():.0f}")

    # Second-tier check: after removing ONLY the single worst-ULP element,
    # is the NEXT-worst element's reference also small relative to this
    # row's scale, or does the "elevated but not extreme" tail have some
    # other character? Report the top 5 remaining elements' |ref|/median
    # ratio explicitly so this can be inspected directly rather than
    # inferred only from the aggregate max.
    order_desc = np.argsort(-flat_ulp)
    print(f"\nTop 5 elements' |ref| relative to this row's median |ref| "
          f"(checking whether the 'second tier' of elevated-but-not-extreme "
          f"ULP is also small-reference, or something else):")
    for rank, idx in enumerate(order_desc[:5]):
        ratio = abs(flat_ref[idx]) / median_ref if median_ref > 0 else float("nan")
        print(f"  #{rank+1}: ULP={flat_ulp[idx]:.0f}  |ref|={abs(flat_ref[idx]):.4g}  "
              f"|ref|/median={ratio:.3e}")

    if HAVE_MPL:
        fig, axes = plt.subplots(1, 3, figsize=(15, 4))
        axes[0].scatter(ref_abs.flatten(), flat_ulp, s=8, alpha=0.5)
        axes[0].set_xscale("log"); axes[0].set_yscale("log")
        axes[0].set_xlabel("|reference|"); axes[0].set_ylabel("ULP")
        axes[0].set_title(f"{name}: |ref| vs ULP")

        axes[1].scatter(flat_abs, flat_ulp, s=8, alpha=0.5)
        axes[1].set_xscale("log"); axes[1].set_yscale("log")
        axes[1].set_xlabel("absolute error"); axes[1].set_ylabel("ULP")
        axes[1].set_title(f"{name}: abs error vs ULP")

        axes[2].scatter(finite_rel, flat_ulp[np.isfinite(flat_rel)], s=8, alpha=0.5)
        axes[2].set_xscale("log"); axes[2].set_yscale("log")
        axes[2].set_xlabel("relative error"); axes[2].set_ylabel("ULP")
        axes[2].set_title(f"{name}: rel error vs ULP")

        plt.tight_layout()
        plt.savefig(f"ulp_investigation_{name}.png", dpi=120)
        plt.close()
        print(f"\nSaved plot: ulp_investigation_{name}.png")

    return {
        "name": name, "ulp": flat_ulp, "abs_err": flat_abs,
        "rel_err": flat_rel, "ref": flat_ref,
    }


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--sweep-dir", default="sweep_out")
    ap.add_argument("--rows", nargs="+", required=True)
    ap.add_argument("--bin", default="./run_conv2d_shape")
    ap.add_argument("--topk", type=int, default=20)
    args = ap.parse_args()

    thresholds = [1e-6, 1e-5, 1e-4, 1e-3, 1e-2, 1e-1]

    with open(f"{args.sweep_dir}/sweep_results_template.csv") as f:
        rows = {r["name"]: r for r in csv.DictReader(f)}

    all_results = []
    for name in args.rows:
        if name not in rows:
            print(f"WARNING: {name} not found in {args.sweep_dir}/sweep_results_template.csv, skipping", file=sys.stderr)
            continue
        out = run_row(name, rows[name], args.bin)
        if out is None:
            continue
        Y_hw, Y_ref, shape = out
        res = analyze_row(name, Y_hw, Y_ref, shape, args.topk, thresholds)
        all_results.append(res)

    if not all_results:
        print("No rows successfully analyzed.", file=sys.stderr)
        return

    if HAVE_MPL and len(all_results) > 1:
        fig, axes = plt.subplots(1, 3, figsize=(15, 4))
        for res in all_results:
            ref_abs = np.abs(res["ref"])
            axes[0].scatter(ref_abs, res["ulp"], s=6, alpha=0.4, label=res["name"])
            axes[1].scatter(res["abs_err"], res["ulp"], s=6, alpha=0.4, label=res["name"])
            finite = np.isfinite(res["rel_err"])
            axes[2].scatter(res["rel_err"][finite], res["ulp"][finite], s=6, alpha=0.4, label=res["name"])
        for ax, xlabel in zip(axes, ["|reference|", "absolute error", "relative error"]):
            ax.set_xscale("log"); ax.set_yscale("log")
            ax.set_xlabel(xlabel); ax.set_ylabel("ULP")
            ax.legend(fontsize=7)
        axes[0].set_title("Pooled: |ref| vs ULP")
        axes[1].set_title("Pooled: abs error vs ULP")
        axes[2].set_title("Pooled: rel error vs ULP")
        plt.tight_layout()
        plt.savefig("ulp_investigation_pooled.png", dpi=120)
        plt.close()
        print("\nSaved pooled plot across all rows: ulp_investigation_pooled.png")

    print(f"\n{'='*70}")
    print("SUMMARY ACROSS ALL ANALYZED ROWS (raw numbers only -- no")
    print("conclusion drawn here; inspect the plots and per-row stats above)")
    print(f"{'='*70}")
    for res in all_results:
        print(f"  {res['name']:<20} max_ULP={res['ulp'].max():>10.0f}  "
              f"max_abs_err={res['abs_err'].max():.6g}")


if __name__ == "__main__":
    main()
