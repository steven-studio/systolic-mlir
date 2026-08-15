#!/usr/bin/env python3
"""
patch_host_framing.py -- 讓 test_uart_kmax.py 送出帶 START / END 標記的 frame。

    python3 patch_host_framing.py test_uart_kmax.py

必須與 patch_rx_framing.py 成對使用。RTL 改完之後,不帶標記的請求會讓
硬體永遠停在 RX_HUNT,一個 byte 都不回 -- 症狀是讀取逾時而不是 MISMATCH。

線上格式
--------
    FRAME_START(4) | HDR(4) | PAYLOAD(K_MAX*64) | FRAME_END(4)

標記以小端序寫入,與 header 中 k_dim 的慣例相同,也與 RTL 裡
sync_next = {rx_byte, sync_sr[31:8]} 的位元組順序相符:先收到的 byte
落在字組的低位。

冪等。原檔備份到 <file>.bak_hostframing。
"""

import sys
import os
import shutil

SITES = [
    (
        "常數",
        "HDR_BYTES = 4\n",
        '''HDR_BYTES = 4

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
''',
    ),
    (
        "request 長度",
        "    req_bytes = HDR_BYTES + rx_bytes\n",
        "    req_bytes = MARK_BYTES + HDR_BYTES + rx_bytes + MARK_BYTES\n",
    ),
    (
        "組 frame",
        "    request = build_request(k, A, B, kmax)\n"
        "    assert len(request) == req_bytes, (len(request), req_bytes)\n",
        "    request = FRAME_START + build_request(k, A, B, kmax) + FRAME_END\n"
        "    assert len(request) == req_bytes, (len(request), req_bytes)\n",
    ),
    (
        "request 那行的輸出",
        '    print(f"request    : {req_bytes} bytes  '
        '({HDR_BYTES} header + {rx_bytes} payload)")\n',
        '    print(\n'
        '        f"request    : {req_bytes} bytes  "\n'
        '        f"({MARK_BYTES} start + {HDR_BYTES} header + "\n'
        '        f"{rx_bytes} payload + {MARK_BYTES} end)"\n'
        '    )\n',
    ),
    (
        "docstring",
        "Request, HDR_BYTES + K_MAX*64 bytes:\n",
        "Request, MARK_BYTES + HDR_BYTES + K_MAX*64 + MARK_BYTES bytes:\n",
    ),
]


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2

    path = sys.argv[1]
    if not os.path.isfile(path):
        print(f"ERROR: 找不到 {path}")
        return 1

    src = open(path, encoding="utf-8").read()

    if "FRAME_START" in src:
        print("已經套用過(檔案裡已有 FRAME_START),不做任何事。")
        return 0

    problems = []
    for name, old, _ in SITES:
        n = src.count(old)
        if n != 1:
            problems.append(f"  {name}: 出現 {n} 次(需要剛好 1 次)")

    if problems:
        print("ERROR: 錨點對不上,沒有做任何修改。")
        print("\n".join(problems))
        print()
        print("把這幾行貼給我:")
        print(f"  grep -n 'HDR_BYTES = 4\\|req_bytes =\\|build_request(k' {path}")
        return 1

    for _, old, new in SITES:
        src = src.replace(old, new, 1)

    # 送出前先清掉輸入緩衝,避免讀到上一次殘留的位元組
    import ast
    try:
        ast.parse(src)
    except SyntaxError as e:
        print(f"ERROR: 修改後語法不合法({e}),沒有寫入。")
        return 1

    bak = path + ".bak_hostframing"
    if not os.path.exists(bak):
        shutil.copy2(path, bak)

    open(path, "w", encoding="utf-8").write(src)

    print(f"已修改 {path}")
    print(f"備份    {bak}")
    print()
    for name, _, _ in SITES:
        print(f"  ok  {name}")
    print("  ok  ast.parse 通過")
    print()
    print("K_MAX=16 的 request 從 1028 變成 1036 bytes")
    print("  4 start + 4 header + 1024 payload + 4 end")
    return 0


if __name__ == "__main__":
    sys.exit(main())
