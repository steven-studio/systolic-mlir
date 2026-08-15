import serial
import struct
import time
import numpy as np

PORT = "/dev/ttyUSB2"
BAUD = 115200

# A = identity
A = np.eye(8, dtype=np.float32)

# B = 1..64
B = np.arange(1, 65, dtype=np.float32).reshape(8, 8)

expected = A @ B

def matrix_to_bytes(M):
    data = bytearray()

    for r in range(8):
        for c in range(8):
            data += struct.pack("<f", float(M[r, c]))

    return data

payload = matrix_to_bytes(A) + matrix_to_bytes(B)

print("TX bytes:", len(payload))

ser = serial.Serial(
    PORT,
    BAUD,
    timeout=5
)

time.sleep(0.2)

# 清掉舊資料
ser.reset_input_buffer()
ser.reset_output_buffer()

# 送 512 bytes
ser.write(payload)
ser.flush()

print("Sent A+B, waiting for FPGA...")

# FPGA 應回 64 FP32 = 256 bytes
rx = bytearray()

deadline = time.time() + 10

while len(rx) < 256 and time.time() < deadline:
    chunk = ser.read(256 - len(rx))

    if chunk:
        rx.extend(chunk)

ser.close()

print("RX bytes:", len(rx))

if len(rx) != 256:
    print("FAIL: expected 256 bytes")
    raise SystemExit(1)

values = struct.unpack("<64f", rx)

C = np.array(values, dtype=np.float32).reshape(8, 8)

print("\nFPGA C:")
print(C)

print("\nExpected:")
print(expected)

print("\nDifference:")
print(C - expected)

if np.allclose(C, expected, rtol=1e-5, atol=1e-5):
    print("\nPASS: FPGA 8x8 GEMM is correct.")
else:
    print("\nFAIL: FPGA result does not match CPU.")
