#!/usr/bin/env python3
"""
透過 UART 驗證 Arty A7 上的 4x4 matmul 加速器
協定: 送 192 bytes (arg0=16 arg1=16 arg2_in=16 個 float, little-endian)
      收 64 bytes (16 個 float 結果)
"""
import serial
import struct
import time

PORT = "/dev/ttyUSB1"
BAUD = 115200
N = 4

def make_matrix(seed_offset):
    return [[float(i + j + seed_offset) for j in range(N)] for i in range(N)]

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
    # 測試資料: A, B 用小的遞增值, C_init = 0
    A = make_matrix(0)          # 0..6
    B = [[float(i - j) for j in range(N)] for i in range(N)]  # -3..3
    C_init = [[0.0]*N for _ in range(N)]

    C_ref = matmul_ref(A, B, C_init)
    ref_flat = flatten(C_ref)

    print("=== 送出的矩陣 ===")
    print("A =", A)
    print("B =", B)
    print("C_init =", C_init)
    print()
    print("=== 軟體參考答案 (C = A@B + C_init) ===")
    for row in C_ref:
        print(["%.3f" % v for v in row])
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
        print("FAIL: 沒有收到完整的 64 bytes,可能是逾時或協定不同步")
        return

    result_flat = list(struct.unpack("<16f", rx_bytes))

    print()
    print("=== 硬體回傳結果 ===")
    errors = 0
    for i in range(16):
        diff = abs(result_flat[i] - ref_flat[i])
        status = "PASS" if diff < 1e-2 else "FAIL"
        if status == "FAIL":
            errors += 1
        print(f"[{status}] idx={i:2d}  hw={result_flat[i]:10.4f}  ref={ref_flat[i]:10.4f}  diff={diff:.6f}")

    print()
    if errors == 0:
        print(f"✅ 全部通過! 16/16 PASS")
    else:
        print(f"❌ {errors}/16 個結果不符")

if __name__ == "__main__":
    main()
