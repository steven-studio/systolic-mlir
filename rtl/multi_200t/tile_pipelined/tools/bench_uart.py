#!/usr/bin/env python3
"""牆鐘時間量測 -- 傳輸項的實測,與 cycle 模型互補。

    python3 bench_uart.py --kmax 16 --reps 20
    python3 bench_uart.py --kmax 16 --reps 20 --baud 2000000
    python3 bench_uart.py --kmax 16 --dry-run          # 只印預測,不碰板子

為什麼要有這個
--------------
test_uart_kmax.py 量的是 on-chip cycle counter,那是「計算」這一項,
與 baud 無關。這支量的是「傳輸」那一項:

    T_total = T_compute(k, N) + T_transport(bytes, baud)
              └ k+2(N-1)+105 ┘  └ bytes*10/baud    ┘

前者已經在兩個幾何上驗到殘差 0。後者目前只有推導,沒有量測 --
改 baud 之前先把 115200 的基準量下來,改完再量一次,兩組的比值
應該等於 baud 的比值(17.36)。對得上,傳輸項就從推導變成實測;
對不上,差額就是 host 端的固定成本,那本身也是要報告的東西。

方法學
------
* 同一顆 bitstream、同一次開埠、同一組刺激,只有 baud 不同。
* 每次 invocation 都驗 bit-exact。壞掉的交易的時間沒有意義。
* on-chip cycle 數當作對照變數:它不該隨 baud 改變,變了就是
  有別的東西被動到了。
* 前幾次當暖機丟掉 -- 第一次交易含 pyserial 的埠設定與核心緩衝
  配置,不代表穩態。
* 報 median 而不是 mean。OS 排程造成的尖峰是單邊的,平均會被拉走。

wire format 一律從 test_uart_kmax 匯入,不在這裡重寫 --
兩份實作遲早會漂移,而漂移的那天你會以為是硬體壞了。
"""

import argparse
import statistics
import sys
import time

import numpy as np

try:
    import serial
except ImportError:
    sys.exit("pyserial not installed:  pip install pyserial")

try:
    from test_uart_kmax import (
        FRAME_START, FRAME_END, HDR_BYTES, MARK_BYTES,
        build_request, expected_result,
    )
except ImportError:
    sys.exit("找不到 test_uart_kmax.py -- 請在同一個目錄下執行")


