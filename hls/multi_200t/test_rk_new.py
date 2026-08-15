#!/usr/bin/env python3
"""runtime-K 的端到端 bit-exact 測試。

    python3 test_rk_new.py /dev/ttyUSB2 [baud]

協定(與 matmul_top_rk.v 一致):
    [1B dev][4B K, LE][A 8*K*4B][B 8*K*4B][C_init 256B]  ->  [C 256B]

  A 的順序:bank 0 的 word 0..K-1、bank 1 的 0..K-1、...、bank 7
           bank i 就是 A 的第 i 列,所以是 A[i][k] 逐列展開
  B 的順序:同樣八個 bank,bank j 是 B 的第 j 行,即 B[k][j]
           注意 B 是 **轉置後** 送的 —— 這是 partition dim=2 的直接後果

為什麼要測 K=63 而不是只測 K=64
  fill_word 是 6 bit,K_MAX=64。如果計數器寫錯成「溢位才回捲」而不是
  「數到 K-1 回捲」,K=64 時兩者行為完全相同,測試會過。K=63 才會把
  這個 bug 逼出來。同理 K=1 檢查 k_reg-1=0 的邊界。
  只測 2 的冪次是這類 bug 最典型的漏網方式。

bit-exact 的前提
  浮點加法不可結合,所以參考解必須用**相同的 k 順序**累加,而且每一步
  都留在 float32。PE(i,j) 在第 i+j+k 拍做第 k 次 MAC,k 遞增,所以參考
  解也從 k=0 累加到 K-1。任何 numpy 的向量化 sum 都可能重排順序或用
  float64 中間值,結果會差幾個 ulp —— 那不是硬體錯。
"""

import sys
import struct
import numpy as np
import serial

R = 8
C = 8
K_MAX = 64


def make_payload(dev, K, A, B, Cin):
    """組出一次交易的位元組。順序必須與 matmul_top_rk.v 的 FSM 完全一致。"""
    buf = bytearray()
    buf.append(dev & 0xFF)
    buf += struct.pack('<I', K)

    # A:bank i = 第 i 列,word k = A[i][k]
    for i in range(R):
        for k in range(K):
            buf += struct.pack('<f', float(A[i][k]))

    # B:bank j = 第 j 行,word k = B[k][j] —— 轉置
    for j in range(C):
        for k in range(K):
            buf += struct.pack('<f', float(B[k][j]))

    # C_init:row-major
    for i in range(R):
        for j in range(C):
            buf += struct.pack('<f', float(Cin[i][j]))

    return bytes(buf)


def reference(K, A, B, Cin):
    """逐 k 累加的 float32 參考解。不要改成 np.dot 或 sum()。"""
    out = np.zeros((R, C), dtype=np.float32)
    for i in range(R):
        for j in range(C):
            acc = np.float32(Cin[i][j])
            for k in range(K):
                acc = np.float32(acc + np.float32(A[i][k] * B[k][j]))
            out[i][j] = acc
    return out


def bit_equal(x, y):
    """比對位元樣式而不是數值 —— NaN 也要一致,而 x==y 對 NaN 是 False。"""
    return x.astype(np.float32).tobytes() == y.astype(np.float32).tobytes()


def run_one(ser, dev, K, rng, label):
    A   = rng.standard_normal((R, K)).astype(np.float32)
    B   = rng.standard_normal((K, C)).astype(np.float32)
    Cin = rng.standard_normal((R, C)).astype(np.float32)

    payload = make_payload(dev, K, A, B, Cin)
    ser.reset_input_buffer()
    ser.write(payload)
    ser.flush()

    got_raw = ser.read(R * C * 4)
    if len(got_raw) != R * C * 4:
        print(f"  dev={dev} K={K:<3} {label:<12} 逾時:"
              f"只收到 {len(got_raw)}/{R*C*4} bytes")
        return False

    got = np.frombuffer(got_raw, dtype='<f4').reshape(R, C)
    exp = reference(K, A, B, Cin)

    if bit_equal(got, exp):
        print(f"  dev={dev} K={K:<3} {label:<12} BIT-EXACT"
              f"  ({len(payload)}B 出 / {len(got_raw)}B 回)")
        return True

    # 不合就把第一個差異印出來 —— 全域錯和單點錯的成因完全不同
    bad = np.argwhere(got != exp)
    i, j = bad[0]
    print(f"  dev={dev} K={K:<3} {label:<12} 不符,共 {len(bad)}/{R*C} 個元素")
    print(f"      第一個差異 C[{i}][{j}]: 收到 {got[i][j]!r}  期望 {exp[i][j]!r}")
    if len(bad) == R * C:
        print("      全部都錯 -> 多半是 payload 排列或 K 沒吃到")
    else:
        print("      只有部分錯 -> 多半是某個 bank 的位址或讀取延遲")
    return False


def main():
    port = sys.argv[1] if len(sys.argv) > 1 else '/dev/ttyUSB2'
    baud = int(sys.argv[2]) if len(sys.argv) > 2 else 2000000

    # 固定 seed:失敗時可以重現同一組資料,不用猜是哪次的亂數
    rng = np.random.default_rng(20260806)

    ser = serial.Serial(port, baud, timeout=15)
    print(f"port={port} baud={baud}\n")

    cases = [
        (0, 8,  "回歸基準"),      # 與固定 K 版同樣的工作量
        (1, 8,  "回歸基準"),
        (0, 1,  "邊界最小"),      # k_reg-1 = 0
        (0, 63, "非 2 冪次"),     # 只有這個抓得到回捲條件寫錯
        (0, 64, "K_MAX"),
        (1, 64, "K_MAX"),
    ]

    ok = all(run_one(ser, dev, K, rng, label) for dev, K, label in cases)

    ser.close()
    print("\n全部通過" if ok else "\n有失敗")
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
