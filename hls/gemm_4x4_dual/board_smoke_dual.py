#!/usr/bin/env python3
# 雙陣列 bitstream 板上測試(新協定:[1B device][192B] -> [64B])
# 向量同單陣列版;device 0 和 1 各跑一筆,結果須一致且正確。
import struct, sys, serial

port = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyUSB1"
A = [1.0]*16
B = [ (m % 4 + 1) * 0.5 for m in range(16) ]
C = [ (m // 4) * 0.25   for m in range(16) ]
exp = [ 2.0*(m % 4 + 1) + 0.25*(m // 4) for m in range(16) ]
payload = b"".join(struct.pack("<f", v) for v in A + B + C)

ser = serial.Serial(port, 115200, timeout=5)
total_err = 0
for dev in (0, 1):
    ser.reset_input_buffer()
    ser.write(bytes([dev]) + payload)          # 裝置 byte + 192B
    resp = ser.read(64)
    if len(resp) != 64:
        print(f"FAIL dev{dev}: got {len(resp)} bytes"); total_err += 1; continue
    errs = sum(1 for m in range(16)
               if struct.pack("<f", struct.unpack("<f", resp[m*4:m*4+4])[0])
                  != struct.pack("<f", exp[m]))
    print(f"device {dev}: {'16/16 PASS' if errs == 0 else f'{errs} FAILURES'}")
    total_err += errs
ser.close()
print("=== ALL PASS (board, dual fixed-point arrays, device-select protocol) ==="
      if total_err == 0 else f"=== {total_err} FAILURES ===")
sys.exit(1 if total_err else 0)
