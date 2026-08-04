#!/usr/bin/env python3
"""
sweep_harness.py — 為 conv2d shape sweep 產生 golden reference 並量測 ULP 誤差。

用法:
  # 1. 先驗證 golden 本身是對的（拿 naive 迴圈當交叉檢查）
  python3 sweep_harness.py --selftest

  # 2. 只產生輸入 / golden 輸出的 .bin，不跑 DUT
  python3 sweep_harness.py --csv sweep_out/sweep_results_template.csv \
                           --workdir /tmp/sweep_work --emit-only

  # 3. 完整跑：對每組呼叫 DUT，回讀輸出，算 ULP，填 csv
  python3 sweep_harness.py --csv sweep_out/sweep_results_template.csv \
                           --workdir /tmp/sweep_work \
                           --out sweep_out/sweep_results_filled.csv

要接上你的 pipeline，只需要改 run_dut()（見檔案下方）。

資料 layout（全部 float32、little-endian、row-major、無 header）:
  input  : N  x H  x W  x Cin      (NHWC，未 padding 的原始輸入)
  filter : Kh x Kw x Cin x Cout    (HWCF)
  output : N  x Hout x Wout x Cout (NHWC)
"""

import argparse
import csv
import os
import subprocess
import sys

import numpy as np

# ----------------------------------------------------------------------
# shape 計算
# ----------------------------------------------------------------------

def out_dim(in_size, pad_lo, pad_hi, k, dilation, stride):
    eff_k = (k - 1) * dilation + 1
    return (in_size + pad_lo + pad_hi - eff_k) // stride + 1


def gemm_dims(cfg):
    """im2col 之後的 GEMM 維度。"""
    M = cfg["N"] * cfg["Hout"] * cfg["Wout"]
    N = cfg["Cout"]
    K = cfg["Cin"] * cfg["Kh"] * cfg["Kw"]
    return M, N, K


