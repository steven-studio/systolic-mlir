import serial
import numpy as np
import time

PORT = "/dev/ttyUSB2"
BAUD = 115200

A0 = np.eye(8, dtype=np.float32)

B0 = np.arange(
    1, 65, dtype=np.float32
).reshape(8, 8)

A1 = (
    2.0 * np.eye(8, dtype=np.float32)
).astype(np.float32)

B1 = B0.copy()

expected0 = (A0 @ B0).astype(np.float32)
expected1 = (A1 @ B1).astype(np.float32)

# FPGA RX:
# A0 + B0 + A1 + B1
# 4 matrices * 64 FP32 * 4 bytes = 1024 bytes
payload = b"".join([
    A0.astype("<f4").tobytes(order="C"),
    B0.astype("<f4").tobytes(order="C"),
    A1.astype("<f4").tobytes(order="C"),
    B1.astype("<f4").tobytes(order="C"),
])

assert len(payload) == 1024

# FPGA TX:
# ctx0: 64 FP32 = 256 bytes
# ctx1: 64 FP32 = 256 bytes
EXPECTED_RX = 512

print("TX bytes:", len(payload))

with serial.Serial(
    PORT,
    BAUD,
    timeout=1
) as ser:

    ser.reset_input_buffer()
    ser.reset_output_buffer()

    time.sleep(0.1)

    ser.write(payload)
    ser.flush()

    print("Sent A0+B0+A1+B1, waiting for FPGA...")

    rx = bytearray()

    deadline = time.time() + 15.0

    while len(rx) < EXPECTED_RX and time.time() < deadline:
        chunk = ser.read(EXPECTED_RX - len(rx))

        if chunk:
            rx.extend(chunk)

            print(
                f"\rRX bytes: {len(rx)}/{EXPECTED_RX}",
                end="",
                flush=True
            )

    print()

print("RX bytes:", len(rx))

if len(rx) != EXPECTED_RX:
    print(
        f"FAIL: expected {EXPECTED_RX} bytes, "
        f"received {len(rx)}"
    )
    raise SystemExit(1)

raw = np.frombuffer(
    bytes(rx),
    dtype="<f4"
).copy()

results = raw.reshape(
    2,
    8,
    8
)

C0 = results[0]
C1 = results[1]

np.set_printoptions(
    precision=4,
    suppress=True
)

print()
print("========== FOLD 0 ==========")
print("FPGA C0:")
print(C0)
print()
print("Expected C0:")
print(expected0)
print()
print("Difference C0:")
print(C0 - expected0)

print()
print("========== FOLD 1 ==========")
print("FPGA C1:")
print(C1)
print()
print("Expected C1:")
print(expected1)
print()
print("Difference C1:")
print(C1 - expected1)

ok0 = np.allclose(
    C0,
    expected0,
    rtol=1e-5,
    atol=1e-5
)

ok1 = np.allclose(
    C1,
    expected1,
    rtol=1e-5,
    atol=1e-5
)

print()

if ok0 and ok1:
    print("ALL FPGA FOLD TESTS PASS")
else:
    print("FPGA FOLD TEST FAILED")
    raise SystemExit(1)
