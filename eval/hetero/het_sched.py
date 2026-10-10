#!/usr/bin/env python3
"""異質陣列排程模擬 -- 一顆 8x8 加一顆 4x4 要怎麼平行算一個 layer。

    python3 het_sched.py --layer linear --M 128 --N 64 --K 256
    python3 het_sched.py --layer conv --conv 2,10,10,8,3,3,16 --link uart
    python3 het_sched.py --sweep                       # 掃全部預設 shape

為什麼要有這支
--------------
runtime 目前的 fpga_matmul_tiled_dim(fd, dim, ndev, ...) 只有單一個 dim，
用 tile_no % ndev 輪流派 -- 那是為 2x(8x8) 同質寫的，餵不了兩顆幾何不同的
陣列。MLIR 那邊 SystolicSelectDevicePass 的粒度又是整個 GEMM。所以「一顆
8x8 + 一顆 4x4」在 repo 裡還不存在，動手改 RTL 與 runtime 之前，先用模型
把「值不值得做」算出來。

模型的三個組成
--------------
1. 計算：cycles = II*(depth+rows+cols-2) + fixedOverhead，per tile。
   這條在 14 個 (R,C,K) 點上 cosim 殘差為 0（HLS：II=1, overhead=6）。
2. 傳輸：沿用現行封包格式，dim x dim 的陣列每個 tile 送 3*dim^2 個 float
   進去、收 dim^2 個 float 出來（8x8: 768/256 B，4x4: 192/64 B）。
3. 連結：這是關鍵的一項，而且有兩種完全不同的結論 --
   * shared  一條 UART 兩顆陣列共用。連結是全域互斥資源。
   * private 每顆陣列自己的 DMA channel，可與計算重疊（double buffering）。

工作切割
--------
輸出矩陣切成 BM x BN 的 output block（預設 8x8，取最大陣列的邊長）。
一個 block 的整條 K chain 綁在同一顆陣列上 -- running sum 以 C_init 傳遞，
中途換陣列要多送一趟，不划算。block 之間完全獨立，這就是可平行的維度。

排程用 list scheduling：block 由大到小，每個丟給「預計最早做完」的陣列。
這跟 SystolicSelectDevicePass 的貪婪規則同一套，只是粒度降到 block。

離散事件模擬而不是閉式解
------------------------
shared link 下傳輸會互相排隊，閉式解會低估。這裡跑一個小的 DES：每顆陣列
輪到自己的 tile 時依序請求 link(in) -> compute -> link(out)，link 以 FIFO
授予。private link 下每顆陣列各有一條，模擬退化成各自的 pipeline。
"""

import argparse
import heapq
import math
from dataclasses import dataclass, field

CLK_HZ = 100e6          # 100 MHz，與 XDC 的 create_clock -period 10.000 一致


