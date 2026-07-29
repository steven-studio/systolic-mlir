#!/usr/bin/env python3
# 單陣列定點 bitstream 板上冒煙測試(舊協定:192B 進 -> 64B 出,無裝置 byte)
# 向量與 tb_matmul_top_dual.v 相同:A=1.0, B[k][j]=(j+1)*0.5, Cinit=0.25*i
# 期望 C[i][j] = 2*(j+1) + 0.25*i,定點下 bit-exact。
import struct, sys, serial

port = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyUSB1"
A = [1.0]*16
B = [ (m % 4 + 1) * 0.5 for m in range(16) ]
C = [ (m // 4) * 0.25   for m in range(16) ]
exp = [ 2.0*(m % 4 + 1) + 0.25*(m // 4) for m in range(16) ]

payload = b"".join(struct.pack("<f", v) for v in A + B + C)
assert len(payload) == 192

ser = serial.Serial(port, 115200, timeout=5)
ser.reset_input_buffer()
ser.write(payload)
resp = ser.read(64)
ser.close()

if len(resp) != 64:
    print(f"FAIL: expected 64 bytes, got {len(resp)} (port {port} 對嗎?試另一個 ttyUSB)")
    sys.exit(1)

errors = 0
for m in range(16):
    got = struct.unpack("<f", resp[m*4:m*4+4])[0]
    ok = struct.pack("<f", got) == struct.pack("<f", exp[m])
    print(f"word {m:2d}: got {got:6.2f}  expected {exp[m]:6.2f}  {'PASS' if ok else 'FAIL'}")
    if not ok: errors += 1
print("=== ALL PASS (board, single fixed-point array) ===" if errors == 0
      else f"=== {errors} FAILURES ===")
sys.exit(1 if errors else 0)