# 週期模型的幾何無關常數。105 是 ctx 移除前的值;重寫之後要重新擬合。
# 這支只拿它做「計算 vs 傳輸」的量級對照,差幾拍不影響那個對照的結論。
H_CONST = 105


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--kmax", type=int, default=16)
    ap.add_argument("--k", type=int, default=None)
    ap.add_argument("--n", type=int, default=8)
    ap.add_argument("--port", default="/dev/ttyUSB2")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--reps", type=int, default=20)
    ap.add_argument("--warmup", type=int, default=3)
    ap.add_argument("--seed", type=int, default=0)
    ap.add_argument("--timeout", type=float, default=120.0)
    ap.add_argument("--csv", default="bench_results.csv")
    ap.add_argument("--dry-run", action="store_true",
                    help="只印預測值,不開序列埠")
    a = ap.parse_args()

    kmax, n = a.kmax, a.n
    k = a.k if a.k is not None else kmax

    rx_bytes = kmax * 8 * n
    tx_bytes = 4 * n * n          # ctx 移除後砍半(舊:8*n*n)
    req_bytes = MARK_BYTES + HDR_BYTES + rx_bytes + MARK_BYTES

    # 10 bits per byte on the wire: 1 start + 8 data + 1 stop.
    bit_time = 10.0 / a.baud
    pred_w = req_bytes * bit_time * 1000.0          # ms
    pred_r = tx_bytes * bit_time * 1000.0
    pred_c = (k + 2 * (n - 1) + H_CONST) / 100e6 * 1000.0

    print(f"K_MAX {kmax}   N {n}   k_dim {k}   baud {a.baud}")
    print(f"request  {req_bytes} bytes   response {tx_bytes} bytes (+4 cycle counter)")
    print()
    print(f"predicted  write   {pred_w:9.3f} ms")
    print(f"predicted  read    {pred_r:9.3f} ms")
    print(f"predicted  compute {pred_c*1000:9.3f} us   "
          f"({(pred_w+pred_r)/pred_c:,.0f}x smaller than transport)")
    print(f"predicted  total   {pred_w+pred_r:9.3f} ms")
    print()

    if a.dry_run:
        return 0

    rng = np.random.default_rng(a.seed)
    A = rng.integers(-4, 5, size=(n, kmax)).astype(np.float32)
    B = rng.integers(-4, 5, size=(kmax, n)).astype(np.float32)
    if k < kmax:
        A[:, k:] = 1024.0
        B[k:, :] = 1024.0

    request = FRAME_START + build_request(k, A, B, kmax) + FRAME_END
    assert len(request) == req_bytes
    exp_c = expected_result(A, B, k)

    t_w, t_r, t_t, cycles = [], [], [], []
    total = a.warmup + a.reps

    with serial.Serial(a.port, a.baud, timeout=a.timeout) as ser:
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        time.sleep(0.2)

        for i in range(total):
            t0 = time.perf_counter()
            ser.write(request)
            ser.flush()                 # tcdrain: 等到真的送完才回來
            t1 = time.perf_counter()

            rx = ser.read(tx_bytes)
            t2 = time.perf_counter()

            cyc_raw = ser.read(4)       # CYCLE_COUNTER=1 的尾巴

            if len(rx) != tx_bytes:
                print(f"\nrep {i}: 只收到 {len(rx)}/{tx_bytes} bytes -- 中止。")
                print("時間數字在交易不完整時沒有意義。先確認板子狀態(看 LED)。")
                return 1

            raw = np.frombuffer(rx, dtype="<f4")
            if not np.array_equal(raw.reshape(n, n), exp_c):
                print(f"\nrep {i}: 結果不是 bit-exact -- 中止。")
                return 1

            if i >= a.warmup:
                t_w.append((t1 - t0) * 1000.0)
                t_r.append((t2 - t1) * 1000.0)
                t_t.append((t2 - t0) * 1000.0)
                if len(cyc_raw) == 4:
                    cycles.append(int.from_bytes(cyc_raw, "little"))

            print(f"\r{i+1}/{total}", end="", flush=True)

    print("\n")

    def stat(name, xs, pred):
        med = statistics.median(xs)
        print(f"{name:<9} median {med:9.3f} ms   min {min(xs):9.3f}   "
              f"max {max(xs):9.3f}   predicted {pred:9.3f}   "
              f"residual {med - pred:+8.3f} ms ({(med/pred - 1)*100:+.1f}%)")
        return med

    med_w = stat("write", t_w, pred_w)
    med_r = stat("read", t_r, pred_r)
    med_t = stat("total", t_t, pred_w + pred_r)

    print()
    if cycles:
        uniq = sorted(set(cycles))
        exp_cyc = k + 2 * (n - 1) + H_CONST
        print(f"on-chip cycles : {uniq}   (模型 {exp_cyc})"
              f"{'  一致' if uniq == [exp_cyc] else '  << 不一致,對照變數變了'}")
    print(f"effective rate : {(req_bytes + tx_bytes) / (med_t/1000) / 1024:,.1f} KiB/s")

    row = (f"{kmax},{n},{k},{a.baud},{a.reps},"
           f"{med_w:.4f},{med_r:.4f},{med_t:.4f},"
           f"{pred_w:.4f},{pred_r:.4f},{pred_w+pred_r:.4f}\n")
    try:
        with open(a.csv, "a") as f:
            if f.tell() == 0:
                f.write("kmax,n,k,baud,reps,"
                        "med_write_ms,med_read_ms,med_total_ms,"
                        "pred_write_ms,pred_read_ms,pred_total_ms\n")
            f.write(row)
        print(f"appended to {a.csv}")
    except OSError as e:
        print(f"(無法寫入 {a.csv}: {e})")

    return 0


if __name__ == "__main__":
    sys.exit(main())
