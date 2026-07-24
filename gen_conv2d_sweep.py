#!/usr/bin/env python3
"""
Generates a systematic conv2d shape/kernel/stride/dilation sweep, analogous
in spirit to the five-shape matmul evaluation (Table 1 / Section 5.1), to
replace the "two configurations" conv2d evaluation flagged as too thin in
Limitations.

This script does NOT run anything on hardware and does NOT fabricate any
numbers -- it only emits (1) one .mlir test file per configuration, ready
to push through --conv2d-to-fpga and your existing toolchain, and (2) a
CSV template with the Max error (ULP) and Tiles columns left blank for you
to fill in after running each one against the physical FPGA, the same way
Table 1's numbers were originally collected.

Usage:
    python3 gen_conv2d_sweep.py --outdir sweep_out

Then for each generated .mlir file:
    systolic-opt --conv2d-to-fpga sweep_out/<name>.mlir | <rest of your
        existing Section 4.3 toolchain> -> run on hardware -> record the
        measured ULP error and tile count in sweep_results.csv
"""
import argparse
import csv
import itertools
import os
import random

# Sweep axes. Kept deliberately small per-axis so the full cross product
# is a few dozen configs, not thousands -- large enough to be a real
# "systematic sweep" claim, small enough to actually run on one board in
# a reasonable amount of time given the ~30ms/tile UART latency measured
# in Section 5.6.
KERNEL_SIZES = [(1, 1), (3, 3), (5, 5), (3, 5)]          # (Kh, Kw)
CHANNELS = [(1, 1), (3, 8), (8, 16)]                      # (Cin, Cout)
STRIDES = [(1, 1), (2, 2), (1, 2)]                        # (strideH, strideW)
DILATIONS = [(1, 1), (2, 1)]                              # (dilationH, dilationW)
INPUT_HW = [(8, 8), (10, 10)]                             # (H, W), pre-pad
PAD = [(0, 0, 0, 0), (1, 1, 1, 1)]                        # (top, bottom, left, right)
BATCH = [1, 2]

MLIR_TEMPLATE = """\
// Auto-generated shape-sweep test case: {name}
// N={N} H={H} W={W} Cin={Cin} Kh={Kh} Kw={Kw} Cout={Cout}
// stride=({strideH},{strideW}) dilation=({dilationH},{dilationW})
// pad=(top={padTop},bottom={padBottom},left={padLeft},right={padRight})
func.func @{name}(%input: tensor<{N}x{H}x{W}x{Cin}xf32>,
                   %filter: tensor<{Kh}x{Kw}x{Cin}x{Cout}xf32>) -> tensor<{N}x{Hout}x{Wout}x{Cout}xf32> {{
  %cst = arith.constant 0.0 : f32
  %padded = tensor.pad %input low[0, {padTop}, {padLeft}, 0] high[0, {padBottom}, {padRight}, 0] {{
  ^bb0(%a: index, %b: index, %c: index, %d: index):
    tensor.yield %cst : f32
  }} : tensor<{N}x{H}x{W}x{Cin}xf32> to tensor<{N}x{Hpadded}x{Wpadded}x{Cin}xf32>
  %init = tensor.empty() : tensor<{N}x{Hout}x{Wout}x{Cout}xf32>
  %out = linalg.conv_2d_nhwc_hwcf
      {{strides = dense<[{strideH}, {strideW}]> : tensor<2xi64>,
        dilations = dense<[{dilationH}, {dilationW}]> : tensor<2xi64>}}
      ins(%padded, %filter : tensor<{N}x{Hpadded}x{Wpadded}x{Cin}xf32>, tensor<{Kh}x{Kw}x{Cin}x{Cout}xf32>)
      outs(%init : tensor<{N}x{Hout}x{Wout}x{Cout}xf32>) -> tensor<{N}x{Hout}x{Wout}x{Cout}xf32>
  return %out : tensor<{N}x{Hout}x{Wout}x{Cout}xf32>
}}
"""


