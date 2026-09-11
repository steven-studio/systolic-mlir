#!/usr/bin/env python3
"""measure_fold.py -- 量一個 fold 在某顆 bitstream 上的週期數,含 K > K_MAX 的多次 invocation。

    python3 tools/measure_fold.py --kmax 256  --K 576  --label layer1.0.conv1
    python3 tools/measure_fold.py --kmax 2048 --K 1728 --label features.6 --port /dev/serial/by-id/usb-FTDI_...
    python3 tools/measure_fold.py --kmax 256  --K 1728 --label features.6 --dry-run   # 只印拆法與預測

做法與 Table 3 的 K=264 / K=512 列完全相同:host 把 K 拆成 ceil(K/K_MAX) 次
invocation(前面每次 k = K_MAX,最後一次 k = K mod K_MAX),每次呼叫
test_uart_kmax.py,讀它印的 `hardware cycles :`,相加。wire format 不在這裡
重寫,理由同 bench_uart.py:兩份實作遲早會漂移。

每次 invocation 的完整 stdout 存到
    results/tbd_633/<label>_kmax<K_MAX>_inv<i>_k<k>.log
總表一列 append 到 results/tbd_633/summary.csv:
    label,K,kmax,n,n_inv,per_inv_cycles,measured,predicted,match,bitexact,bit_file,bit_mtime,bit_sha256_12

predicted = K + n_inv * (2(N-1) + H),H 預設 95(paper-hw-v1 的 H_8)。
match 是「量到的總和是否等於預測」;bitexact 是每次 invocation 都 PASS。
任何一次 invocation 沒 PASS,總和就不寫進 summary(週期數在壞交易上沒有意義),
但 log 仍會留著。

前提(跟 program_kmax.tcl 的提醒一樣):燒錄後先按 BTNC 再送第一筆。
"""

import argparse
import csv
import hashlib
import os
import re
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))          # .../fold_pipelined/tools
ROOT = os.path.normpath(os.path.join(HERE, ".."))          # .../fold_pipelined
TEST = os.path.join(HERE, "test_uart_kmax.py")

CYC_RE = re.compile(r"hardware cycles\s*:\s*(\d+)")


