#!/usr/bin/env python3
"""iso-DSP + iso-bandwidth 下的異質 vs 同質。

三個控制變因，缺一個結論就會反過來：
  1. iso-DSP     DSP = 5*R*C（你 15 個點驗過）=> iso-DSP 就是 iso-PE。
                 全部固定 640 DSP = 128 PE，也就是已經跑在板子上的規模。
  2. iso-BW      總 DMA 頻寬固定 32 B/cyc，由 fleet 內各陣列均分。
                 不控這項的話，「切成 8 顆小陣列」會偷偷拿到 4 倍頻寬。
  3. iso-depth   depth 一律 8。depth 不吃 DSP 卻會改變 fill/drain 攤提，
                 不固定就不是在比幾何。
"""
import sys
sys.path.insert(0, "/tmp")
from het_sched import Link, analyse, parse_device, CLK_HZ

TOTAL_BPC = 32.0
DSP_BUDGET = 640


def fleet(*specs):
    return [parse_device(s) for s in specs]


FLEETS = {
    "1x(16x8)":          fleet("a:16x8x8@104"),
    "2x(8x8)":           fleet("a:8x8x8@104", "b:8x8x8@104"),
    "8x(4x4)":           fleet(*[f"a{i}:4x4x8@104" for i in range(8)]),
    "2x(16x4)":          fleet("a:16x4x8@104", "b:16x4x8@104"),
    "2x(32x2)":          fleet("a:32x2x8@104", "b:32x2x8@104"),
    "1x(8x8)+4x(4x4)":   fleet("big:8x8x8@104", *[f"s{i}:4x4x8@104" for i in range(4)]),
    "1x(8x8)+1x(32x2)":  fleet("big:8x8x8@104", "thin:32x2x8@104"),
    "1x(8x8)+2x(4x4)+1x(16x2)":
        fleet("big:8x8x8@104", "s0:4x4x8@104", "s1:4x4x8@104", "thin:16x2x8@104"),
}
for n, f in FLEETS.items():
    d = sum(x.dsp for x in f)
    assert d == DSP_BUDGET, (n, d)


def link_for(f):
    return Link("dma", bytes_per_cycle=TOTAL_BPC / len(f))


SHAPES = {
    "方正   128x64x256":  (128, 64, 256),
    "conv   200x16x72":   (200, 16, 72),
    "極瘦   256x2x512":   (256, 2, 512),
    "Cout=1 256x1x512":   (256, 1, 512),
    "M=1 頭 1x1000x512":  (1, 1000, 512),
}

print("=" * 74)
print(f"實驗一：單層 latency，{DSP_BUDGET} DSP / 128 PE，總頻寬 {TOTAL_BPC:.0f} B/cyc 均分")
print("=" * 74)

for sname, (M, N, K) in SHAPES.items():
    res = []
    for fname, f in FLEETS.items():
        r = analyse(M, N, K, f, link_for(f), False, split="rows")
        res.append((fname, r["makespan"], r["t_link"], r["t_compute"]))
    best = min(x[1] for x in res)
    print(f"\n{sname}")
    for fname, mk, tl, tc in sorted(res, key=lambda x: x[1])[:4]:
        bd = "link" if tl >= tc else "計算"
        print(f"   {fname:<26} {mk:>11,} cyc  {best / mk:5.2f}x  ({bd}-bound)")


print("\n" + "=" * 74)
print("實驗二：串流推論的穩態吞吐（layer pipelining，純計算，不含傳輸）")
print("=" * 74)
print("單張圖的 layer 有先後，但連續的圖可以把不同 layer 疊起來跑。")
print("穩態 II = max_d(分配到 d 的 layer 的總 cycle)。整層路由 = 你的 SelectDevicePass。\n")

LAYERS = [
    ("conv1 3x3   3->32  @32x32", 1024, 32, 27),
    ("conv2 3x3  32->64  @16x16", 256, 64, 288),
    ("conv3 3x3  64->128 @8x8", 64, 128, 576),
    ("conv4 1x1 128->128 @8x8", 64, 128, 128),
    ("pw    1x1 128->1   @8x8", 64, 1, 128),
    ("fc1   128->256", 1, 256, 128),
    ("fc2   256->1000", 1, 1000, 256),
]


def route(f, layers):
    load = {d.name: 0 for d in f}
    where = {}
    for lname, M, N, K in sorted(layers, key=lambda x: -x[1] * x[2] * x[3]):
        best, bfin = None, None
        for d in f:
            fin = load[d.name] + d.gemm_cycles(M, N, K)
            if bfin is None or fin < bfin:
                best, bfin = d, fin
        load[best.name] = bfin
        where[lname] = best.name
    return max(load.values()), where


results = [(n,) + route(f, LAYERS) for n, f in FLEETS.items()]
base = next(ii for n, ii, _ in results if n == "2x(8x8)")
print(f"{'fleet':<26} {'穩態 II':>13} {'img/s':>9} {'vs 2x(8x8)':>11}")
print("-" * 62)
for n, ii, _ in sorted(results, key=lambda x: x[1]):
    print(f"{n:<26} {ii:>11,} c {CLK_HZ / ii:>8,.0f} {base / ii:>10.2f}x")

winner, wii, wwhere = min(results, key=lambda x: x[1])
print(f"\n最佳（{winner}）的層分派：")
for lname, M, N, K in LAYERS:
    print(f"   {lname:<28} -> {wwhere[lname]}")
print("\n對照 2x(8x8)：")
_, _, w2 = next(r for r in results if r[0] == "2x(8x8)")
for lname, M, N, K in LAYERS:
    print(f"   {lname:<28} -> {w2[lname]}")


print("\n" + "=" * 74)
print("實驗三：雙峰工作負載（一半方正、一半極瘦），串流吞吐")
print("=" * 74)
print("MobileNet 式的網路正是這種形狀：pointwise conv 是方正 GEMM，")
print("depthwise conv 每組 Cout=1，是極瘦 GEMM。兩群的總工作量相當。\n")

for n_skew in (1, 2, 3, 4):
    W = [("square", 256, 64, 256)] + [
        (f"skew{i}", 2048, 2, 256) for i in range(n_skew)]
    res = [(n,) + route(f, W) for n, f in FLEETS.items()]
    b = next(ii for n, ii, _ in res if n == "2x(8x8)")
    top = sorted(res, key=lambda x: x[1])[:3]
    print(f"1 方正 + {n_skew} 極瘦：")
    for n, ii, _ in top:
        mark = "  <== 異質" if "+" in n else ""
        print(f"   {n:<26} II {ii:>10,}  vs 2x(8x8) {b / ii:5.2f}x{mark}")
    print()
