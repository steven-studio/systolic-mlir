#!/usr/bin/env python3
"""Multi-invocation host driver: reduce a workload K deeper than the
hardware capacity K_MAX by issuing ceil(K / K_MAX) successive invocations.

    python3 test_uart_multi_invocation.py --kmax 64 --K 65
    python3 test_uart_multi_invocation.py --kmax 64 --K 100
    python3 test_uart_multi_invocation.py --kmax 64 --K 128
    python3 test_uart_multi_invocation.py --kmax 64 --K 65 --dry-run

DECOMPOSITION
-------------
    K_MAX = 64, K = 65   ->  k_dim = 64, 1
    K_MAX = 64, K = 100  ->  k_dim = 64, 36
    K_MAX = 64, K = 128  ->  k_dim = 64, 64

Invocation i covers the global reduction range

    [ i*K_MAX , min((i+1)*K_MAX, K) )

and is issued at its TRUE depth, not padded up to capacity. That is the
whole point of the runtime k_dim header: the remainder invocation of a
K=65 workload reduces one step, not sixty-four.

ACCUMULATION IS ON THE HOST
---------------------------
The request format carries A and B only -- there is no C_init input, so
the array cannot be seeded with a running partial sum. Each invocation
therefore returns its own pair of context matrices and the host forms

    C = sum over invocations of ( C_ctx0 + C_ctx1 )

This is a property of the current wire format, not of the cost model:
each invocation still pays its own fill/drain and per-invocation
overhead either way, which is what the cycle prediction below measures.
Adding a C_init path would move the accumulation into hardware without
changing the cycle accounting.

CONTEXT MAPPING IS LOCAL
------------------------
Within one invocation the accumulator context for reduction step gk is
(gk >> 3) & 1, with gk counted from 0 at the start of THAT invocation --
not from the start of the workload. A remainder invocation of depth 36
therefore fills windows 0..4 locally, regardless of where those steps
sit in the global K range.

CYCLE PREDICTION
----------------
Per invocation, on an 8x8 array at depth 1:

    cycles_i = k_dim_i + (rows + cols - 2) + c0
             = k_dim_i + 14 + 386

so a whole workload costs

    sum_i k_dim_i + n_inv * 400  =  K + ceil(K/K_MAX) * 400

These are PREDICTIONS. The design counts elapsed cycles internally
(last_accel_cycles) but does not transmit them, so this script cannot
measure them. Confirming the per-invocation overhead on hardware needs
that counter exposed in the response.
"""

import argparse
import math
import sys
import time

import numpy as np

try:
    import serial
except ImportError:
    serial = None


HDR_BYTES = 4
TX_BYTES = 512

# Measured on this implementation: fill/drain = rows+cols-2 = 14,
# per-invocation overhead c0 = 386 (16 accumulator banks x 2 contexts
# reduced through one shared FP adder). See SWEEP.md.
FILL_DRAIN = 14
C0 = 386


def plan(K, kmax):
    """The invocation schedule: list of (start, k_dim)."""
    out = []
    start = 0
    while start < K:
        out.append((start, min(kmax, K - start)))
        start += kmax
    return out


