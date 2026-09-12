#!/usr/bin/env python3
"""陣列組合掃描 —— 用 systolic-opt 實跑,供論文 §7.6.1。

config_sweep/results.md(2026-07)是 Python 重演,它自己寫著「進論文前
必須用 systolic-opt 復算」。這支就是那個復算,校正值換成 paper-hw-v1:

  DSP = 6N^2(不是 HLS 的 5RC),  k_max = 256,  H = 95(N=4 與 N=8 都是)

用法(在 repo 根目錄):
    python3 eval/config_sweep/sweep_opt.py
    python3 eval/config_sweep/sweep_opt.py <systolic-opt 的路徑>
"""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
DEFAULT_OPT = os.path.join(HERE, "..", "..", "build", "bin", "systolic-opt")
OPT = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_OPT

DSP_BUDGET = 740
DSP = {4: 6 * 4 * 4, 8: 6 * 8 * 8}
KMAX, H = 256, 95


def feasible():
    """列舉 96a + 384b <= 740 的所有 (a, b)。"""
    out = []
    for b in range(0, DSP_BUDGET // DSP[8] + 1):
        for a in range(0, (DSP_BUDGET - b * DSP[8]) // DSP[4] + 1):
            if a + b:
                out.append((a, b))
    return out


WORKLOADS = {
    "W1 16x64^3": [(64, 64, 64)] * 16,
    "W2 32x16^3": [(16, 16, 16)] * 32,
    "W3 4big+16sm": [(64, 64, 64)] * 4 + [(16, 16, 16)] * 16,
    "W4 2x64^3": [(64, 64, 64)] * 2,
}


def mlir(cfg, tiles):
    a, b = cfg
    L = [f'  systolic.device @s{i} rows = 4 cols = 4 dataflow = output_stationary '
         f'{{k_max = {KMAX} : i64, tile_overhead = {H} : i64}}' for i in range(a)]
    L += [f'  systolic.device @b{i} rows = 8 cols = 8 dataflow = output_stationary '
          f'{{k_max = {KMAX} : i64, tile_overhead = {H} : i64}}' for i in range(b)]
    for n, (M, N, K) in enumerate(tiles):
        L.append(
            f'  func.func @t{n}(%a: tensor<{M}x{K}xf32>, %b: tensor<{K}x{N}xf32>,'
            f' %c: tensor<{M}x{N}xf32>) -> tensor<{M}x{N}xf32> {{\n'
            f'    %0 = systolic.matmul_tile %a, %b, %c '
            f'{{m = {M} : i64, n = {N} : i64, k = {K} : i64}}\n'
            f'         : (tensor<{M}x{K}xf32>, tensor<{K}x{N}xf32>, tensor<{M}x{N}xf32>)'
            f' -> tensor<{M}x{N}xf32>\n    return %0 : tensor<{M}x{N}xf32>\n  }}')
    return "module {\n" + "\n".join(L) + "\n}\n"


def makespan(cfg, tiles):
    """makespan = 各裝置累計 est_cycles 的最大值。

    pass 只標每個 tile 的 est_cycles 和它被指派到哪一台,makespan 要自己
    依裝置分組加總 —— 這一步不要省,不然量到的是總工作量不是完工時間。
    """
    p = subprocess.run([OPT, "--systolic-select-device"], input=mlir(cfg, tiles),
                       capture_output=True, text=True)
    if p.returncode:
        raise RuntimeError(p.stderr[:400])
    load = {}
    for dev, cyc in re.findall(r"on @(\w+) \{est_cycles = (\d+)", p.stdout):
        load[dev] = load.get(dev, 0) + int(cyc)
    return max(load.values()) if load else 0


if __name__ == "__main__":
    if not os.path.exists(OPT):
        sys.exit(f"找不到 systolic-opt: {OPT}\n"
                 f"先 ./configure.sh --build,或把路徑當參數傳進來。")
    cfgs = feasible()
    print(f"DSP {DSP_BUDGET};  4x4={DSP[4]}  8x8={DSP[8]};  feasible {len(cfgs)}")
    res = {w: {c: makespan(c, t) for c in cfgs} for w, t in WORKLOADS.items()}
    best = {w: min(v.values()) for w, v in res.items()}
    print(f"\n{'config':<14}" + "".join(f"{w:>18}" for w in WORKLOADS) + f"{'worst':>8}")
    rank = []
    for c in cfgs:
        regs = [res[w][c] / best[w] for w in WORKLOADS]
        rank.append((max(regs), c))
        print(f"{f'{c[0]}x4x4+{c[1]}x8x8':<14}"
              + "".join(f"{res[w][c]:>12}({res[w][c]/best[w]:.2f})" for w in WORKLOADS)
              + f"{max(regs):>8.2f}")
    rank.sort()
    print(f"\nminimax regret: {rank[0][1][0]}x4x4+{rank[0][1][1]}x8x8"
          f"  (worst {rank[0][0]:.2f}x)")