def split_k(K, kmax):
    """K -> [kmax, kmax, ..., K % kmax]  (Table 3 的拆法)"""
    chunks = [kmax] * (K // kmax)
    if K % kmax:
        chunks.append(K % kmax)
    return chunks


def bit_identity(kmax):
    """回報這顆 K_MAX 對應的 .bit 檔身分(路徑、mtime、sha256 前 12 碼)。
    找不到就回 NA -- 板上是哪顆 bit 這支腳本管不到,只能把能查的記下來。"""
    bit = os.path.join(ROOT, "build_kmax", f"k{kmax}", f"systolic_uart_top_k{kmax}.bit")
    if not os.path.exists(bit):
        return ("NA", "NA", "NA")
    h = hashlib.sha256()
    with open(bit, "rb") as f:
        for blk in iter(lambda: f.read(1 << 20), b""):
            h.update(blk)
    mt = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(os.path.getmtime(bit)))
    return (os.path.relpath(bit, ROOT), mt, h.hexdigest()[:12])


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--kmax", type=int, required=True, help="板上這顆 bitstream 的 K_MAX")
    ap.add_argument("--K", type=int, required=True, help="這個 fold 的 reduction depth(workload 的 K)")
    ap.add_argument("--n", type=int, default=8)
    ap.add_argument("--h", type=int, default=95, help="模型常數 H(paper-hw-v1: 95)")
    ap.add_argument("--label", required=True, help="例如 layer1.0.conv1 / features.6 / fc")
    ap.add_argument("--port", default="/dev/ttyUSB2")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--seed", type=int, default=0, help="第 i 次 invocation 用 seed+i")
    ap.add_argument("--timeout", type=float, default=60.0)
    ap.add_argument("--outdir", default=os.path.join(ROOT, "results", "tbd_633"))
    ap.add_argument("--dry-run", action="store_true", help="只印拆法與預測,不碰板子")
    a = ap.parse_args()

    if a.kmax < 16 or a.kmax % 8:
        sys.exit(f"--kmax must be a multiple of 8 and >= 16 (got {a.kmax})")
    if a.K < 1:
        sys.exit("--K must be >= 1")
    if a.K % 8:
        print(f"WARNING: K={a.K} 不是 8 的倍數;硬體的 k window 是 8 深,最後一段會被當 partial window 收費")

    chunks = split_k(a.K, a.kmax)
    geo = 2 * (a.n - 1)
    per_inv_pred = [k + geo + a.h for k in chunks]
    predicted = sum(per_inv_pred)

    print(f"label      : {a.label}")
    print(f"K          : {a.K}   on K_MAX = {a.kmax}  (N = {a.n}, H = {a.h})")
    print(f"invocations: {len(chunks)}  -> k = {chunks}")
    print(f"predicted  : " + " + ".join(f"({k}+{geo}+{a.h})" for k in chunks) + f" = {predicted}")
    print(f"bitstream  : {bit_identity(a.kmax)}")
    if a.dry_run:
        print("(dry-run: 沒有碰板子)")
        return 0

    if not os.path.exists(TEST):
        sys.exit(f"missing {TEST}")
    os.makedirs(a.outdir, exist_ok=True)

    per_inv = []
    all_pass = True
    for i, k in enumerate(chunks):
        log = os.path.join(a.outdir, f"{a.label}_kmax{a.kmax}_inv{i}_k{k}.log")
        cmd = [sys.executable, TEST, "--kmax", str(a.kmax), "--k", str(k), "--n", str(a.n),
               "--port", a.port, "--baud", str(a.baud), "--seed", str(a.seed + i),
               "--timeout", str(a.timeout), "--h", str(a.h)]
        print(f"\n--- invocation {i}: k = {k} ---")
        print("$ " + " ".join(cmd))
        r = subprocess.run(cmd, capture_output=True, text=True)
        with open(log, "w") as f:
            f.write("$ " + " ".join(cmd) + "\n\n")
            f.write(r.stdout)
            if r.stderr:
                f.write("\n--- stderr ---\n" + r.stderr)
        m = CYC_RE.search(r.stdout)
        passed = ("\nPASS:" in r.stdout) and r.returncode == 0
        cyc = int(m.group(1)) if m else None
        print(f"  hardware cycles : {cyc}   (predicted {per_inv_pred[i]})   "
              f"{'PASS' if passed else 'FAIL'}   log: {os.path.relpath(log, ROOT)}")
        if cyc is None or not passed:
            all_pass = False
            print("  這次 invocation 沒有可用的週期數;看上面的 log。總和不會寫進 summary。")
            break
        per_inv.append(cyc)
        time.sleep(0.5)   # 兩次交易之間讓 FSM 回到 idle;比 21 拍寬裕得多

    if not all_pass:
        return 1

    measured = sum(per_inv)
    match = measured == predicted
    print(f"\n{a.label}  K={a.K}  K_MAX={a.kmax}")
    print(f"  measured  : " + " + ".join(map(str, per_inv)) + f" = {measured}")
    print(f"  predicted : {predicted}")
    print(f"  {'MATCH' if match else 'MISMATCH  <- 差 ' + str(measured - predicted)}")

    bit_file, bit_mtime, bit_sha = bit_identity(a.kmax)
    summary = os.path.join(a.outdir, "summary.csv")
    new = not os.path.exists(summary)
    with open(summary, "a", newline="") as f:
        w = csv.writer(f)
        if new:
            w.writerow(["label", "K", "kmax", "n", "n_inv", "per_inv_cycles", "measured",
                        "predicted", "match", "bitexact", "bit_file", "bit_mtime", "bit_sha256_12"])
        w.writerow([a.label, a.K, a.kmax, a.n, len(chunks), "+".join(map(str, per_inv)), measured,
                    predicted, int(match), 1, bit_file, bit_mtime, bit_sha])
    print(f"  appended  : {os.path.relpath(summary, ROOT)}")
    return 0 if match else 2


if __name__ == "__main__":
    sys.exit(main())
