#!/usr/bin/env python3
"""Host test for the K_MAX/N-parameterised FP32 fold array.

    python3 test_uart_kmax.py --kmax 64              # 8x8, k_dim = K_MAX
    python3 test_uart_kmax.py --kmax 64 --k 36       # runtime k_dim
    python3 test_uart_kmax.py --kmax 64 --n 4        # 4x4 bitstream

--n 是陣列邊長(bitstream 的 N generic),預設 8。隨 N 改變的只有
lane 數(payload = K_MAX * 8N bytes)與回應長度(4*N*N bytes);
8-deep k window、標頭與 frame marker 都是協定常數,不變。

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

Response, 4*N*N bytes regardless of K_MAX or k_dim (N=8: 256):

    C (NxN float32)

舊版是 512 bytes:兩個 accumulator context 各一片,主機端再相加。
那兩個 context 從來沒有在時間上重疊過(歸約要等乘法與加法全部
排空才開始),所以它們 pipeline 不了任何東西 —— 整組已經移除。
回應因此砍半,主機端的跨 context 相加也一併消失。

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
(16 rotating accumulator banks, then a tree reduction) differs from
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


def expected_result(A, B, k_dim):
    """歸約 k_dim 步之後那一片 C 應該是什麼。

    ctx 移除之前這裡是 expected_contexts(),回傳兩片(偶數 window
    進 ctx0、奇數進 ctx1),因為硬體會分兩片送回來。現在只有一片,
    所以它退化成一個普通的矩陣乘法。
    """
    return (A[:, :k_dim] @ B[:k_dim, :]).astype(np.float32)


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
    ap.add_argument("--h", type=int, default=105,
                    help="週期模型的幾何無關常數。105 是 ctx 移除前的值,"
                         "重寫之後必定改變 —— 這一版是用來『量』它的")
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
    tx_bytes = 4 * n * n
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

    exp_c = expected_result(A, B, k)

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
                     0xA3: "結果出爐",   0xA4: "(已停用)",
                     0xA5: "進入 SEND"}
            print("breadcrumbs :", " ".join(f"0x{b:02X}({names[b]})"
                                            for b in markers))
            print("              (DEBUG_MARKERS=1 bitstream;已剝除並補讀對齊)")
            deadline = time.time() + 5
            while len(rx) < tx_bytes and time.time() < deadline:
                chunk = ser.read(tx_bytes - len(rx))
                if chunk:
                    rx.extend(chunk)

        # CYCLE_COUNTER=1 的 bitstream 會在結果之後再送 4 bytes
        # （小端序）。用短 timeout 試讀:沒有就是舊 bitstream，其餘檢查
        # 完全不受影響。必須在 with 區塊內讀，離開後 ser 就關了。
        _saved_to = ser.timeout
        ser.timeout = 0.5
        cyc_raw = ser.read(4)
        ser.timeout = _saved_to

    if len(cyc_raw) == 4:
        cyc = int.from_bytes(cyc_raw, "little")
        # 週期模型:T = k + 2(N-1) + H
        #   2(N-1) = fill/drain(輸入 skew N-1 + 輸出波前 N-1)
        #   H      = 幾何無關的常數(FP IP 管線、FSM、BRAM 同步讀那一拍)
        #
        # H 在 ctx 移除之前是 105,兩個錨點:N=8 -> k+119、N=4 -> k+111,
        # 殘差 0。但週期計數器的終點定義變了 —— 舊版數到「ctx1 published」,
        # 也就是連發兩拍的第二拍;現在只發一片,終點提前。
        #
        # 所以這裡不再是「檢查 H 等不等於 105」,而是「量出 H 是多少」。
        # 兩個幾何點都量完之後才重新擬合,不要用單點下結論。
        h_meas = cyc - k - 2 * (n - 1)
        print(f"hardware cycles : {cyc}")
        print(f"  T = k + 2(N-1) + H  ->  H = {cyc} - {k} - {2*(n-1)} = {h_meas}")
        if h_meas == a.h:
            print(f"                        與 --h {a.h} 相同")
        else:
            print(f"                        與 --h {a.h} 差 {h_meas - a.h:+d}"
                  f"   <- ctx 移除後 H 本來就會變,這個數字是結果不是錯誤")
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
            print("      A1=frame 已被接受  A2=進入 WAIT_RESULT  A3=結果出爐")
            print("      A4=(已停用)        A5=進入 SEND")
            print("      斷在哪個 marker 之後,就是卡在那一級。")
        if not rx:
            print("      nothing came back. Two usual causes:")
            print("      1. no reset after programming -- k_dim loads K_MAX on")
            print("         reset only, so press BTNC before the first request;")
            print("      2. bitstream predates the request header, in which")
            print("         case it is still waiting for 4 more payload bytes.")
        return 1

    raw = np.frombuffer(bytes(rx), dtype="<f4").copy()
    got_c = raw.reshape(n, n)

    np.set_printoptions(precision=3, suppress=True, linewidth=140)

    okc = np.array_equal(got_c, exp_c)
    print(f"C : {'BIT-EXACT' if okc else 'MISMATCH'}")

    if not okc:
        print(f"\n--- got ---\n{got_c}")
        print(f"--- expected ---\n{exp_c}")
        print(f"--- diff ---\n{got_c - exp_c}")
        print()
        if k < kmax and not a.zero_tail:
            if np.array_equal(got_c, expected_result(A, B, kmax)):
                print("DIAGNOSIS: 結果等於完整 K_MAX 的歸約,表示 header 沒有被")
                print("採用 —— k_dim 仍然是 K_MAX。確認這顆 bitstream 有含請求標頭。")
                return 1
        # ctx 移除之後,「哪一半錯」這個診斷不存在了。剩下的線索是規模:
        if np.array_equal(got_c * 2, exp_c) or np.array_equal(got_c, exp_c * 2):
            print("DIAGNOSIS: 差一個 2 倍。過去這代表兩個 context 的其中一片被")
            print("當成全部;現在只有一片,比較可能是主機端仍在做跨 context 相加,")
            print("或是 bitstream 還是舊的(會回 8*N*N bytes)。")
        else:
            print("整片都錯的話,先懷疑 RX framing 或標頭長度;")
            print("只有某幾格錯的話,先看那幾格的 (row, col) 有沒有規律 ——")
            print("同一列 / 同一行出錯,通常是 skew 或 feeder 的位址。")
        return 1

    print(f"\nmax |error| : {np.abs(got_c - exp_c).max()}")
    print(f"\nPASS: K_MAX={kmax}, k_dim={k} end-to-end bit-exact on hardware.")
    return 0


if __name__ == "__main__":
    sys.exit(main())