#!/usr/bin/env python3
"""Host test for the K_MAX-parameterised 8x8 FP32 fold array.

    python3 test_uart_kmax.py --kmax 64
    python3 test_uart_kmax.py --kmax 64 --port /dev/ttyUSB2

Generalises test_uart_fold8x8.py, which hardcodes the 1024-byte K_MAX=16
framing.

WIRE FORMAT
-----------
RX (host -> FPGA), K_MAX * 64 bytes, A and B interleaved one 8-deep k
window at a time:

    A[:, 0:8]  B[0:8, :]  A[:, 8:16]  B[8:16, :]  ...

Each matrix is 8x8 float32, little-endian, row-major:

    A window w holds A[row][k] for k = 8w .. 8w+7   -> indexed [row][k-8w]
    B window w holds B[k][col] for k = 8w .. 8w+7   -> indexed [k-8w][col]

TX (FPGA -> host), always 512 bytes regardless of K_MAX:

    C_ctx0 (8x8 float32) then C_ctx1 (8x8 float32)

There are only two accumulator contexts in hardware, so even-numbered
windows accumulate into ctx0 and odd-numbered into ctx1. The final result
is the host's job:

    C = C_ctx0 + C_ctx1

That cross-context add is deliberately not in hardware, and it does not
change with K_MAX.

NUMERICS
--------
The default stimulus uses small integers so every product and partial sum
is exactly representable in float32. The hardware's summation order (16
rotating accumulator banks, then a linear reduction) differs from numpy's,
so a floating-point stimulus would legitimately differ in the last ulp;
integers remove that ambiguity and let the test demand bit-exactness.
"""

import argparse
import struct
import sys
import time

import numpy as np

try:
    import serial
except ImportError:
    sys.exit("pyserial not installed:  pip install pyserial")


def build_payload(A, B, kmax):
    """A is 8 x kmax, B is kmax x 8. Returns the RX byte string."""
    out = bytearray()
    for w in range(kmax // 8):
        ks = slice(w * 8, w * 8 + 8)
        # A window: [row][k-within-window], row-major
        out += np.ascontiguousarray(A[:, ks], dtype="<f4").tobytes()
        # B window: [k-within-window][col], row-major
        out += np.ascontiguousarray(B[ks, :], dtype="<f4").tobytes()
    return bytes(out)


def expected_contexts(A, B, kmax):
    """What each hardware accumulator context should hold."""
    c0 = np.zeros((8, 8), dtype=np.float32)
    c1 = np.zeros((8, 8), dtype=np.float32)
    for w in range(kmax // 8):
        ks = slice(w * 8, w * 8 + 8)
        part = (A[:, ks] @ B[ks, :]).astype(np.float32)
        if w % 2 == 0:
            c0 += part
        else:
            c1 += part
    return c0, c1


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--kmax", type=int, default=64,
                    help="must match the synthesised K_MAX")
    ap.add_argument("--port", default="/dev/ttyUSB2")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--timeout", type=float, default=30.0)
    ap.add_argument("--seed", type=int, default=0)
    ap.add_argument("--ones", action="store_true",
                    help="use all-ones instead of random integers")
    a = ap.parse_args()

    if a.kmax < 16 or a.kmax % 8:
        sys.exit(f"K_MAX must be a multiple of 8 and >= 16 (got {a.kmax})")

    rx_bytes = a.kmax * 64
    tx_bytes = 512

    if a.ones:
        A = np.ones((8, a.kmax), dtype=np.float32)
        B = np.ones((a.kmax, 8), dtype=np.float32)
    else:
        rng = np.random.default_rng(a.seed)
        A = rng.integers(-4, 5, size=(8, a.kmax)).astype(np.float32)
        B = rng.integers(-4, 5, size=(a.kmax, 8)).astype(np.float32)

    payload = build_payload(A, B, a.kmax)
    assert len(payload) == rx_bytes, (len(payload), rx_bytes)

    exp_c0, exp_c1 = expected_contexts(A, B, a.kmax)
    exp_c = (A @ B).astype(np.float32)

    print(f"K_MAX      : {a.kmax}  ({a.kmax // 8} windows)")
    print(f"port       : {a.port} @ {a.baud}")
    print(f"TX bytes   : {rx_bytes}")
    print(f"RX expected: {tx_bytes}")
    print()

    with serial.Serial(a.port, a.baud, timeout=1) as ser:
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        time.sleep(0.1)

        ser.write(payload)
        ser.flush()
        print("sent, waiting for FPGA...")

        rx = bytearray()
        deadline = time.time() + a.timeout
        while len(rx) < tx_bytes and time.time() < deadline:
            chunk = ser.read(tx_bytes - len(rx))
            if chunk:
                rx.extend(chunk)
                print(f"\rRX {len(rx)}/{tx_bytes}", end="", flush=True)
        print()

    if len(rx) != tx_bytes:
        print(f"FAIL: expected {tx_bytes} bytes, got {len(rx)}")
        if len(rx) and rx[0] in (0xA1, 0xA2, 0xA3, 0xA4, 0xA5):
            print(f"      first byte is 0x{rx[0]:02X}, a breadcrumb marker --")
            print("      this bitstream was built with DEBUG_MARKERS=1.")
        if not rx:
            print("      nothing came back. Did you press BTNC (reset) after")
            print("      programming? k_dim loads K_MAX on reset only; out of")
            print("      configuration it is 0 and the feed FSM injects nothing.")
        return 1

    raw = np.frombuffer(bytes(rx), dtype="<f4").copy()
    got_c0 = raw[:64].reshape(8, 8)
    got_c1 = raw[64:].reshape(8, 8)
    got_c = (got_c0 + got_c1).astype(np.float32)

    np.set_printoptions(precision=3, suppress=True, linewidth=140)

    ok0 = np.array_equal(got_c0, exp_c0)
    ok1 = np.array_equal(got_c1, exp_c1)
    okc = np.array_equal(got_c, exp_c)

    print(f"ctx0 (even windows) : {'BIT-EXACT' if ok0 else 'MISMATCH'}")
    print(f"ctx1 (odd  windows) : {'BIT-EXACT' if ok1 else 'MISMATCH'}")
    print(f"C = ctx0 + ctx1     : {'BIT-EXACT' if okc else 'MISMATCH'}")

    if not (ok0 and ok1):
        for name, got, exp in (("ctx0", got_c0, exp_c0),
                               ("ctx1", got_c1, exp_c1)):
            if not np.array_equal(got, exp):
                print(f"\n--- {name} got ---\n{got}")
                print(f"--- {name} expected ---\n{exp}")
                print(f"--- {name} diff ---\n{got - exp}")
        print("\nIf ctx0 is right and ctx1 is wrong (or vice versa), suspect")
        print("the window->context mapping. If both are wrong by a whole")
        print("window's worth, suspect the RX framing: this build expects")
        print(f"exactly {rx_bytes} bytes for K_MAX={a.kmax}.")
        return 1

    print(f"\nmax |error| : {np.abs(got_c - exp_c).max()}")
    print(f"\nPASS: K_MAX={a.kmax} end-to-end bit-exact on hardware.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