def conv_out_dim(dim_padded, k, stride, dilation):
    return (dim_padded - dilation * (k - 1) - 1) // stride + 1


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--outdir", default="sweep_out")
    ap.add_argument("--max-configs", type=int, default=48,
                     help="cap on total configs emitted, chosen as a "
                          "random sample of all FEASIBLE combinations, not "
                          "a prefix of itertools.product's iteration order "
                          "-- a plain prefix would get stuck on the first "
                          "kernel size for any reasonable cap, since each "
                          "kernel size alone has more feasible combinations "
                          "than a typical cap.")
    ap.add_argument("--seed", type=int, default=0,
                     help="RNG seed for which configs get sampled, for "
                          "reproducibility across re-runs.")
    args = ap.parse_args()
    os.makedirs(args.outdir, exist_ok=True)

    # First pass: enumerate every FEASIBLE combination (kernel fits in the
    # padded input) across the full cross product, before any capping.
    # This is what gets sampled from, so the cap can't get stuck on one
    # axis the way taking a prefix in itertools.product order would.
    feasible = []
    for (Kh, Kw), (Cin, Cout), (sH, sW), (dH, dW), (H, W), \
        (padTop, padBottom, padLeft, padRight), N in itertools.product(
            KERNEL_SIZES, CHANNELS, STRIDES, DILATIONS, INPUT_HW, PAD, BATCH):
        Hpadded = H + padTop + padBottom
        Wpadded = W + padLeft + padRight
        if Hpadded < dH * (Kh - 1) + 1 or Wpadded < dW * (Kw - 1) + 1:
            continue
        feasible.append((Kh, Kw, Cin, Cout, sH, sW, dH, dW, H, W,
                          padTop, padBottom, padLeft, padRight, N))

    total_feasible = len(feasible)
    rng = random.Random(args.seed)
    if len(feasible) > args.max_configs:
        rng.shuffle(feasible)
        selected = feasible[:args.max_configs]
    else:
        selected = feasible
    skipped = total_feasible - len(selected)

    rows = []
    for idx, (Kh, Kw, Cin, Cout, sH, sW, dH, dW, H, W,
              padTop, padBottom, padLeft, padRight, N) in enumerate(selected, start=1):

        Hpadded = H + padTop + padBottom
        Wpadded = W + padLeft + padRight
        Hout = conv_out_dim(Hpadded, Kh, sH, dH)
        Wout = conv_out_dim(Wpadded, Kw, sW, dW)
        name = f"conv_sweep_{idx:03d}"

        mlir_text = MLIR_TEMPLATE.format(
            name=name, N=N, H=H, W=W, Cin=Cin, Kh=Kh, Kw=Kw, Cout=Cout,
            strideH=sH, strideW=sW, dilationH=dH, dilationW=dW,
            padTop=padTop, padBottom=padBottom, padLeft=padLeft, padRight=padRight,
            Hpadded=Hpadded, Wpadded=Wpadded, Hout=Hout, Wout=Wout)

        with open(os.path.join(args.outdir, f"{name}.mlir"), "w") as f:
            f.write(mlir_text)

        # Expected tile-invocation count under the existing 4x4 tiling
        # scheme (Section 4.1/4.4), for cross-checking against what you
        # observe on hardware -- this is a prediction from the paper's own
        # tiling formula, not a measured result.
        colRows = Hout * Wout * N
        colCols = Kh * Kw * Cin
        predicted_tiles = (
            -(-colRows // 4) * -(-colCols // 4) * -(-Cout // 4)
        )

        rows.append({
            "name": name, "N": N, "H": H, "W": W, "Cin": Cin,
            "Kh": Kh, "Kw": Kw, "Cout": Cout,
            "strideH": sH, "strideW": sW, "dilationH": dH, "dilationW": dW,
            "padTop": padTop, "padBottom": padBottom,
            "padLeft": padLeft, "padRight": padRight,
            "Hout": Hout, "Wout": Wout,
            "predicted_tiles": predicted_tiles,
            "measured_tiles": "",       # fill in after running on hardware
            "max_error_ulp": "",        # fill in after running on hardware
            "result": "",               # PASS / FAIL, fill in after running
        })

    csv_path = os.path.join(args.outdir, "sweep_results_template.csv")
    with open(csv_path, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)

    print(f"Wrote {len(rows)} .mlir test cases to {args.outdir}/")
    print(f"Wrote results template to {csv_path}")
    print(f"Skipped {skipped} configs (either infeasible kernel/input "
          f"combinations, or beyond --max-configs={args.max_configs}).")


if __name__ == "__main__":
    main()
