#!/usr/bin/env python3
"""
uart_check.py -- send the step-3b seed over UART to a bitstream that already
exists, and check the array's answer against the golden.

WHY THIS RUNS BEFORE ANY RTL IS TOUCHED
  Everything in bring-up step 3b is compared against a model: the operand image
  against seed_ref's placement, the fold result against seed_ref's golden C.  If
  that model has the row/column convention backwards, or reads k the wrong way,
  then the first DMA-fed run disagrees with it -- and there is no way to tell
  whether the DMA is wrong or the model is.  Debugging that on a fresh bitstream
  is the expensive version of this question.

  So ask it now, on hardware that is already known good.  The UART path has been
  validated across 48 configurations; feeding it this seed and getting the
  golden back confirms the model, and costs one script run and no build.

  If it MISMATCHES, that is not a hardware failure -- it means seed_ref's idea of
  the array's semantics is wrong, and the mismatch pattern says how (a transpose
  shows up as C[r][c] <-> C[c][r], a k-direction error as a completely different
  magnitude).  Fix the model here, where nothing else depends on it yet.

USAGE
  python3 tools/uart_check.py --port /dev/ttyUSB1 --kmax 16
  python3 tools/uart_check.py --port /dev/ttyUSB1 --kmax 16 --cycles --markers

  --kmax must match the K_MAX the loaded bitstream was built with, and --n its N.
  Wrong K_MAX means the wrong payload length, which the receiver's end marker
  will reject -- the symptom is a read timeout, not a mismatch.
"""

import argparse
import struct
import sys
import time

