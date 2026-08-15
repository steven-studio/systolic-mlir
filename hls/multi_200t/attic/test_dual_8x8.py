#!/usr/bin/env python3
"""Smoke-test the dual 8x8 accelerator over UART.

    python3 test_dual_8x8.py [port] [baud]
    python3 test_dual_8x8.py /dev/ttyUSB0 115200

Deliberately standalone: the runtime library still speaks the 4x4 protocol
(192 bytes in, 64 out, no device byte), so bringing the board up through it
would mean rewriting the library before ever confirming the wiring works.
This script talks the board's protocol directly and answers one question --
does a single tile come back correct, on each of the two arrays?

Protocol, from matmul_top_dual.v:
    host -> board : 1 byte device select (0x00 / 0x01)
                    256 bytes A       (8x8 fp32, row-major)
                    256 bytes B
                    256 bytes C_init
    board -> host : 256 bytes C       (8x8 fp32)
All little-endian: the RX shift register assembles words LSB-first.

The reference accumulates over k in order, matching what the array does --
PE(i,j) performs its k-th MAC on beat i+j+k, so its accumulator sees the
products in ascending k. Any other order differs by a few ULP and turns an
equality check into a tolerance check.
"""

import sys
import time

import numpy as np
import serial

N = 8
BYTES_IN = 1 + 3 * N * N * 4      # 769
BYTES_OUT = N * N * 4             # 256


def reference(A, B, Cinit):
    """fp32, sequential k -- must be bit-identical to the hardware."""
    A = A.astype(np.float32)
    B = B.astype(np.float32)
    C = Cinit.astype(np.float32).copy()
    for k in range(A.shape[1]):
        C = (C + A[:, k:k+1] * B[k:k+1, :]).astype(np.float32)
    return C


def ulp(a, b):
    ai = a.astype(np.float32).view(np.int32).astype(np.int64)
    bi = b.astype(np.float32).view(np.int32).astype(np.int64)
    ai = np.where(ai < 0, np.int64(0x80000000) - ai, ai)
    bi = np.where(bi < 0, np.int64(0x80000000) - bi, bi)
    return np.abs(ai - bi)


def one_tile(ser, dev, A, B, Cinit):
    payload = (bytes([dev])
               + A.astype('<f4').tobytes()
               + B.astype('<f4').tobytes()
               + Cinit.astype('<f4').tobytes())
    assert len(payload) == BYTES_IN, len(payload)

    # Drain anything stale before starting: the protocol has no framing, so
    # a leftover byte from an aborted run shifts every field by one.
    ser.reset_input_buffer()
    ser.reset_output_buffer()

    ser.write(payload)
    ser.flush()

    raw = ser.read(BYTES_OUT)
    if len(raw) != BYTES_OUT:
        return None, f"讀回 {len(raw)} bytes，預期 {BYTES_OUT}"
    return np.frombuffer(raw, dtype='<f4').reshape(N, N).copy(), None


def main():
    port = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyUSB0"
    baud = int(sys.argv[2]) if len(sys.argv) > 2 else 115200

    print(f"port={port} baud={baud}  ({BYTES_IN} bytes out / {BYTES_OUT} back)")
    ser = serial.Serial(port, baud, timeout=10)
    time.sleep(0.2)

    rng = np.random.default_rng(1234)
    fails = 0

    for dev in (0, 1):
        # Small integer-valued data first: any wiring or byte-order fault
        # shows up as a wildly wrong number rather than a rounding question.
        A = rng.integers(-4, 5, size=(N, N)).astype(np.float32)
        B = rng.integers(-4, 5, size=(N, N)).astype(np.float32)
        C0 = np.zeros((N, N), dtype=np.float32)

        got, err = one_tile(ser, dev, A, B, C0)
        if err:
            print(f"  dev={dev}  FAIL: {err}")
            fails += 1
            continue
        ref = reference(A, B, C0)
        u = ulp(got, ref)
        nz = int((u != 0).sum())
        print(f"  dev={dev}  整數資料: {'BIT-EXACT' if nz == 0 else f'{nz}/{N*N} 不符, max_ulp={u.max()}'}")
        if nz:
            fails += 1
            bad = np.argwhere(got != ref)[:3]
            for (i, j) in bad:
                print(f"      ({i},{j}) got={got[i,j]:.6g} ref={ref[i,j]:.6g}")

        # Then random floats plus a non-zero C_init, which is what the tiled
        # driver actually sends: accumulation across K tiles feeds the running
        # sum back in as C_init.
        A = rng.standard_normal((N, N)).astype(np.float32)
        B = rng.standard_normal((N, N)).astype(np.float32)
        C0 = rng.standard_normal((N, N)).astype(np.float32)
        got, err = one_tile(ser, dev, A, B, C0)
        if err:
            print(f"  dev={dev}  FAIL: {err}")
            fails += 1
            continue
        ref = reference(A, B, C0)
        u = ulp(got, ref)
        nz = int((u != 0).sum())
        print(f"  dev={dev}  浮點+C_init: {'BIT-EXACT' if nz == 0 else f'{nz}/{N*N} 不符, max_ulp={u.max()}'}")
        if nz:
            fails += 1

    ser.close()
    print("\n" + ("全部通過" if fails == 0 else f"{fails} 項失敗"))
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