def predicted_tiles(cfg, R=4, C=4, KT=4):
    M, N, K = gemm_dims(cfg)
    return -(-M // R) * -(-N // C) * -(-K // KT)


# ----------------------------------------------------------------------
# golden reference
# ----------------------------------------------------------------------

def conv2d_golden(x, w, cfg, dtype=np.float64):
    """
    x : (N, H, W, Cin)   未 padding
    w : (Kh, Kw, Cin, Cout)
    回傳 (N, Hout, Wout, Cout)

    以 float64 累加。這是 ground truth，不是要模仿硬體。
    """
    x = x.astype(dtype)
    w = w.astype(dtype)
    N, H, W, Cin = x.shape
    Kh, Kw, _, Cout = w.shape
    sh, sw = cfg["strideH"], cfg["strideW"]
    dh, dw = cfg["dilationH"], cfg["dilationW"]
    pt, pb = cfg["padTop"], cfg["padBottom"]
    pl, pr = cfg["padLeft"], cfg["padRight"]

    xp = np.pad(x, ((0, 0), (pt, pb), (pl, pr), (0, 0)), mode="constant")
    Hout = out_dim(H, pt, pb, Kh, dh, sh)
    Wout = out_dim(W, pl, pr, Kw, dw, sw)

    # im2col: (N*Hout*Wout, Kh*Kw*Cin)
    cols = np.empty((N * Hout * Wout, Kh * Kw * Cin), dtype=dtype)
    idx = 0
    for n in range(N):
        for oh in range(Hout):
            for ow in range(Wout):
                patch = xp[n,
                           oh * sh: oh * sh + (Kh - 1) * dh + 1: dh,
                           ow * sw: ow * sw + (Kw - 1) * dw + 1: dw,
                           :]
                cols[idx] = patch.reshape(-1)
                idx += 1
    wmat = w.reshape(Kh * Kw * Cin, Cout)
    y = cols @ wmat
    return y.reshape(N, Hout, Wout, Cout)


def conv2d_naive(x, w, cfg):
    """最笨的七層迴圈版本，只用來 self-test golden。"""
    x = x.astype(np.float64)
    w = w.astype(np.float64)
    N, H, W, Cin = x.shape
    Kh, Kw, _, Cout = w.shape
    sh, sw = cfg["strideH"], cfg["strideW"]
    dh, dw = cfg["dilationH"], cfg["dilationW"]
    pt, pb = cfg["padTop"], cfg["padBottom"]
    pl, pr = cfg["padLeft"], cfg["padRight"]
    xp = np.pad(x, ((0, 0), (pt, pb), (pl, pr), (0, 0)), mode="constant")
    Hout = out_dim(H, pt, pb, Kh, dh, sh)
    Wout = out_dim(W, pl, pr, Kw, dw, sw)
    y = np.zeros((N, Hout, Wout, Cout), dtype=np.float64)
    for n in range(N):
        for oh in range(Hout):
            for ow in range(Wout):
                for co in range(Cout):
                    acc = 0.0
                    for kh in range(Kh):
                        for kw in range(Kw):
                            for ci in range(Cin):
                                acc += (xp[n, oh * sh + kh * dh,
                                           ow * sw + kw * dw, ci]
                                        * w[kh, kw, ci, co])
                    y[n, oh, ow, co] = acc
    return y


# ----------------------------------------------------------------------
# ULP 距離
# ----------------------------------------------------------------------

def ulp_distance(a, b):
    """
    a, b : float32 array。回傳兩者之間相差幾個 ULP（整數距離）。
    用 monotonic ordinal 映射處理正負號。
    """
    ai = a.astype(np.float32).view(np.int32).astype(np.int64)
    bi = b.astype(np.float32).view(np.int32).astype(np.int64)
    # 負數轉成單調遞增的序數
    ai = np.where(ai < 0, np.int64(0x80000000) - ai, ai)
    bi = np.where(bi < 0, np.int64(0x80000000) - bi, bi)
    return np.abs(ai - bi)


def rel_error(ref64, got32):
    ref = ref64.astype(np.float64).ravel()
    got = got32.astype(np.float64).ravel()
    denom = np.maximum(np.abs(ref), 1e-30)
    return np.abs(got - ref) / denom


# ----------------------------------------------------------------------
# csv 解析
# ----------------------------------------------------------------------

INT_FIELDS = ["N", "H", "W", "Cin", "Kh", "Kw", "Cout",
              "strideH", "strideW", "dilationH", "dilationW",
              "padTop", "padBottom", "padLeft", "padRight",
              "Hout", "Wout"]


def load_configs(path):
    cfgs = []
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            cfg = {"name": row["name"]}
            for k in INT_FIELDS:
                cfg[k] = int(row[k])
            cfg["_row"] = row
            cfgs.append(cfg)
    return cfgs


def check_shapes(cfg):
    """驗證 csv 記的 Hout/Wout 跟公式一致——不一致代表 generator 或 csv 有問題。"""
    h = out_dim(cfg["H"], cfg["padTop"], cfg["padBottom"],
                cfg["Kh"], cfg["dilationH"], cfg["strideH"])
    w = out_dim(cfg["W"], cfg["padLeft"], cfg["padRight"],
                cfg["Kw"], cfg["dilationW"], cfg["strideW"])
    return (h == cfg["Hout"] and w == cfg["Wout"]), h, w


# ----------------------------------------------------------------------
# DUT hook —— 這是唯一需要你改的地方
# ----------------------------------------------------------------------

def run_dut(cfg, workdir, dut_cmd):
    """
    跑你的 pipeline，回傳 float32 的輸出 array，shape = (N, Hout, Wout, Cout)。
    失敗回傳 None。

    預設實作：把 dut_cmd 當 shell 樣板展開後執行，然後讀 {name}_Y_dut.bin。
    樣板可用的欄位：{name} {workdir} {mlir} {X} {K} {Y} 以及所有 shape 參數。

    例如:
      --dut-cmd './build/bin/systolic-run --input sweep_out/{name}.mlir \
                  --x {X} --k {K} --out {Y}'

    如果你的流程是「mlir-opt -> HLS C -> gcc -> 執行」，把那串包成一個
    shell script，這裡呼叫那個 script 即可。
    """
    if not dut_cmd:
        return None
    name = cfg["name"]
    fmt = dict(cfg)
    fmt.update({
        "workdir": workdir,
        "mlir": f"sweep_out/{name}.mlir",
        "X": os.path.join(workdir, f"{name}_X.bin"),
        "K": os.path.join(workdir, f"{name}_K.bin"),
        "Y": os.path.join(workdir, f"{name}_Y_dut.bin"),
    })
    cmd = dut_cmd.format(**fmt)
    try:
        subprocess.run(cmd, shell=True, check=True,
                       stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    except subprocess.CalledProcessError as e:
        print(f"  [{name}] DUT 失敗: {e.stderr.decode()[:200]}", file=sys.stderr)
        return None
    ypath = fmt["Y"]
    if not os.path.exists(ypath):
        print(f"  [{name}] DUT 沒有產生 {ypath}", file=sys.stderr)
        return None
    n = cfg["N"] * cfg["Hout"] * cfg["Wout"] * cfg["Cout"]
    arr = np.fromfile(ypath, dtype=np.float32)
    if arr.size != n:
        print(f"  [{name}] 輸出大小不符: 讀到 {arr.size}, 預期 {n}",
              file=sys.stderr)
        return None
    return arr.reshape(cfg["N"], cfg["Hout"], cfg["Wout"], cfg["Cout"])


# ----------------------------------------------------------------------
# 主流程
# ----------------------------------------------------------------------

def process(cfg, workdir, dut_cmd, seed, emit_only):
    name = cfg["name"]
    rng = np.random.default_rng(seed + int(name.split("_")[-1]))

    x = rng.standard_normal(
        (cfg["N"], cfg["H"], cfg["W"], cfg["Cin"])).astype(np.float32)
    w = rng.standard_normal(
        (cfg["Kh"], cfg["Kw"], cfg["Cin"], cfg["Cout"])).astype(np.float32)

    x.tofile(os.path.join(workdir, f"{name}_X.bin"))
    w.tofile(os.path.join(workdir, f"{name}_K.bin"))

    ref64 = conv2d_golden(x, w, cfg)                       # float64 ground truth
    ref32 = conv2d_golden(x, w, cfg, dtype=np.float32)     # fp32 累加參考
    ref64.astype(np.float32).tofile(
        os.path.join(workdir, f"{name}_Y_golden.bin"))

    rec = {
        "name": name,
        "M": gemm_dims(cfg)[0], "N_gemm": gemm_dims(cfg)[1],
        "K": gemm_dims(cfg)[2],
        "tiles": predicted_tiles(cfg),
        # fp32 軟體累加相對 fp64 的誤差 —— 這是「數值本身」的下限
        "sw_fp32_ulp": int(ulp_distance(ref32, ref64.astype(np.float32)).max()),
        "sw_fp32_rel": float(rel_error(ref64, ref32).max()),
        "dut_ulp": None, "dut_rel": None, "result": "emit-only",
    }

    if emit_only:
        return rec

    got = run_dut(cfg, workdir, dut_cmd)
    if got is None:
        rec["result"] = "DUT_FAIL"
        return rec

    rec["dut_ulp"] = int(ulp_distance(got, ref64.astype(np.float32)).max())
    rec["dut_rel"] = float(rel_error(ref64, got).max())
    # 判定：DUT 誤差若與軟體 fp32 累加同量級，就是正常數值行為
    rec["result"] = "OK" if rec["dut_ulp"] <= max(64, rec["sw_fp32_ulp"] * 8) \
                    else "MISMATCH"
    return rec


def selftest():
    """用小 shape 交叉驗證 golden：im2col 版 vs naive 七層迴圈。"""
    rng = np.random.default_rng(0)
    cases = [
        dict(N=1, H=8, W=8, Cin=8, Kh=5, Kw=5, Cout=16,
             strideH=1, strideW=2, dilationH=1, dilationW=1,
             padTop=1, padBottom=1, padLeft=1, padRight=1),   # 029
        dict(N=2, H=10, W=10, Cin=8, Kh=3, Kw=3, Cout=16,
             strideH=1, strideW=2, dilationH=1, dilationW=1,
             padTop=0, padBottom=0, padLeft=0, padRight=0),   # 042
        dict(N=1, H=8, W=8, Cin=1, Kh=3, Kw=5, Cout=1,
             strideH=1, strideW=2, dilationH=2, dilationW=1,
             padTop=1, padBottom=1, padLeft=1, padRight=1),   # 002
    ]
    ok = True
    for i, cfg in enumerate(cases):
        x = rng.standard_normal(
            (cfg["N"], cfg["H"], cfg["W"], cfg["Cin"])).astype(np.float32)
        w = rng.standard_normal(
            (cfg["Kh"], cfg["Kw"], cfg["Cin"], cfg["Cout"])).astype(np.float32)
        a = conv2d_golden(x, w, cfg)
        b = conv2d_naive(x, w, cfg)
        same = a.shape == b.shape and np.allclose(a, b, rtol=0, atol=1e-12)
        Hout = out_dim(cfg["H"], cfg["padTop"], cfg["padBottom"],
                       cfg["Kh"], cfg["dilationH"], cfg["strideH"])
        Wout = out_dim(cfg["W"], cfg["padLeft"], cfg["padRight"],
                       cfg["Kw"], cfg["dilationW"], cfg["strideW"])
        print(f"case {i}: shape={a.shape} Hout/Wout={Hout}/{Wout} "
              f"match={'YES' if same else 'NO'} "
              f"maxdiff={np.abs(a - b).max():.3e}")
        ok = ok and same
    print("\nselftest:", "PASS" if ok else "FAIL")
    return 0 if ok else 1


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--csv")
    ap.add_argument("--workdir", default="/tmp/sweep_work")
    ap.add_argument("--out")
    ap.add_argument("--dut-cmd", default=None,
                    help="shell 樣板，見 run_dut() 的 docstring")
    ap.add_argument("--seed", type=int, default=1234)
    ap.add_argument("--emit-only", action="store_true")
    ap.add_argument("--selftest", action="store_true")
    ap.add_argument("--only", default=None,
                    help="逗號分隔的 config 編號，例如 004,029,032,042")
    args = ap.parse_args()

    if args.selftest:
        return selftest()
    if not args.csv:
        ap.error("需要 --csv（或用 --selftest）")

    os.makedirs(args.workdir, exist_ok=True)
    cfgs = load_configs(args.csv)

    if args.only:
        keep = {s.strip() for s in args.only.split(",")}
        cfgs = [c for c in cfgs if c["name"].split("_")[-1] in keep]

    # 先做 shape 一致性檢查
    bad = []
    for c in cfgs:
        okshape, h, w = check_shapes(c)
        if not okshape:
            bad.append((c["name"], c["Hout"], c["Wout"], h, w))
    if bad:
        print("!! csv 的 Hout/Wout 與公式不符（generator 或 csv 有問題）:")
        for nm, ch, cw, h, w in bad:
            print(f"   {nm}: csv=({ch},{cw}) 公式=({h},{w})")
        print()

    hdr = (f"{'name':<18} {'M':>6} {'N':>5} {'K':>6} {'tiles':>7} "
           f"{'sw_fp32_ulp':>12} {'sw_fp32_rel':>12} "
           f"{'dut_ulp':>9} {'dut_rel':>11}  result")
    print(hdr)
    print("-" * len(hdr))

    recs = []
    for c in cfgs:
        r = process(c, args.workdir, args.dut_cmd, args.seed, args.emit_only)
        recs.append(r)
        dulp = "-" if r["dut_ulp"] is None else str(r["dut_ulp"])
        drel = "-" if r["dut_rel"] is None else "%.3e" % r["dut_rel"]
        print(f"{r['name']:<18} {r['M']:>6} {r['N_gemm']:>5} {r['K']:>6} "
              f"{r['tiles']:>7} {r['sw_fp32_ulp']:>12} "
              f"{r['sw_fp32_rel']:>12.3e} "
              f"{dulp:>9} {drel:>11}  {r['result']}")

    if args.out:
        cols = ["name", "M", "N_gemm", "K", "tiles",
                "sw_fp32_ulp", "sw_fp32_rel", "dut_ulp", "dut_rel", "result"]
        with open(args.out, "w", newline="") as f:
            wtr = csv.DictWriter(f, fieldnames=cols)
            wtr.writeheader()
            for r in recs:
                wtr.writerow({k: r[k] for k in cols})
        print(f"\n寫入 {args.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
