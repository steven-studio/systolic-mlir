#!/usr/bin/env python3
"""
浮點精度驗證版:用有小數點、會觸發捨入的數值
測試矩陣包含: 分數、負數、大小懸殊的值(觸發對齊/捨入邏輯)
"""
import serial
import struct
import time

PORT = "/dev/ttyUSB1"
BAUD = 115200
N = 4

def matmul_ref(A, B, C):
    result = [[C[i][j] for j in range(N)] for i in range(N)]
    for i in range(N):
        for j in range(N):
            acc = C[i][j]
            for k in range(N):
                acc += A[i][k] * B[k][j]
            result[i][j] = acc
    return result

def flatten(mat):
    return [mat[i][j] for i in range(N) for j in range(N)]

def main():
    # 真正的浮點測資:小數、負數、pi/e 之類無法整除的值
    A = [
        [0.5,   1.25,  -2.75,  3.1],
        [-1.6,  2.333, 0.001, -4.9],
        [3.14159, -0.5, 1.1,   2.2],
        [0.0001, 7.7,  -3.333, 0.618],
    ]
    B = [
        [1.1,  -2.2,  3.3,   -4.4],
        [0.05, 0.15,  -0.25,  0.35],
        [-1.0, 2.71828, -3.5, 0.9],
        [10.1, -0.01,  0.001, -100.5],
    ]
    # C_init 也給非零值,確保 accumulate 路徑也被浮點測試覆蓋
    C_init = [
        [0.1, -0.2, 0.3, -0.4],
        [1.5, -1.5, 2.5, -2.5],
        [0.0, 100.0, -100.0, 0.5],
        [-0.001, 0.002, -0.003, 42.42],
    ]

    C_ref = matmul_ref(A, B, C_init)
    ref_flat = flatten(C_ref)

    print("=== 送出的矩陣(含小數、負數) ===")
    print("A =", A)
    print("B =", B)
    print("C_init =", C_init)
    print()
    print("=== 軟體參考答案 (float64 精度計算,再與 float32 硬體結果比對) ===")
    for row in C_ref:
        print(["%.6f" % v for v in row])
    print()

    tx_bytes = b""
    for v in flatten(A):
        tx_bytes += struct.pack("<f", v)
    for v in flatten(B):
        tx_bytes += struct.pack("<f", v)
    for v in flatten(C_init):
        tx_bytes += struct.pack("<f", v)

    assert len(tx_bytes) == 192, f"應該是 192 bytes, 實際 {len(tx_bytes)}"

    print(f"連線 {PORT} @ {BAUD} baud...")
    ser = serial.Serial(PORT, BAUD, timeout=5)
    time.sleep(0.2)
    ser.reset_input_buffer()

    print(f"傳送 {len(tx_bytes)} bytes...")
    ser.write(tx_bytes)
    ser.flush()

    print("等待接收 64 bytes 結果...")
    rx_bytes = ser.read(64)
    ser.close()

    print(f"實際收到 {len(rx_bytes)} bytes")
    if len(rx_bytes) != 64:
        print("FAIL: 沒有收到完整的 64 bytes")
        return

    result_flat = list(struct.unpack("<16f", rx_bytes))

    print()
    print("=== 硬體回傳結果 vs 軟體參考 ===")
    errors = 0
    # 用 float32 的合理容忍度(不是 0,因為 accumulate 順序不同本來就會有 ULP 級差異)
    TOL = 1e-3
    for i in range(16):
        # 用 float32 重新量化參考值,才是公平比較(硬體全程 float32)
        ref32 = struct.unpack("<f", struct.pack("<f", ref_flat[i]))[0]
        diff = abs(result_flat[i] - ref32)
        rel_diff = diff / max(abs(ref32), 1e-6)
        status = "PASS" if diff < TOL or rel_diff < TOL else "FAIL"
        if status == "FAIL":
            errors += 1
        print(f"[{status}] idx={i:2d}  hw={result_flat[i]:12.6f}  ref32={ref32:12.6f}  diff={diff:.8f}")

    print()
    if errors == 0:
        print(f"✅ 全部通過! 16/16 PASS (真正的浮點精度測試)")
    else:
        print(f"❌ {errors}/16 個結果不符 — 這代表浮點運算路徑可能有問題")

if __name__ == "__main__":
    main()
