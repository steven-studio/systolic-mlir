import serial
import numpy as np
import time

PORT = "/dev/ttyUSB2"
BAUD = 115200

# ============================================================
# Test matrices
#
# Fold 0:
#   A0 = I
#   B0 = 1..64
#   expected C0 = B0
#
# Fold 1:
#   A1 = 2I
#   B1 = B0
#   expected C1 = 2B0
#
# All values are exactly representable in FP32, making this
# a very clean first FPGA numerical test.
# ============================================================

A0 = np.eye(8, dtype=np.float32)

B0 = np.arange(
    1, 65, dtype=np.float32
).reshape(8, 8)

A1 = (
    2.0 * np.eye(8, dtype=np.float32)
).astype(np.float32)

B1 = B0.copy()


expected0 = (
    A0 @ B0
).astype(np.float32)

expected1 = (
    A1 @ B1
).astype(np.float32)


# ============================================================
# FPGA RX protocol:
#
#   0..255     A0
#   256..511   B0
#   512..767   A1
#   768..1023  B1
#
# little-endian FP32, row-major
# ============================================================

payload = b"".join([
    A0.astype("<f4").tobytes(order="C"),
    B0.astype("<f4").tobytes(order="C"),
    A1.astype("<f4").tobytes(order="C"),
    B1.astype("<f4").tobytes(order="C"),
])

print("TX bytes:", len(payload))

assert len(payload) == 1024


# ============================================================
# FPGA TX protocol:
#
# ctx0:
#   64 PEs * 16 banks * 4 bytes = 4096 bytes
#
# ctx1:
#   another 4096 bytes
#
# Total = 8192 bytes
# ============================================================

EXPECTED_RX = 8192


with serial.Serial(
    PORT,
    BAUD,
    timeout=10
) as ser:

    # Throw away anything left from an earlier run.
    ser.reset_input_buffer()
    ser.reset_output_buffer()

    time.sleep(0.1)

    ser.write(payload)
    ser.flush()

    print("Sent A0+B0+A1+B1, waiting for FPGA...")

    rx = bytearray()

    deadline = time.time() + 15.0

    while (
        len(rx) < EXPECTED_RX
        and time.time() < deadline
    ):
        chunk = ser.read(
            EXPECTED_RX - len(rx)
        )

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


# ============================================================
# Decode raw accumulator banks.
#
# Wire order:
#
#   ctx
#     -> PE 0..63
#       -> bank 0..15
#
# PE index:
#
#   pe = row*8 + col
# ============================================================

raw = np.frombuffer(
    bytes(rx),
    dtype="<f4"
).copy()

banks = raw.reshape(
    2,     # ctx
    64,    # PE
    16     # accumulator bank
)


banks0 = banks[0]
banks1 = banks[1]


# ============================================================
# Final software reduction of the 16 rotating banks.
#
# Force FP32 accumulation.
# ============================================================

C0_flat = np.zeros(
    64,
    dtype=np.float32
)

C1_flat = np.zeros(
    64,
    dtype=np.float32
)


for pe in range(64):

    acc0 = np.float32(0.0)
    acc1 = np.float32(0.0)

    for bank in range(16):

        acc0 = np.float32(
            acc0 + banks0[pe, bank]
        )

        acc1 = np.float32(
            acc1 + banks1[pe, bank]
        )

    C0_flat[pe] = acc0
    C1_flat[pe] = acc1


C0 = C0_flat.reshape(8, 8)
C1 = C1_flat.reshape(8, 8)


# ============================================================
# Print representative raw banks
# ============================================================

print()
print("PE[0][0] ctx0 banks:")
print(banks0[0])

print()
print("PE[0][0] ctx1 banks:")
print(banks1[0])


# ============================================================
# Results
# ============================================================

np.set_printoptions(
    precision=4,
    suppress=True
)

print()
print("========== FOLD 0 ==========")

print()
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

print()
print("FPGA C1:")
print(C1)

print()
print("Expected C1:")
print(expected1)

print()
print("Difference C1:")
print(C1 - expected1)


# ============================================================
# Exact check first because this testcase uses integers that are
# exactly representable as FP32.
# ============================================================

pass0 = np.array_equal(
    C0,
    expected0
)

pass1 = np.array_equal(
    C1,
    expected1
)


print()
print("================================")


if pass0:
    print("PASS: Fold 0 FPGA GEMM is exact.")
else:
    print("FAIL: Fold 0 result mismatch.")


if pass1:
    print("PASS: Fold 1 FPGA GEMM is exact.")
else:
    print("FAIL: Fold 1 result mismatch.")


if pass0 and pass1:

    print(
        "PASS: both fold contexts are numerically correct."
    )

    print(
        "PASS: 8x8 fold-pipelined FPGA numerical validation succeeded."
    )

else:

    max_err0 = np.max(
        np.abs(C0 - expected0)
    )

    max_err1 = np.max(
        np.abs(C1 - expected1)
    )

    print(
        "Max abs error Fold 0:",
        max_err0
    )

    print(
        "Max abs error Fold 1:",
        max_err1
    )

    raise SystemExit(1)