def ceil_div(a, b):
    return -(-a // b)


# ---------------------------------------------------------------- 裝置模型

@dataclass
class Device:
    name: str
    rows: int
    cols: int
    depth: int
    ii: int = 1
    overhead: int = 6            # HLS 校正值；fold RTL 8x8 是 104

    @property
    def dsp(self):
        return 5 * self.rows * self.cols          # 全 15 個點上精確成立

    @property
    def pes(self):
        return self.rows * self.cols

    @property
    def tile_cycles(self):
        return self.ii * (self.depth + self.rows + self.cols - 2) + self.overhead

    @property
    def tile_bytes_in(self):
        # 現行協定送方陣：A, B, C_init 各 dim^2 個 float。非方陣時用實際
        # 的 A(rows x depth), B(depth x cols), C(rows x cols)。
        return 4 * (self.rows * self.depth + self.depth * self.cols
                    + self.rows * self.cols)

    @property
    def tile_bytes_out(self):
        return 4 * self.rows * self.cols

    @property
    def macs_per_cycle(self):
        """穩態算力：一個 tile 做 R*C*D 個 MAC，花 tile_cycles 拍。"""
        return self.rows * self.cols * self.depth / self.tile_cycles

    def gemm_cycles(self, m, n, k):
        return (ceil_div(m, self.rows) * ceil_div(n, self.cols)
                * ceil_div(k, self.depth)) * self.tile_cycles

    def tiles_for_block(self, bm, bn, k):
        return (ceil_div(bm, self.rows) * ceil_div(bn, self.cols)
                * ceil_div(k, self.depth))


def parse_device(spec):
    """'8x8x8' 或 'name:8x8x8' 或 'name:8x8x8@104'（覆寫 overhead）。"""
    name, _, geom = spec.rpartition(":")
    ov = 6
    if "@" in geom:
        geom, _, ov_s = geom.partition("@")
        ov = int(ov_s)
    r, c, d = (int(x) for x in geom.lower().split("x"))
    return Device(name or f"acc_{r}x{c}", r, c, d, overhead=ov)


# ---------------------------------------------------------------- 連結模型

@dataclass
class Link:
    """bytes -> cycles。uart 給 baud，dma 給 bytes/cycle。"""
    kind: str
    baud: int = 115200
    bytes_per_cycle: float = 4.0

    def cycles(self, nbytes):
        if self.kind == "uart":
            return math.ceil(nbytes * 10.0 / self.baud * CLK_HZ)
        return math.ceil(nbytes / self.bytes_per_cycle)


# ------------------------------------------------------------ 工作與排程

@dataclass
class Block:
    """一塊 output block，連同它整條 K chain。"""
    i0: int
    j0: int
    bm: int
    bn: int
    k: int

    @property
    def useful_macs(self):
        return self.bm * self.bn * self.k


def make_blocks(M, N, K, bm, bn):
    out = []
    for i in range(0, M, bm):
        for j in range(0, N, bn):
            out.append(Block(i, j, min(bm, M - i), min(bn, N - j), K))
    return out


def assign_greedy(blocks, devices):
    """List scheduling：block 由大到小，丟給預計最早做完的陣列。

    這裡只用計算成本估完成時間（跟 SystolicSelectDevicePass 一樣）；真正的
    makespan 由後面的 DES 決定，兩者不一定一致 —— 這正是重點之一。
    """
    load = {d.name: 0 for d in devices}
    assign = {d.name: [] for d in devices}
    for b in sorted(blocks, key=lambda x: -x.useful_macs):
        best, best_fin = None, None
        for d in devices:
            fin = load[d.name] + d.tiles_for_block(b.bm, b.bn, b.k) * d.tile_cycles
            if best_fin is None or fin < best_fin:
                best, best_fin = d, fin
        assign[best.name].append(b)
        load[best.name] = best_fin
    return assign


def assign_rows(M, N, K, devices):
    """把 C 的列切給各陣列，每顆用自己的幾何鋪 tile。

    block 網格法要求所有陣列共用一種 block 大小，混幾何（8x8 + 32x2）時
    那個網格對誰都不合身。改切 M 維：每顆陣列拿一段連續的列，段長依算力
    分配並對齊到自己的 rows，然後各自鋪自己的 tile。列之間本來就獨立，
    不牽涉 K chain。
    """
    speeds = [d.macs_per_cycle for d in devices]
    tot = sum(speeds)
    cuts, acc = [], 0
    for i, d in enumerate(devices):
        if i == len(devices) - 1:
            m = M - acc
        else:
            m = int(round(M * speeds[i] / tot / d.rows)) * d.rows
            m = max(0, min(m, M - acc))
        cuts.append(m)
        acc += m
    assign = {}
    off = 0
    for d, m in zip(devices, cuts):
        blocks = []
        for i in range(off, off + m, d.rows):
            for j in range(0, N, d.cols):
                blocks.append(Block(i, j, min(d.rows, off + m - i),
                                    min(d.cols, N - j), K))
        assign[d.name] = blocks
        off += m
    return assign


# ------------------------------------------------------------------- DES

def simulate(assign, devices, link, shared_link, onchip_acc=False):
    """回傳 (makespan, per-device stats, bounds)。

    資源三種：link（shared 一條 / private 每顆一條）、每顆陣列的計算單元、
    以及每個 block 的 K chain 相依。

    一個 tile 是兩次獨立的 link 請求，中間夾一次計算：

        link(in)  ->  compute  ->  link(out)

    兩次請求必須分開排隊，否則計算那段時間 link 會被錯誤地佔住 —— 而
    double buffering 的整個好處就在那段時間裡：陣列在算 block A 的 tile 時，
    link 應該正在灌 block B 的 A/B。

      * 同一個 block 的下一個 K tile 要等 C 回來（running sum 相依）。
        開 onchip_acc 時累加留在片上，中途不送 C_init 也不回送 C，這條
        相依就只剩「計算單元要空出來」。
      * 不同 block 之間完全獨立 —— 這就是可平行的維度。

    緩衝深度不設上限（等同無限 double buffering），所以這是樂觀上界。
    實際只有兩個漸近線會綁住 makespan，函式一併回傳讓呼叫端對照：
        T_link    = 最忙的那條 link 的總傳輸時間
        T_compute = 最忙的那顆陣列的總計算時間
    makespan 應該落在 max(T_link, T_compute) 附近，差額是 fill/drain。
    """
    byname = {d.name: d for d in devices}

    def slot(name):
        return "__shared__" if shared_link else name

    link_free = {"__shared__": 0} if shared_link else {n: 0 for n in assign}
    link_used = {k: 0 for k in link_free}
    cmp_free = {n: 0 for n in assign}
    busy = {n: 0 for n in assign}
    xfer = {n: 0 for n in assign}
    tiles_tot = {n: 0 for n in assign}

    # link 請求佇列：(ready, seq, name, kind, bseq, left)
    # kind 'I' = 送 A/B/C_init 進去，'O' = 把 C 收回來
    q = []
    seq = 0
    for name, blocks in assign.items():
        d = byname[name]
        for b in blocks:
            n = d.tiles_for_block(b.bm, b.bn, b.k)
            tiles_tot[name] += n
            if n:
                heapq.heappush(q, (0, seq, name, "I", seq, n))
                seq += 1
    nseq = seq

    makespan = 0
    while q:
        ready, _, name, kind, bseq, left = heapq.heappop(q)
        d = byname[name]
        s = slot(name)
        mid = onchip_acc and left > 1        # K chain 中段，不必來回搬 C

        if kind == "I":
            nbytes = (4 * (d.rows * d.depth + d.depth * d.cols) if mid
                      else d.tile_bytes_in)
            t = link.cycles(nbytes)
            start = max(ready, link_free[s])
            end = start + t
            link_free[s] = end
            link_used[s] += t
            xfer[name] += t

            start_cmp = max(end, cmp_free[name])
            end_cmp = start_cmp + d.tile_cycles
            cmp_free[name] = end_cmp
            busy[name] += d.tile_cycles
            makespan = max(makespan, end_cmp)

            if mid:
                # C 留在片上：下一個 K tile 只等計算單元
                heapq.heappush(q, (end_cmp, nseq, name, "I", bseq, left - 1))
                nseq += 1
            else:
                heapq.heappush(q, (end_cmp, nseq, name, "O", bseq, left))
                nseq += 1
        else:
            t = link.cycles(d.tile_bytes_out)
            start = max(ready, link_free[s])
            end = start + t
            link_free[s] = end
            link_used[s] += t
            xfer[name] += t
            makespan = max(makespan, end)
            if left > 1:
                heapq.heappush(q, (end, nseq, name, "I", bseq, left - 1))
                nseq += 1

    stats = {n: dict(tiles=tiles_tot[n], busy=busy[n], xfer=xfer[n])
             for n in assign}
    bounds = dict(t_link=max(link_used.values()) if link_used else 0,
                  t_compute=max(busy.values()) if busy else 0)
    return makespan, stats, bounds


# ------------------------------------------------------------------ 報表

def analyse(M, N, K, devices, link, shared_link, bm=None, bn=None, label="", onchip_acc=False, split="blocks"):
    bmax = max(d.rows for d in devices)
    cmax = max(d.cols for d in devices)
    bm = bm or bmax
    bn = bn or cmax

    if split == "rows":
        assign = assign_rows(M, N, K, devices)
        blocks = [b for bl in assign.values() for b in bl]
    else:
        blocks = make_blocks(M, N, K, bm, bn)
        assign = assign_greedy(blocks, devices)
    makespan, stats, bounds = simulate(assign, devices, link, shared_link, onchip_acc)

    useful = M * N * K
    padded = 0
    for name, bl in assign.items():
        d = next(x for x in devices if x.name == name)
        for b in bl:
            padded += (d.tiles_for_block(b.bm, b.bn, b.k)
                       * d.rows * d.cols * d.depth)
    total_pes = sum(d.pes for d in devices)

    return dict(
        label=label, M=M, N=N, K=K,
        devices=[d.name for d in devices],
        dsp=sum(d.dsp for d in devices),
        pes=total_pes,
        blocks=len(blocks),
        assign={n: len(b) for n, b in assign.items()},
        tiles={n: s["tiles"] for n, s in stats.items()},
        busy={n: s["busy"] for n, s in stats.items()},
        xfer={n: s["xfer"] for n, s in stats.items()},
        makespan=makespan,
        t_link=bounds['t_link'], t_compute=bounds['t_compute'],
        seconds=makespan / CLK_HZ,
        util=useful / (total_pes * makespan) if makespan else 0.0,
        frag=padded / useful if useful else 0.0,
    )


def conv_to_gemm(nb, h, w, cin, kh, kw, cout, stride=(1, 1), pad=1):
    """im2col：M = N*Hout*Wout, K = Kh*Kw*Cin, N = Cout。"""
    ho = (h + 2 * pad - kh) // stride[0] + 1
    wo = (w + 2 * pad - kw) // stride[1] + 1
    return nb * ho * wo, cout, kh * kw * cin


CONFIGS = {
    # runtime/harness/sweep_out 的真實 shape
    "conv_sweep_004": dict(conv=(2, 10, 10, 8, 3, 3, 16), stride=(1, 1)),
    "conv_sweep_029": dict(conv=(1, 8, 8, 8, 5, 5, 16), stride=(1, 2)),
    "conv_sweep_048": dict(conv=(2, 10, 10, 8, 3, 5, 16), stride=(1, 1)),
    # 典型 linear layer
    "linear_128x64x256": dict(gemm=(128, 64, 256)),
    "linear_1x1000x512": dict(gemm=(1, 1000, 512)),      # 分類頭，M=1
    "cout1_256x1x512":   dict(gemm=(256, 1, 512)),       # Cout=1，N=1
    "linear_18x16x200":  dict(gemm=(18, 16, 200)),       # 029 的 GEMM
}

FLEETS = {
    "1x(8x8)":    ["acc8a:8x8x8"],
    "2x(8x8)":    ["acc8a:8x8x8", "acc8b:8x8x8"],
    "8x8 + 4x4":  ["acc8a:8x8x8", "acc4:4x4x4"],
    "1x(4x4)":    ["acc4:4x4x4"],
}


def fmt_time(sec):
    if sec >= 1:
        return f"{sec:8.3f} s "
    if sec >= 1e-3:
        return f"{sec * 1e3:8.3f} ms"
    return f"{sec * 1e6:8.3f} us"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--M", type=int)
    ap.add_argument("--N", type=int)
    ap.add_argument("--K", type=int)
    ap.add_argument("--conv", help="Nb,H,W,Cin,Kh,Kw,Cout")
    ap.add_argument("--stride", default="1,1")
    ap.add_argument("--pad", type=int, default=1)
    ap.add_argument("--config", choices=sorted(CONFIGS))
    ap.add_argument("--dev", action="append", default=[],
                    help="name:RxCxD[@overhead]，可重複")
    ap.add_argument("--link", choices=["uart", "dma"], default="uart")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--bpc", type=float, default=4.0, help="DMA bytes/cycle")
    ap.add_argument("--shared-link", action="store_true",
                    help="兩顆陣列共用一條連結（UART 現況）")
    ap.add_argument("--onchip-acc", action="store_true",
                    help="K chain 累加留在片上，中途不回送 C")
    ap.add_argument("--sweep", action="store_true")
    ap.add_argument("--bw-sweep", action="store_true",
                    help="掃 DMA 頻寬，找出計算變成瓶頸的交叉點")
    a = ap.parse_args()

    link = Link(a.link, baud=a.baud, bytes_per_cycle=a.bpc)

    if a.sweep:
        run_sweep(a)
        return 0
    if a.bw_sweep:
        run_bw_sweep(a)
        return 0

    if a.config:
        cfg = CONFIGS[a.config]
        if "conv" in cfg:
            M, N, K = conv_to_gemm(*cfg["conv"], stride=cfg.get("stride", (1, 1)),
                                   pad=a.pad)
        else:
            M, N, K = cfg["gemm"]
    elif a.conv:
        nb, h, w, cin, kh, kw, cout = (int(x) for x in a.conv.split(","))
        st = tuple(int(x) for x in a.stride.split(","))
        M, N, K = conv_to_gemm(nb, h, w, cin, kh, kw, cout, st, a.pad)
    else:
        M, N, K = a.M, a.N, a.K
        if None in (M, N, K):
            ap.error("需要 --M/--N/--K，或 --conv，或 --config")

    devs = [parse_device(s) for s in (a.dev or ["acc8a:8x8x8", "acc4:4x4x4"])]
    r = analyse(M, N, K, devs, link, a.shared_link, onchip_acc=a.onchip_acc)
    print_one(r, link, a.shared_link)
    return 0


def print_one(r, link, shared):
    print(f"GEMM  M={r['M']}  N={r['N']}  K={r['K']}   "
          f"useful MACs = {r['M'] * r['N'] * r['K']:,}")
    print(f"連結  {link.kind}"
          + (f" @ {link.baud}" if link.kind == "uart" else f" @ {link.bytes_per_cycle} B/cyc")
          + ("（共用一條）" if shared else "（每顆各一條）"))
    print(f"陣列  {', '.join(r['devices'])}   DSP {r['dsp']}   PE {r['pes']}")
    print()
    for n in r["assign"]:
        print(f"  {n:8s}  blocks {r['assign'][n]:5d}   tiles {r['tiles'][n]:6d}"
              f"   busy {r['busy'][n]:9,d}   xfer {r['xfer'][n]:11,d} cyc")
    print()
    print(f"  T_link     {r['t_link']:12,d} cyc   {fmt_time(r['t_link'] / CLK_HZ)}   <- 最忙的 link")
    print(f"  T_compute  {r['t_compute']:12,d} cyc   {fmt_time(r['t_compute'] / CLK_HZ)}   <- 最忙的陣列")
    print(f"  makespan   {r['makespan']:12,d} cyc   {fmt_time(r['seconds'])}")
    print(f"  PE util    {r['util'] * 100:12.4f} %")
    print(f"  對齊膨脹   {r['frag']:12.2f} x   (padded MACs / useful MACs)")


def run_sweep(a):
    rows = []
    for cfg_name in sorted(CONFIGS):
        cfg = CONFIGS[cfg_name]
        if "conv" in cfg:
            M, N, K = conv_to_gemm(*cfg["conv"], stride=cfg.get("stride", (1, 1)),
                                   pad=a.pad)
        else:
            M, N, K = cfg["gemm"]
        for link_kind, shared in (("uart", True), ("dma", False)):
            link = Link(link_kind, baud=a.baud, bytes_per_cycle=a.bpc)
            base = None
            for fleet, specs in FLEETS.items():
                devs = [parse_device(s) for s in specs]
                r = analyse(M, N, K, devs, link, shared, onchip_acc=a.onchip_acc)
                if fleet == "1x(8x8)":
                    base = r["makespan"]
                rows.append((cfg_name, f"{M}x{N}x{K}", link_kind, fleet,
                             r["dsp"], r["makespan"], r["seconds"],
                             base / r["makespan"] if base else 1.0,
                             r["util"] * 100))

    hdr = f"{'config':<18} {'GEMM':<14} {'link':<5} {'fleet':<11} {'DSP':>4} {'makespan':>13} {'time':>11} {'vs 1x8x8':>9} {'util%':>8}"
    print(hdr)
    print("-" * len(hdr))
    last = None
    for c, g, lk, fl, dsp, mk, sec, sp, ut in rows:
        if last and last != (c, lk):
            print()
        last = (c, lk)
        print(f"{c:<18} {g:<14} {lk:<5} {fl:<11} {dsp:>4} {mk:>13,} "
              f"{fmt_time(sec):>11} {sp:>8.2f}x {ut:>7.4f}")


def run_bw_sweep(a):
    """掃 DMA 頻寬：makespan 何時從 link-bound 轉成 compute-bound。

    交叉點就是「這個陣列要吃多少頻寬才餵得飽」，也是決定 DMA 該接
    BRAM / DDR / 幾條 channel 的那個數字。
    """
    M, N, K = 128, 64, 256
    print(f"GEMM M={M} N={N} K={K}   每顆陣列一條 private DMA\n")
    for onchip in (False, True):
        tag = "on-chip 累加（K chain 不回送 C）" if onchip else "現行協定（每個 tile 來回送 C）"
        print(f"--- {tag}")
        hdr = f"{'B/cyc':>7} {'GB/s':>7} " + " ".join(f"{f:>14}" for f in FLEETS)
        print(hdr)
        for bpc in (1, 2, 4, 8, 16, 32, 64, 128):
            link = Link("dma", bytes_per_cycle=bpc)
            cells = []
            for fleet, specs in FLEETS.items():
                devs = [parse_device(x) for x in specs]
                r = analyse(M, N, K, devs, link, False, onchip_acc=onchip)
                bound = "L" if r["t_link"] >= r["t_compute"] else "C"
                cells.append(f"{fmt_time(r['seconds']).strip():>12}{bound:>2}")
            print(f"{bpc:>7} {bpc * CLK_HZ / 1e9:>7.2f} " + " ".join(f"{c:>14}" for c in cells))
        print("   L = link-bound（傳輸綁住）   C = compute-bound（陣列綁住）\n")


if __name__ == "__main__":
    raise SystemExit(main())