def build_request(k_dim, A_slice, B_slice, kmax, poison=1024.0):
    """A_slice is 8 x k_dim, B_slice is k_dim x 8, placed at LOCAL k 0..k_dim-1.

    Positions at local k >= k_dim are filled with a non-zero pattern so
    that a design ignoring k_dim produces a visibly wrong answer rather
    than a coincidentally correct one.
    """
    A = np.full((8, kmax), poison, dtype=np.float32)
    B = np.full((kmax, 8), poison, dtype=np.float32)
    A[:, :k_dim] = A_slice
    B[:k_dim, :] = B_slice

    out = bytearray(int(k_dim).to_bytes(HDR_BYTES, "little"))
    for w in range(kmax // 8):
        ks = slice(w * 8, w * 8 + 8)
        out += np.ascontiguousarray(A[:, ks], dtype="<f4").tobytes()
        out += np.ascontiguousarray(B[ks, :], dtype="<f4").tobytes()
    return bytes(out)


def expected_contexts(A_slice, B_slice, k_dim):
    """Per-invocation context split, mirroring the RTL's local gk >> 3."""
    c0 = np.zeros((8, 8), dtype=np.float32)
    c1 = np.zeros((8, 8), dtype=np.float32)
    for gk in range(k_dim):
        term = np.outer(A_slice[:, gk], B_slice[gk, :]).astype(np.float32)
        if ((gk >> 3) & 1) == 0:
            c0 += term
        else:
            c1 += term
    return c0, c1


def one_invocation(ser, req, timeout):
    ser.reset_input_buffer()
    ser.write(req)
    ser.flush()
    rx = bytearray()
    deadline = time.time() + timeout
    while len(rx) < TX_BYTES and time.time() < deadline:
        chunk = ser.read(TX_BYTES - len(rx))
        if chunk:
            rx.extend(chunk)
    return bytes(rx)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--kmax", type=int, default=64,
                    help="hardware capacity of the loaded bitstream")
    ap.add_argument("--K", type=int, required=True,
                    help="workload reduction length")
    ap.add_argument("--port", default="/dev/ttyUSB2")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--timeout", type=float, default=60.0)
    ap.add_argument("--seed", type=int, default=0)
    ap.add_argument("--settle", type=float, default=0.05,
                    help="pause between invocations, seconds")
    ap.add_argument("--dry-run", action="store_true",
                    help="check the decomposition and predictions, no hardware")
    a = ap.parse_args()

    kmax, K = a.kmax, a.K
    if kmax < 16 or kmax % 8:
        sys.exit(f"--kmax must be a multiple of 8 and >= 16 (got {kmax})")
    if K < 1:
        sys.exit(f"--K must be >= 1 (got {K})")

    sched = plan(K, kmax)
    n_inv = len(sched)
    assert n_inv == math.ceil(K / kmax), (n_inv, math.ceil(K / kmax))

    rng = np.random.default_rng(a.seed)
    A = rng.integers(-4, 5, size=(8, K)).astype(np.float32)
    B = rng.integers(-4, 5, size=(K, 8)).astype(np.float32)
    reference = (A @ B).astype(np.float32)

    per_inv = [d + FILL_DRAIN + C0 for _, d in sched]
    total_cycles = sum(per_inv)

    print(f"K_MAX          : {kmax}")
    print(f"K              : {K}")
    print(f"ceil(K/K_MAX)  : {n_inv} invocation(s)")
    print()
    print(f"{'inv':>4} {'k range':>12} {'k_dim':>6} {'predicted cycles':>17}")
    print("-" * 44)
    for i, ((s, d), c) in enumerate(zip(sched, per_inv)):
        print(f"{i:>4} {f'[{s},{s+d})':>12} {d:>6} {c:>17}")
    print("-" * 44)
    print(f"{'':>4} {'':>12} {sum(d for _, d in sched):>6} {total_cycles:>17}")
    print()
    print(f"model: K + ceil(K/K_MAX) * (rows+cols-2 + c0)"
          f" = {K} + {n_inv} * {FILL_DRAIN + C0} = {K + n_inv * (FILL_DRAIN + C0)}")
    print("       (prediction only -- last_accel_cycles is not transmitted,")
    print("        so this script cannot measure per-invocation overhead)")
    print()

    if a.dry_run:
        # Verify the decomposition reconstructs the reference exactly,
        # without touching hardware.
        acc = np.zeros((8, 8), dtype=np.float32)
        for s, d in sched:
            c0, c1 = expected_contexts(A[:, s:s + d], B[s:s + d, :], d)
            acc = (acc + c0 + c1).astype(np.float32)
        ok = np.array_equal(acc, reference)
        print(f"dry run: host decomposition reproduces A@B exactly: {ok}")
        return 0 if ok else 1

    if serial is None:
        sys.exit("pyserial not installed:  pip install pyserial")

    acc = np.zeros((8, 8), dtype=np.float32)
    with serial.Serial(a.port, a.baud, timeout=1) as ser:
        time.sleep(0.1)
        for i, (s, d) in enumerate(sched):
            A_s = np.ascontiguousarray(A[:, s:s + d])
            B_s = np.ascontiguousarray(B[s:s + d, :])
            req = build_request(d, A_s, B_s, kmax)

            print(f"invocation {i}: k range [{s},{s+d}), k_dim={d}, "
                  f"{len(req)} bytes ... ", end="", flush=True)
            rx = one_invocation(ser, req, a.timeout)

            if len(rx) != TX_BYTES:
                print(f"FAIL ({len(rx)}/{TX_BYTES} bytes)")
                if rx and rx[0] in (0xA1, 0xA2, 0xA3, 0xA4, 0xA5):
                    print("  breadcrumb marker received -- bitstream built "
                          "with DEBUG_MARKERS=1")
                if not rx:
                    print("  nothing returned. Did you reset after programming,")
                    print("  and does this bitstream include the k_dim header?")
                return 1

            raw = np.frombuffer(rx, dtype="<f4").copy()
            got0, got1 = raw[:64].reshape(8, 8), raw[64:].reshape(8, 8)

            exp0, exp1 = expected_contexts(A_s, B_s, d)
            if not (np.array_equal(got0, exp0) and np.array_equal(got1, exp1)):
                print("MISMATCH")
                print(f"  ctx0 got\n{got0}\n  ctx0 expected\n{exp0}")
                print(f"  ctx1 got\n{got1}\n  ctx1 expected\n{exp1}")
                if d < kmax:
                    full0, full1 = expected_contexts(
                        np.pad(A_s, ((0, 0), (0, kmax - d)), constant_values=1024.0),
                        np.pad(B_s, ((0, kmax - d), (0, 0)), constant_values=1024.0),
                        kmax)
                    if np.array_equal(got0, full0):
                        print("  DIAGNOSIS: result matches a full K_MAX reduction --")
                        print("  the k_dim header was not honoured on this invocation.")
                return 1

            acc = (acc + got0 + got1).astype(np.float32)
            print("ok")
            time.sleep(a.settle)

    print()
    exact = np.array_equal(acc, reference)
    print(f"accumulated C over {n_inv} invocation(s) vs A@B : "
          f"{'BIT-EXACT' if exact else 'MISMATCH'}")
    if not exact:
        print(f"--- got ---\n{acc}\n--- expected ---\n{reference}")
        print(f"--- diff ---\n{acc - reference}")
        return 1

    print(f"max |error| : {np.abs(acc - reference).max()}")
    print()
    print(f"PASS: K={K} on K_MAX={kmax} hardware via {n_inv} invocation(s), "
          f"bit-exact.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
