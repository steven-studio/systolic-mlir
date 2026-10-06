#!/usr/bin/env python3
"""
seed_ref.py -- software twin of dma_seed_writer's pattern, and of the decode
that lands it in the operand buffers.

Three jobs, all of which need the SAME model, which is why they live in one
file rather than three:

  1. Emit the UART payload.  Bring-up step 3b runs one fold twice on one
     bitstream -- once fed over UART, once fed by the DMA -- and compares.  That
     only means anything if the two paths carry byte-identical operands, so the
     host sender must generate exactly what dma_seed_writer wrote into DRAM.

  2. Compute EXPECT_CHK for dma_bringup_top, using the same position-weighted
     formula the hardware accumulates.

  3. Self-check.  Mode 0 must reproduce 0x387fdc00, the checksum measured on
     the board in step 3a.  That constant was produced by hardware this script
     has never seen, so agreement is evidence the model is right -- and if a
     later edit breaks the model, mode 0 says so immediately, before a wrong
     mode-1 constant gets burned into a bitstream and read as a real failure.

The wire format is not invented here; it is dma_operand_writer's decode, which
is in turn systolic_uart_top's rx_count decode.  See those files for why every
field is a plain bit slice.
"""

import argparse
import struct
import sys

MASK32 = 0xFFFFFFFF

# systolic_uart_top's RX framing:
#   FRAME_START(4) | k_dim(4) | payload(K_MAX*8*N) | FRAME_END(4)
# all little-endian words -- the receiver's sliding window puts the oldest byte
# in bits [7:0], which is the same convention as the header word.
FRAME_START = 0xA55AC33C
FRAME_END = 0x5AA53CC3


def fp32_small(v: int) -> int:
    """fp32 bit pattern of a small positive integer, the way the RTL builds it.

    Deliberately NOT struct.pack: this mirrors the hardware's priority encode +
    shift so that the two can be compared.  check_fp32() below is what confirms
    the mirror is faithful."""
    assert 1 <= v <= 127
    e = v.bit_length() - 1
    return ((127 + e) << 23) | ((v << (23 - e)) & 0x7FFFFF)


def check_fp32() -> None:
    """The RTL's construction must agree with IEEE-754 for every value it can
    emit.  A 7-bit integer needs 6 mantissa bits against fp32's 23, so this is
    exact by construction -- but 'by construction' is an argument, and this is
    the check."""
    for v in range(1, 128):
        ref = struct.unpack("<I", struct.pack("<f", float(v)))[0]
        if fp32_small(v) != ref:
            sys.exit(f"fp32_small({v}) = 0x{fp32_small(v):08x}, IEEE says 0x{ref:08x}")


def seed_words(n_words: int, mode: int, modulus: int):
    """The DRAM image, in linear 32-bit word order -- byte order on the wire."""
    if mode == 0:
        return [i & MASK32 for i in range(n_words)]
    return [fp32_small((i % modulus) + 1) for i in range(n_words)]


def place(words, n: int, k_max: int):
    """Run the payload through dma_operand_writer's decode.

    Returns (a, b) as [bank][k].  Field widths follow the module exactly:
        WORD_W = log2(K_MAX*2*N), MAT_W = WORD_W - (LANE_W+3), WIN_W = MAT_W-1
    """
    lane_w = (n - 1).bit_length()
    word_w = (k_max * 2 * n - 1).bit_length()
    mat_w = word_w - (lane_w + 3)

    a = [[None] * k_max for _ in range(n)]
    b = [[None] * k_max for _ in range(n)]

    for w, data in enumerate(words):
        mat = w >> (lane_w + 3)
        is_b = mat & 1
        win = mat >> 1
        if is_b:
            koff = (w >> lane_w) & 7
            lane = w & (n - 1)
            b[lane][(win << 3) | koff] = data
        else:
            koff = w & 7
            lane = (w >> 3) & (n - 1)
            a[lane][(win << 3) | koff] = data

    holes = [(m, l, k) for m, buf in (("A", a), ("B", b))
             for l in range(n) for k in range(k_max) if buf[l][k] is None]
    if holes:
        sys.exit(f"decode left {len(holes)} slot(s) unwritten, first {holes[0]}; "
                 f"mat_w={mat_w} -- the payload does not tile the buffers")
    return a, b


