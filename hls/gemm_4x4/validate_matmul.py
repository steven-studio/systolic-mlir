#!/usr/bin/env python3
"""
Host-side validator for the 4x4 systolic matmul FPGA design.

Protocol (reverse-engineered from matmul_top.v):
  - UART: 115200 baud, 8N1 (actual bit rate ~115606 due to 20MHz/173,
    within normal UART tolerance).
  - Host sends 192 bytes = 48 float32 values, little-endian, in this order:
      A[0][0..3], A[1][0..3], A[2][0..3], A[3][0..3]   (16 floats, row-major)
      B[0][0..3], B[1][0..3], B[2][0..3], B[3][0..3]   (16 floats, row-major)
      Cin[0][0..3], Cin[1][0..3], Cin[2][0..3], Cin[3][0..3] (16 floats, row-major)
  - Board replies with 64 bytes = 16 float32 values, little-endian,
    row-major: C[0][0..3], C[1][0..3], C[2][0..3], C[3][0..3]

  Cin is fed into the HLS "Cinout" argument. It's unconfirmed from the RTL
  alone whether the core computes C = A@B (ignoring Cin) or C = A@B + Cin
  (accumulate). Send Cin = zeros for the first test; the script checks
  against both hypotheses and tells you which one matched.

  The FSM in matmul_top.v loops back to S_RX automatically after each
  transaction (no reset needed between runs), but it has no framing/resync
  logic -- if a transaction gets out of sync (wrong byte count sent, noise,
  etc.) you may need to pulse btn_rst on the board before the next attempt.
"""

import sys
import struct
import numpy as np
import serial

PORT = "/dev/ttyUSB1"   # adjust: check `ls /dev/ttyUSB*` or `ls /dev/ttyACM*`
                         # after plugging in the Arty board. Digilent boards
                         # often expose two ports (JTAG + UART); try the
                         # other one if this one doesn't respond.
BAUD = 115200
TIMEOUT_S = 5


def mat_to_bytes(mat: np.ndarray) -> bytes:
    """4x4 float32 matrix -> 64 bytes, row-major, little-endian."""
    assert mat.shape == (4, 4)
    return b"".join(struct.pack("<f", v) for v in mat.astype(np.float32).flatten())


def bytes_to_mat(data: bytes) -> np.ndarray:
    """64 bytes -> 4x4 float32 matrix, row-major, little-endian."""
    assert len(data) == 64
    vals = struct.unpack("<16f", data)
    return np.array(vals, dtype=np.float32).reshape(4, 4)


def run_one(ser: serial.Serial, A: np.ndarray, B: np.ndarray, Cin: np.ndarray) -> np.ndarray:
    payload = mat_to_bytes(A) + mat_to_bytes(B) + mat_to_bytes(Cin)
    assert len(payload) == 192, f"payload is {len(payload)} bytes, expected 192"

    ser.reset_input_buffer()
    ser.write(payload)
    ser.flush()

    resp = ser.read(64)
    if len(resp) != 64:
        raise RuntimeError(
            f"Expected 64 bytes back, got {len(resp)}. "
            "Check wiring/port, or the FSM may be out of sync -- try pulsing btn_rst."
        )
    return bytes_to_mat(resp)


def main():
    port = sys.argv[1] if len(sys.argv) > 1 else PORT
    print(f"Opening {port} @ {BAUD} 8N1 ...")
    ser = serial.Serial(port, BAUD, timeout=TIMEOUT_S)

    rng = np.random.default_rng(0)
    A = rng.uniform(-10, 10, (4, 4)).astype(np.float32)
    B = rng.uniform(-10, 10, (4, 4)).astype(np.float32)
    Cin = np.zeros((4, 4), dtype=np.float32)

    print("A =\n", A)
    print("B =\n", B)
    print("Cin =\n", Cin)

    C = run_one(ser, A, B, Cin)
    print("\nFPGA result C =\n", C)

    expect_no_accum = A @ B
    expect_accum = A @ B + Cin

    err_no_accum = np.max(np.abs(C - expect_no_accum))
    err_accum = np.max(np.abs(C - expect_accum))

    print(f"\nmax|C - A@B|        = {err_no_accum:.6g}")
    print(f"max|C - (A@B + Cin)| = {err_accum:.6g}")

    tol = 1e-3  # loose tolerance for fixed-point float pipeline rounding
    if err_no_accum < tol:
        print("\n=> Matches C = A @ B (Cinout is overwritten, not accumulated).")
    elif err_accum < tol:
        print("\n=> Matches C = A @ B + Cin (Cinout is accumulated).")
    else:
        print("\n=> Neither hypothesis matched within tolerance -- something is off.")
        print("   Re-check byte order / row-vs-column-major assumption, or verify")
        print("   with an XSIM testbench feeding A_i_j/B_i_j directly before blaming UART.")

    ser.close()


if __name__ == "__main__":
    main()
