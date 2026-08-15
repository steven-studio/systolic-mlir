#!/usr/bin/env python3
"""為 test_uart_kmax.py 加上 CYCLE_COUNTER 的 4-byte 讀取。冪等。"""
import ast, sys

p = sys.argv[1] if len(sys.argv) > 1 else "test_uart_kmax.py"
s = open(p).read()

if "hardware cycles" in s:
    print("已經改過了，不重複套用"); sys.exit(0)

A = '''                print(f"\\rRX {len(rx)}/{TX_BYTES}", end="", flush=True)
        print()
'''
B = '''                print(f"\\rRX {len(rx)}/{TX_BYTES}", end="", flush=True)
        print()

        # CYCLE_COUNTER=1 的 bitstream 會在 512 bytes 之後再送 4 bytes
        # （小端序）。用短 timeout 試讀:沒有就是舊 bitstream，其餘檢查
        # 完全不受影響。必須在 with 區塊內讀，離開後 ser 就關了。
        _saved_to = ser.timeout
        ser.timeout = 0.5
        cyc_raw = ser.read(4)
        ser.timeout = _saved_to
'''

C = '''    if len(rx) != TX_BYTES:
        print(f"FAIL: expected {TX_BYTES} bytes, got {len(rx)}")
'''
D = '''    if len(cyc_raw) == 4:
        cyc = int.from_bytes(cyc_raw, "little")
        exp = k + 118
        print(f"hardware cycles : {cyc}   (xsim 預期 k_dim + 118 = {exp})")
        if cyc == exp:
            print("                  與模擬完全一致")
        else:
            print(f"                  差 {cyc - exp:+d} 拍")
    else:
        print("hardware cycles : 未回報（此 bitstream 的 CYCLE_COUNTER 為 0）")

    if len(rx) != TX_BYTES:
        print(f"FAIL: expected {TX_BYTES} bytes, got {len(rx)}")
'''

for name, a in (("A", A), ("C", C)):
    if a not in s:
        print(f"✗ anchor {name} 沒對上，檔案可能已被改動"); sys.exit(1)

s = s.replace(A, B, 1).replace(C, D, 1)
ast.parse(s)
open(p, "w").write(s)
print("✓ 已套用，語法檢查通過")