def checksum(a, b, n: int, k_max: int) -> int:
    """dma_bringup_top's accumulator, in the order it scans:

        scan_i = 0 .. K_MAX*N-1,  k = scan_i >> LANE_W,  bank = scan_i & (N-1)
        pos    = (bank << 16) | k
        chk   += (A ^ pos) + (B ^ (0x80000000 | pos))

    XOR-ing the position in is what makes a swap detectable: a plain sum of the
    words is invariant under any permutation, so it would pass on a scrambled
    image."""
    lane_w = (n - 1).bit_length()
    chk = 0
    for scan_i in range(k_max * n):
        k = scan_i >> lane_w
        bank = scan_i & (n - 1)
        pos = ((bank << 16) | k) & MASK32
        chk = (chk + (a[bank][k] ^ pos) + (b[bank][k] ^ (0x80000000 | pos))) & MASK32
    return chk


def golden_c(a, b, n: int, k_dim: int):
    """C = A.B for this operand image, in exact integer arithmetic.

    The buffers hold A[row][k] in bank = row and B[k][col] in bank = col
    (systolic_uart_top: "A_buf is [row][k] and B_buf is [k][col]", written with
    wsel = a_lane and wsel = b_lane respectively), so

        C[r][c] = sum over k < k_dim of  a[r][k] * b[c][k]

    Only k < k_dim contributes: the feeder never reads past it, though the
    buffers are sized to K_MAX and the payload always fills all of them.

    THIS IS EXACT.  Seed values are 1..127, so every product is at most 16129
    and a dot product over k_dim <= 256 is at most 4,129,024 -- comfortably
    inside fp32's 2^24 integer range.  The array's reduction tree can add these
    in any order and get the same answer, which is what makes a bit-exact
    comparison against this model legitimate."""
    c_int = [[0] * n for _ in range(n)]
    limit = 1 << 24
    for r in range(n):
        for col in range(n):
            acc = 0
            for k in range(k_dim):
                acc += fp32_to_int(a[r][k]) * fp32_to_int(b[col][k])
            if acc >= limit:
                sys.exit(f"C[{r}][{col}] = {acc} >= 2^24: no longer exact in "
                         f"fp32, so a bit-exact comparison would be invalid. "
                         f"Lower --modulus or --kdim.")
            c_int[r][col] = acc
    return c_int


def fp32_to_int(bits: int) -> int:
    """Inverse of fp32_small, for values this file put there."""
    return int(struct.unpack("<f", struct.pack("<I", bits))[0])


