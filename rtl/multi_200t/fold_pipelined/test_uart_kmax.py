#!/usr/bin/env python3
"""Host test for the K_MAX/N-parameterised FP32 fold array.

    python3 test_uart_kmax.py --kmax 64              # 8x8, k_dim = K_MAX
    python3 test_uart_kmax.py --kmax 64 --k 36       # runtime k_dim
    python3 test_uart_kmax.py --kmax 64 --n 4        # 4x4 bitstream

--n 是陣列邊長(bitstream 的 N generic),預設 8。隨 N 改變的只有
lane 數(payload = K_MAX * 8N bytes)與回應長度(8*N*N bytes);
8-deep k window、ctx 交替、標頭與 frame marker 都是協定常數,不變。

Two distinct quantities, deliberately separate arguments:

    --kmax   synthesis-time hardware capacity of the loaded bitstream
    --k      runtime valid reduction length of THIS invocation

WIRE FORMAT
-----------
Request, MARK_BYTES + HDR_BYTES + K_MAX*64 + MARK_BYTES bytes:

    [ k_dim : 4 bytes little-endian ] [ A/B payload ]

The payload is A and B interleaved one 8-deep k window at a time and is
always full length -- operand storage and the RX framing are sized by
K_MAX, and positions at k >= k_dim are simply never read by the feeder:

    A[:, 0:8]  B[0:8, :]  A[:, 8:16]  B[8:16, :]  ...

Each matrix is 8x8 float32, little-endian, row-major:

    A window w holds A[row][k] for k = 8w .. 8w+7   -> indexed [row][k-8w]
    B window w holds B[k][col] for k = 8w .. 8w+7   -> indexed [k-8w][col]

Response, always 512 bytes regardless of K_MAX or k_dim:

    C_ctx0 (8x8 float32) then C_ctx1 (8x8 float32)

There are two accumulator contexts in hardware, so even-numbered k
windows accumulate into ctx0 and odd-numbered into ctx1. The host
computes the final result:

    C = C_ctx0 + C_ctx1

That cross-context add is not in hardware and does not change with
K_MAX or k_dim.

POISONED TAIL
-------------
By default the payload positions at k >= k_dim are filled with a
non-zero pattern rather than zeros. Zero padding cannot distinguish a
design that honours k_dim from one that ignores it and reduces the full
K_MAX, because the extra terms would contribute nothing either way. A
non-zero tail makes that difference observable: if the hardware reads
past k_dim, the result is wrong. Use --zero-tail to pad with zeros
instead.

NUMERICS
--------
The default stimulus uses small integers so every product and partial
sum is exactly representable in float32. The hardware's summation order
(16 rotating accumulator banks, then a linear reduction) differs from
numpy's, so a floating-point stimulus would legitimately differ in the
last ulp; integers remove that ambiguity and let the test demand
bit-exactness.
"""

import argparse
import sys
import time

import numpy as np

try:
    import serial
except ImportError:
    sys.exit("pyserial not installed:  pip install pyserial")


HDR_BYTES = 4

# Frame markers. Little-endian on the wire, matching the header's k_dim
# and the RTL's sliding window, in which the first byte received ends up
# in the low byte of the compared word.
#
# These exist because the transaction used to be a bare fixed-length
# burst: with no delimiter, a host that sent the wrong number of bytes
# left the receiver pointing into the middle of a frame and every later
# request was split across two of them, until the board was reset by
# hand. The receiver now hunts for FRAME_START and only acts on a frame
# whose FRAME_END is where the length says it should be.
FRAME_START = (0xA55AC33C).to_bytes(4, "little")
FRAME_END = (0x5AA53CC3).to_bytes(4, "little")
MARK_BYTES = 4