import seed_ref


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--port", required=True, help="e.g. /dev/ttyUSB1")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--n", type=int, default=8)
    ap.add_argument("--kmax", type=int, required=True,
                    help="K_MAX of the LOADED bitstream, not of the DMA path")
    ap.add_argument("--kdim", type=int, default=None)
    ap.add_argument("--modulus", type=int, default=127)
    ap.add_argument("--markers", action="store_true",
                    help="bitstream built with DEBUG_MARKERS = 1 (5 leading bytes)")
    ap.add_argument("--cycles", action="store_true",
                    help="bitstream built with CYCLE_COUNTER = 1 (4 trailing bytes)")
    ap.add_argument("--timeout", type=float, default=10.0)
    ap.add_argument("--scan", action="store_true",
                    help="try a range of K_MAX values and report what each gets "
                         "back; use when you do not know how the loaded "
                         "bitstream was built")
    args = ap.parse_args()

    try:
        import serial
    except ImportError:
        sys.exit("pyserial is not installed: pip install pyserial")

    n, k_max = args.n, args.kmax
    k_dim = k_max if args.kdim is None else args.kdim

    seed_ref.check_fp32()

    if args.scan:
        # TX length is 8*N*N whatever K_MAX is, so the candidate that gets a
        # full-length reply is the one the bitstream was built with.  Nothing
        # about the payload matters here -- this is a framing question only.
        want = 2 * 4 * n * n
        print(f"scanning K_MAX candidates on {args.port}, "
              f"a full reply is {want} bytes\n")
        for cand in (16, 32, 64, 128, 256):
            w = seed_ref.seed_words(cand * 2 * n, 1, args.modulus)
            fr = (struct.pack("<I", seed_ref.FRAME_START)
                  + struct.pack("<I", cand)
                  + b"".join(struct.pack("<I", x) for x in w)
                  + struct.pack("<I", seed_ref.FRAME_END))
            with serial.Serial(args.port, args.baud, timeout=4.0) as ser:
                ser.reset_input_buffer()
                ser.write(fr)
                ser.flush()
                back = ser.read(want)
            note = "  <-- this is the one" if len(back) == want else ""
            head = back[:8].hex(" ") if back else "(nothing)"
            print(f"  K_MAX {cand:4d}   sent {len(fr):6d} B   "
                  f"got {len(back):4d} B   {head}{note}")
        print("\nIf every row got 0 bytes, the board is not running a UART "
              "bitstream at all.  If every row got the SAME small number, that "
              "is noise or a baud mismatch, not framing.")
        return
    words = seed_ref.seed_words(k_max * 2 * n, 1, args.modulus)
    a, b = seed_ref.place(words, n, k_max)
    c_int = seed_ref.golden_c(a, b, n, k_dim)

    frame = (struct.pack("<I", seed_ref.FRAME_START)
             + struct.pack("<I", k_dim)
             + b"".join(struct.pack("<I", w) for w in words)
             + struct.pack("<I", seed_ref.FRAME_END))

    # TX IS ONE COPY OF C, NOT TWO.
    #
    # systolic_uart_top still carries "localparam int TX_BYTES = 8*N*N" and
    # program_kmax.tcl still prints "TX 512 bytes (C_ctx0 then C_ctx1)".  Both
    # are stale: the accumulator contexts were removed ("只剩一片結果" in the
    # breadcrumb comment) and systolic_tx_source now derives its own length.
    # The board sends 4*N*N bytes, plus 4 for the cycle counter when it is
    # enabled -- 260 for N=8, measured.
    #
    # Expecting the old 512 is what made the first run here look like a
    # truncated reply: it read a COMPLETE result and then sat waiting for a
    # second copy that no longer exists.
    c_bytes = 4 * n * n
    n_read = 5 * args.markers + c_bytes

    print(f"port {args.port} @ {args.baud}   N={n} K_MAX={k_max} k_dim={k_dim}")
    print(f"sending {len(frame)} bytes, expecting {n_read} back "
          f"(+4 if this build has CYCLE_COUNTER)")

    try:
        ser = serial.Serial(args.port, args.baud, timeout=args.timeout)
    except serial.SerialException as exc:
        # The board exposes two FTDI channels -- the first is JTAG, the second
        # is this UART -- so the index moves when other USB serial devices are
        # present.  Show what is actually there instead of making the user guess.
        from serial.tools import list_ports
        found = list(list_ports.comports())
        lines = "\n".join(f"    {p.device}   {p.description}" for p in found) \
            or "    (none)"
        sys.exit(f"{exc}\n\nserial ports on this machine:\n{lines}\n\n"
                 f"On a Nexys Video the UART is the SECOND FTDI channel; the "
                 f"first is JTAG.  If nothing is listed, the board is off or "
                 f"unplugged; if a port is listed but will not open, check that "
                 f"you are in the dialout group.")

    with ser:
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        t0 = time.time()
        ser.write(frame)
        ser.flush()
        got = ser.read(n_read)
        # The cycle counter is a build-time option, so rather than require the
        # caller to know, take the result first and then look for four more
        # bytes with a short timeout.  Absent means CYCLE_COUNTER was off; it is
        # not an error either way.
        ser.timeout = 1.0
        tail = ser.read(4) if len(got) == n_read else b""
    dt = time.time() - t0

    if len(got) != n_read:
        print(f"\nread {len(got)} of {n_read} bytes after {dt:.1f} s.")
        if got:
            print(f"got:  {got.hex(' ')}")
            # Deliberately NOT scanning the body for 0xA1..0xA5.  Float data
            # contains those bytes constantly -- every exponent near 2^15 does --
            # so a scan reports breadcrumbs in a perfectly good result and sends
            # the reader off to debug a state machine that never stalled.  Only
            # the first five bytes can be markers, and only when the build has
            # them.
            if args.markers:
                names = {0xA1: "matrices_ready", 0xA2: "ST_WAIT_RESULT entry",
                         0xA3: "result published", 0xA4: "(disused ctx1)",
                         0xA5: "ST_SEND entry"}
                lead = [f"0x{x:02x} = {names.get(x, 'not a marker')}"
                        for x in got[:5]]
                print("\nleading bytes, read as breadcrumbs:")
                for line in lead:
                    print(f"    {line}")
            print("\nA reply shorter than one full C is a truncated result "
                  "stream.  If the byte count is 4*N*N or 4*N*N+4, the design is "
                  "fine and this script's expectation is wrong.")
        else:
            print("Nothing came back at all: the receiver is most likely still "
                  "in RX_BODY waiting for a longer payload, i.e. the loaded "
                  "bitstream has a larger K_MAX than --kmax.  Try --scan.")
        sys.exit(1)

    off = 5 if args.markers else 0
    if args.markers:
        print(f"markers      {got[:5].hex(' ')}")

    hw = list(struct.unpack(f"<{n * n}f", got[off:off + c_bytes]))

    bad = []
    for r in range(n):
        for col in range(n):
            want = float(c_int[r][col])
            have = hw[r * n + col]
            if have != want:
                bad.append((r, col, want, have))

    print(f"round trip   {dt:.2f} s")
    if len(tail) == 4:
        cyc = struct.unpack("<I", tail)[0]
        h = cyc - k_dim - 2 * (n - 1)
        print(f"cycles       {cyc}   (feed_t==0 to c_valid_out, both ends inclusive)")
        print(f"             T = k_dim + 2(N-1) + H  ->  H = {h}")
    else:
        print("cycles       not reported (CYCLE_COUNTER off in this build)")

    if not bad:
        print(f"\n  ALL {n * n} ENTRIES OF C MATCH THE GOLDEN.")
        print("  seed_ref's model of the array is confirmed; the DMA path can be "
              "compared against it.")
        return

    print(f"\n  {len(bad)} of {n * n} ENTRIES DIFFER.")
    for r, col, want, have in bad[:8]:
        print(f"    C[{r}][{col}]  golden {want:.0f}   board {have:.0f}")
    # Name the two failure shapes worth recognising immediately.
    if all(hw[c * n + r] == float(c_int[r][c]) for r in range(n) for c in range(n)):
        print("\n  Every entry matches the TRANSPOSE: seed_ref has A and B the "
              "wrong way round.  Swap them in golden_c(), not in the RTL.")
    elif all(v == 0.0 for v in hw):
        print("\n  The board returned all zeros.  Either the frame was not "
              "accepted, or the operands underflowed -- check that this really "
              "is a mode 1 seed.")
    sys.exit(1)


if __name__ == "__main__":
    main()
