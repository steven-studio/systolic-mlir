#!/usr/bin/env python3
"""
mutate_writeback.py -- mutation test for dma_writeback_engine.sv.

A bench that passes proves nothing on its own: it has to fail when the design
is wrong.  This injects one defect at a time and requires the bench to catch
each of them, at BOTH burst lengths -- a mutant killed at only one length is
still killed, but the table shows WHICH length was doing the work, and that
is the part worth reading.

    cd rtl/multi_200t/fold_pipelined/dma
    python3 tools/mutate_writeback.py

Requires iverilog on PATH.  Leaves nothing behind but /tmp files.
"""

import io
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
DMA  = os.path.dirname(HERE)                      # .../dma
DUT  = os.path.join(DMA, "dma_writeback_engine.sv")
TB   = os.path.join(DMA, "tb", "tb_dma_writeback_engine.sv")

BURST_LENS = (16, 2)

# (name, exact text to replace, replacement).  The anchors must match exactly
# once; if the design is edited so that one no longer matches, this aborts
# rather than silently testing fewer mutants.
MUTANTS = [
    ("wlast one beat early",
     "    m_axi_wlast   = (w_left == LEN_W'(1));",
     "    m_axi_wlast   = (w_left == LEN_W'(2));"),

    ("4 KiB clamp removed",
     "      if (to_page   < lim) lim = to_page;",
     "      // clamp removed"),

    ("bresp unchecked",
     "      if (b_fire && !((m_axi_bresp == RESP_OKAY) || (m_axi_bresp == RESP_EXOK)))\n"
     "        err_resp <= 1'b1;",
     "      // bresp unchecked"),

    ("alignment unchecked",
     "            if (desc_addr[LSB-1:0] != '0) begin\n"
     "              err_align <= 1'b1;\n"
     "            end else begin",
     "            if (1'b0) begin\n"
     "              err_align <= 1'b1;\n"
     "            end else begin"),

    ("credit same-cycle cancel dropped",
     "      if (aw_fire && !b_fire)      credit <= credit - 1'b1;\n"
     "      else if (!aw_fire && b_fire) credit <= credit + 1'b1;",
     "      if (aw_fire)     credit <= credit - 1'b1;\n"
     "      else if (b_fire) credit <= credit + 1'b1;"),

    ("src_ready ignores wready",
     "  assign src_ready  = (state == S_W) && m_axi_wready;",
     "  assign src_ready  = (state == S_W);"),

    ("done before the last B",
     "          if (credit == CRED_W'(MAX_OUTSTANDING)) begin",
     "          if (1'b1) begin"),

    ("burst length not reloaded",
     "                cur_len <= LEN_W'(len_next);\n"
     "                awlen_r <= 8'(len_next - 32'd1);",
     "                // length not reloaded"),
]


def run_once(src_text, burst_len, workdir):
    mut = os.path.join(workdir, "mut.sv")
    exe = os.path.join(workdir, f"mut{burst_len}.out")
    io.open(mut, "w", encoding="utf-8").write(src_text)
    c = subprocess.run(
        ["iverilog", "-g2012", "-o", exe,
         f"-Ptb_dma_writeback_engine.BURST_LEN={burst_len}", TB, mut],
        capture_output=True, text=True)
    if c.returncode:
        return "COMPILE-FAIL"
    try:
        r = subprocess.run([("vvp"), exe], capture_output=True, text=True, timeout=300)
    except subprocess.TimeoutExpired:
        return "TIMEOUT"
    return "PASS" if "PASS: 0 error" in r.stdout else "FAIL"


def main():
    src = io.open(DUT, encoding="utf-8").read()

    with tempfile.TemporaryDirectory() as workdir:
        # The unmutated design must pass first, or every "kill" below is
        # meaningless.
        base = [run_once(src, bl, workdir) for bl in BURST_LENS]
        if any(b != "PASS" for b in base):
            print("baseline does not pass:", dict(zip(BURST_LENS, base)))
            return 1

        head = "mutant".ljust(34) + "".join(f"BL={bl}".ljust(9) for bl in BURST_LENS)
        print(head + "verdict")
        print("-" * (len(head) + 8))

        killed = 0
        for name, old, new in MUTANTS:
            n = src.count(old)
            if n != 1:
                print(f"ABORT: anchor matches {n} times: {name}")
                return 2
            results = [run_once(src.replace(old, new), bl, workdir)
                       for bl in BURST_LENS]
            dead = any(r != "PASS" for r in results)
            killed += dead
            print(name.ljust(34)
                  + "".join(r.ljust(9) for r in results)
                  + ("killed" if dead else "*** SURVIVED ***"))

        print(f"\n{killed}/{len(MUTANTS)} killed")
        return 0 if killed == len(MUTANTS) else 3


if __name__ == "__main__":
    sys.exit(main())