def int_to_fp32(v: int) -> int:
    return struct.unpack("<I", struct.pack("<f", float(v)))[0]


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--mode", type=int, default=1, choices=(0, 1),
                    help="0 = word index (step 3a), 1 = fp32 1..modulus (step 3b)")
    ap.add_argument("--n", type=int, default=8, help="array edge / bank count")
    ap.add_argument("--kmax", type=int, default=256, help="operand buffer depth")
    ap.add_argument("--modulus", type=int, default=127, help="mode 1 value range")
    ap.add_argument("--payload", metavar="FILE",
                    help="write the UART payload bytes (no framing, no k_dim header)")
    ap.add_argument("--frame", metavar="FILE",
                    help="write a complete systolic_uart_top request: "
                         "FRAME_START | k_dim | payload | FRAME_END")
    ap.add_argument("--kdim", type=int, default=None,
                    help="runtime reduction length (default: K_MAX)")
    args = ap.parse_args()
    k_dim = args.kmax if args.kdim is None else args.kdim
    if not 1 <= k_dim <= args.kmax:
        sys.exit(f"--kdim must be 1..{args.kmax} (systolic_uart_top clamps "
                 f"anything else to K_MAX, which would silently change the "
                 f"golden)")

    check_fp32()

    n_words = args.kmax * 2 * args.n
    words = seed_words(n_words, args.mode, args.modulus)
    a, b = place(words, args.n, args.kmax)
    chk = checksum(a, b, args.n, args.kmax)

    print(f"N = {args.n}   K_MAX = {args.kmax}   mode = {args.mode}"
          + (f"   modulus = {args.modulus}" if args.mode == 1 else ""))
    print(f"payload      {n_words} words = {n_words * 4} bytes = {n_words // 4} beats")
    print(f"EXPECT_CHK   0x{chk:08x}")

    if args.mode == 0:
        golden = 0x387FDC00
        if chk == golden:
            print(f"self-check   mode 0 reproduces the board's 0x{golden:08x}  OK")
        else:
            sys.exit(f"self-check   FAILED: mode 0 gives 0x{chk:08x}, "
                     f"the board measured 0x{golden:08x}.  The model is wrong; "
                     f"do not trust the mode 1 constant.")
    else:
        # A constant image would make the fold's result independent of placement,
        # so a permutation bug could still pass.  Report the spread as evidence
        # the seed actually varies across the buffers.
        vals = {w for w in words}
        print(f"distinct values {len(vals)} (min 0x{min(vals):08x}, max 0x{max(vals):08x})")
        zeros = sum(1 for w in words if w == 0)
        print(f"zero words   {zeros}   (must be 0: a zero operand drops out of its product)")
        if zeros:
            sys.exit("seed contains zero words")

    # ---- what the array should produce from this image ---------------------
    # Only meaningful for mode 1: in mode 0 every operand is a denormal, every
    # product underflows to zero, and the "golden" would be an all-zero matrix
    # that any broken path also produces.
    if args.mode == 1:
        c_int = golden_c(a, b, args.n, k_dim)
        chk_c = 0
        for r in range(args.n):
            for col in range(args.n):
                pos = ((r << 16) | col) & MASK32
                chk_c = (chk_c + (int_to_fp32(c_int[r][col]) ^ pos)) & MASK32
        flat = [v for row in c_int for v in row]
        print()
        print(f"k_dim        {k_dim}")
        print(f"golden C     {args.n}x{args.n}, integer range {min(flat)}..{max(flat)}"
              f"  (2^24 = {1 << 24})")
        print(f"C checksum   0x{chk_c:08x}"
              f"   [sum of (fp32(C[r][c]) ^ ((r<<16)|c))]")
        print(f"C[0][0]      {c_int[0][0]}  = 0x{int_to_fp32(c_int[0][0]):08x}")
        print(f"C[0][1]      {c_int[0][1]}  = 0x{int_to_fp32(c_int[0][1]):08x}")
        print(f"C[{args.n-1}][{args.n-1}]      {c_int[-1][-1]}"
              f"  = 0x{int_to_fp32(c_int[-1][-1]):08x}")
        if len(set(flat)) == 1:
            sys.exit("every entry of C is identical -- this golden cannot "
                     "distinguish a correct result from a transposed or "
                     "rotated one.  Change the seed.")
        print(f"distinct C   {len(set(flat))} of {args.n * args.n}"
              f"   (a permutation of the operands would have to preserve all of them)")

    if args.payload:
        with open(args.payload, "wb") as f:
            for w in words:
                f.write(struct.pack("<I", w))
        print(f"\npayload written to {args.payload} "
              f"({n_words * 4} bytes, no FRAME_START / k_dim / FRAME_END)")

    if args.frame:
        with open(args.frame, "wb") as f:
            f.write(struct.pack("<I", FRAME_START))
            f.write(struct.pack("<I", k_dim))
            for w in words:
                f.write(struct.pack("<I", w))
            f.write(struct.pack("<I", FRAME_END))
        total = 4 + 4 + n_words * 4 + 4
        print(f"\nframe written to {args.frame} ({total} bytes): "
              f"START(4) + k_dim(4) + payload({n_words * 4}) + END(4)")


if __name__ == "__main__":
    main()
