#!/usr/bin/env python3
"""dump_ref_new.py -- reference serialisation for the runtime-K wire format.

Emits the exact bytes test_rk_new.py would put on the wire, for a fixed
deterministic input, so the C packer can be diffed against it without a
board attached.

The input is deliberately NOT random and NOT symmetric. Values are chosen so
that a transposed A, a swapped A/B, or a wrong K width all produce a diff at
a byte offset that says which mistake it was:

    A[i][k] = 100*i + k        distinct per (i,k), asymmetric under transpose
    B[k][j] = -(10*k + j) - 0.5   negative and fractional -> catches sign/
                                  exponent errors that integers would hide
    C[i][j] = 1000 + 8*i + j

Usage:
    python3 dump_ref_new.py <K> <dev> <out.bin>
"""

import struct
import sys

R = 8
C = 8
K_MAX = 64


def make_inputs(K):
    A = [[float(100 * i + k) for k in range(K)] for i in range(R)]
    B = [[-(float(10 * k + j)) - 0.5 for j in range(C)] for k in range(K)]
    Cin = [[float(1000 + 8 * i + j) for j in range(C)] for i in range(R)]
    return A, B, Cin


def pack(dev, K, A, B, Cin):
    buf = bytearray()
    buf += bytes([dev])
    buf += struct.pack('<I', K)

    for i in range(R):
        for k in range(K):
            buf += struct.pack('<f', float(A[i][k]))

    # B is transposed on the wire: bank j = column j, so j is the OUTER loop.
    # Swapping this nest still type-checks and still produces K*C floats of
    # the right total length -- and is indistinguishable at K=1.
    for j in range(C):
        for k in range(K):
            buf += struct.pack('<f', float(B[k][j]))

    for i in range(R):
        for j in range(C):
            buf += struct.pack('<f', float(Cin[i][j]))

    return bytes(buf)


def main():
    if len(sys.argv) != 4:
        print(__doc__)
        return 2

    K = int(sys.argv[1])
    dev = int(sys.argv[2])
    out = sys.argv[3]

    if not (1 <= K <= K_MAX):
        print(f"K must be in [1, {K_MAX}], got {K}")
        return 2

    A, B, Cin = make_inputs(K)
    blob = pack(dev, K, A, B, Cin)

    expected = 261 + 64 * K

    if len(blob) != expected:
        print(f"INTERNAL: packed {len(blob)} B, formula says {expected} B")
        return 1

    with open(out, 'wb') as f:
        f.write(blob)

    print(f"K={K} dev={dev} -> {len(blob)} B written to {out}")
    return 0


if __name__ == '__main__':
    sys.exit(main())