def build_request(k_dim, A, B, kmax):
    """A is n x kmax, B is kmax x n. Returns the full request bytes."""
    out = bytearray(int(k_dim).to_bytes(HDR_BYTES, "little"))
    for w in range(kmax // 8):
        ks = slice(w * 8, w * 8 + 8)
        out += np.ascontiguousarray(A[:, ks], dtype="<f4").tobytes()
        out += np.ascontiguousarray(B[ks, :], dtype="<f4").tobytes()
    return bytes(out)


def expected_contexts(A, B, k_dim, n):
    """What each accumulator context should hold after reducing k_dim steps.

    Mirrors the RTL exactly: the context for reduction step gk is
    (gk >> 3) & 1, so a partial trailing window contributes only its
    valid steps.
    """
    c0 = np.zeros((n, n), dtype=np.float32)
    c1 = np.zeros((n, n), dtype=np.float32)
    for gk in range(k_dim):
        term = np.outer(A[:, gk], B[gk, :]).astype(np.float32)
        if ((gk >> 3) & 1) == 0:
            c0 += term
        else:
            c1 += term
    return c0, c1


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--kmax", type=int, default=64,
                    help="hardware capacity of the loaded bitstream")
    ap.add_argument("--k", type=int, default=None,
                    help="runtime reduction length (default: --kmax)")
    ap.add_argument("--n", type=int, default=8,
                    help="array edge length of the loaded bitstream (default 8)")
    ap.add_argument("--port", default="/dev/ttyUSB2")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--timeout", type=float, default=60.0)
    ap.add_argument("--seed", type=int, default=0)
    ap.add_argument("--ones", action="store_true",
                    help="all-ones stimulus instead of random integers")
    ap.add_argument("--zero-tail", action="store_true",
                    help="pad k >= k_dim with zeros instead of a poison pattern")
    a = ap.parse_args()

    kmax = a.kmax
    n = a.n
    k = a.k if a.k is not None else kmax

    if n < 2 or (n & (n - 1)):
        sys.exit(f"--n must be a power of two >= 2 (got {n})")

    if kmax < 16 or kmax % 8:
        sys.exit(f"--kmax must be a multiple of 8 and >= 16 (got {kmax})")
    if not (1 <= k <= kmax):
        sys.exit(f"--k must be in [1, {kmax}] (got {k})")

    rx_bytes = kmax * 8 * n
    tx_bytes = 8 * n * n
    req_bytes = MARK_BYTES + HDR_BYTES + rx_bytes + MARK_BYTES

    if a.ones:
        A = np.ones((n, kmax), dtype=np.float32)
        B = np.ones((kmax, n), dtype=np.float32)
    else:
        rng = np.random.default_rng(a.seed)
        A = rng.integers(-4, 5, size=(n, kmax)).astype(np.float32)
        B = rng.integers(-4, 5, size=(kmax, n)).astype(np.float32)

    if k < kmax:
        if a.zero_tail:
            A[:, k:] = 0.0
            B[k:, :] = 0.0
        else:
            # Poison: large, non-cancelling, and exactly representable.
            A[:, k:] = 1024.0
            B[k:, :] = 1024.0

    request = FRAME_START + build_request(k, A, B, kmax) + FRAME_END
    assert len(request) == req_bytes, (len(request), req_bytes)

    exp_c0, exp_c1 = expected_contexts(A, B, k, n)
    exp_c = (A[:, :k] @ B[:k, :]).astype(np.float32)

    print(f"K_MAX      : {kmax}   (hardware capacity)")
    print(f"N          : {n}   (array edge length)")
    print(f"k_dim      : {k}   (this invocation)")
    print(f"tail       : {'zeros' if a.zero_tail else 'poisoned with 1024.0'}")
    print(f"port       : {a.port} @ {a.baud}")
    print(
        f"request    : {req_bytes} bytes  "
        f"({MARK_BYTES} start + {HDR_BYTES} header + "
        f"{rx_bytes} payload + {MARK_BYTES} end)"
    )
    print(f"response   : {tx_bytes} bytes expected")
    print()

    with serial.Serial(a.port, a.baud, timeout=1) as ser:
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        time.sleep(0.1)

        ser.write(request)
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

        # DEBUG_MARKERS=1 的 bitstream 會在結果流前面多出最多 5 個
        # breadcrumb bytes（A1..A5）。不剝掉的話每個 float 都位移,
        # 512 個結果 byte 全部變成垃圾 -- 第一個值會是 0xA4A3A2A1
        # 解出來的 -3.5e-17,印成 "-0."。剝掉幾個就補讀幾個,
        # 對齊之後其餘檢查照舊;乾淨 bitstream 完全不受影響。
        markers = []
        while rx and rx[0] in (0xA1, 0xA2, 0xA3, 0xA4, 0xA5):
            markers.append(rx.pop(0))
        if markers:
            names = {0xA1: "frame 已接受", 0xA2: "進入 WAIT_RESULT",
                     0xA3: "ctx0 出爐",   0xA4: "ctx1 出爐",
                     0xA5: "進入 SEND"}
            print("breadcrumbs :", " ".join(f"0x{b:02X}({names[b]})"
                                            for b in markers))
            print("              (DEBUG_MARKERS=1 bitstream;已剝除並補讀對齊)")
            deadline = time.time() + 5
            while len(rx) < tx_bytes and time.time() < deadline:
                chunk = ser.read(tx_bytes - len(rx))
                if chunk:
                    rx.extend(chunk)

        # CYCLE_COUNTER=1 的 bitstream 會在 512 bytes 之後再送 4 bytes
        # （小端序）。用短 timeout 試讀:沒有就是舊 bitstream，其餘檢查
        # 完全不受影響。必須在 with 區塊內讀，離開後 ser 就關了。
        _saved_to = ser.timeout
        ser.timeout = 0.5
        cyc_raw = ser.read(4)
        ser.timeout = _saved_to

    if len(cyc_raw) == 4:
        cyc = int.from_bytes(cyc_raw, "little")
        # 週期模型:k + 2(N-1) + 105。
        #   2(N-1) = fill/drain(輸入 skew N-1 + 輸出波前 N-1)
        #   105    = 幾何無關的常數(FP IP 管線、FSM、BRAM 同步讀的
        #            那一拍 -- 118 變 119 的來源)
        # 兩個錨點都是實測:N=8 量到 k+119(xsim 與板上一致),
        # N=4 量到 k+111(2026-08-20 板上,恰好少 2*(8-4)=8 拍)。
        exp = k + 2 * (n - 1) + 105
        print(f"hardware cycles : {cyc}   (模型 k + 2(N-1) + 105 = {exp})")
        if cyc == exp:
            print("                  與模擬完全一致")
        else:
            print(f"                  差 {cyc - exp:+d} 拍")
    else:
        print("hardware cycles : 未回報（此 bitstream 的 CYCLE_COUNTER 為 0）")

    if len(rx) != tx_bytes:
        print(f"FAIL: expected {tx_bytes} bytes, got {len(rx)}")
        if len(rx) and rx[0] in (0xA1, 0xA2, 0xA3, 0xA4, 0xA5):
            lead = []
            for b in rx:
                if b in (0xA1, 0xA2, 0xA3, 0xA4, 0xA5):
                    lead.append(f"0x{b:02X}")
                else:
                    break
            print(f"      leading breadcrumbs: {' '.join(lead)}  (DEBUG_MARKERS=1)")
            print("      A1=frame 已被接受  A2=進入 WAIT_RESULT  A3=ctx0 出爐")
            print("      A4=ctx1 出爐      A5=進入 SEND")
            print("      斷在哪個 marker 之後,就是卡在那一級。")
        if not rx:
            print("      nothing came back. Two usual causes:")
            print("      1. no reset after programming -- k_dim loads K_MAX on")
            print("         reset only, so press BTNC before the first request;")
            print("      2. bitstream predates the request header, in which")
            print("         case it is still waiting for 4 more payload bytes.")
        return 1

    raw = np.frombuffer(bytes(rx), dtype="<f4").copy()
    got_c0 = raw[:n * n].reshape(n, n)
    got_c1 = raw[n * n:].reshape(n, n)
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
        print()
        if k < kmax and not a.zero_tail:
            full_c0, full_c1 = expected_contexts(A, B, kmax, n)
            if np.array_equal(got_c0, full_c0) and np.array_equal(got_c1, full_c1):
                print("DIAGNOSIS: the result matches a full K_MAX reduction, so")
                print("the header was not honoured -- k_dim is still K_MAX.")
                print("Check that this bitstream includes the request header.")
                return 1
        print("If ctx0 is right and ctx1 is wrong (or vice versa), suspect the")
        print("window->context mapping. If both are wrong by a whole window,")
        print("suspect the RX framing or the header length.")
        return 1

    print(f"\nmax |error| : {np.abs(got_c - exp_c).max()}")
    print(f"\nPASS: K_MAX={kmax}, k_dim={k} end-to-end bit-exact on hardware.")
    return 0


if __name__ == "__main__":
    sys.exit(main())