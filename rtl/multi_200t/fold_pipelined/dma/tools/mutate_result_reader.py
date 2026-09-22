#!/usr/bin/env python3
"""
mutate_result_reader.py -- mutation test for dma_result_reader.sv.

A bench that passes proves nothing on its own: it has to fail when the design
is wrong.  This injects one defect at a time and requires
tb_dma_result_reader to catch each.  The bench instantiates the real
systolic_tx_source as its golden model, so these mutants are checked against
the serial path's actual byte order rather than a copy of it.

    cd rtl/multi_200t/fold_pipelined/dma
    python3 tools/mutate_result_reader.py

Requires iverilog on PATH.  Leaves nothing behind but temporary files.
"""

import io
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
DMA  = os.path.dirname(HERE)                      # .../dma
DUT  = os.path.join(DMA, "dma_result_reader.sv")
TB   = os.path.join(DMA, "tb", "tb_dma_result_reader.sv")
TX   = os.path.join(os.path.dirname(DMA), "core", "tx_source.sv")

GEOMETRIES = ("N=4/8/16 in one run",)   # the bench sweeps N itself

# (name, exact text to replace, replacement).  The anchors must match exactly
# once; if the design is edited so that one no longer matches, this aborts
# rather than silently testing fewer mutants.
MUTANTS = [
    ("row and col transposed",
     "      wr_data[32*j +: 32] = C[w[WORD_W-1 -: LANE_W]][w[LANE_W-1:0]];",
     "      wr_data[32*j +: 32] = C[w[LANE_W-1:0]][w[WORD_W-1 -: LANE_W]];"),

    ("word order reversed within a beat",
     "      w = {bidx, J_W'(j)};",
     "      w = {bidx, J_W'(WORDS_PER_BEAT-1-j)};"),

    ("wr_en ignores wfull",
     "  assign wr_en = (rd == R_RUN) && !wfull;",
     "  assign wr_en = (rd == R_RUN);"),

    ("last beat not sent",
     "            if (bidx == BIDX_W'(TOTAL_BEATS - 1)) begin",
     "            if (bidx == BIDX_W'(TOTAL_BEATS - 2)) begin"),

    ("bidx never advances",
     "              bidx <= bidx + 1'b1;",
     "              bidx <= bidx;"),

    ("done never pulses",
     "          done <= 1'b1;\n          rd   <= R_IDLE;",
     "          rd   <= R_IDLE;"),
]


def run_once(src_text, workdir):
    mut = os.path.join(workdir, "mut.sv")
    exe = os.path.join(workdir, "mut.out")
    io.open(mut, "w", encoding="utf-8").write(src_text)
    c = subprocess.run(["iverilog", "-g2012", "-o", exe, TB, mut, TX],
                       capture_output=True, text=True)
    if c.returncode:
        # Surfacing the compiler's own message here matters: a silent
        # COMPILE-FAIL on the baseline looks identical to a broken script,
        # and the difference is usually one unsupported construct.
        print("--- iverilog failed ---")
        print((c.stderr or c.stdout).strip()[:4000])
        print("-----------------------")
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
        base = run_once(src, workdir)
        if base != "PASS":
            print("baseline does not pass:", base)
            return 1

        head = "mutant".ljust(38) + "result".ljust(15)
        print(head + "verdict")
        print("-" * (len(head) + 8))

        killed = 0
        for name, old, new in MUTANTS:
            n = src.count(old)
            if n != 1:
                print(f"ABORT: anchor matches {n} times: {name}")
                return 2
            r = run_once(src.replace(old, new), workdir)
            dead = (r != "PASS")
            killed += dead
            print(name.ljust(38) + r.ljust(15)
                  + ("killed" if dead else "*** SURVIVED ***"))

        print(f"\n{killed}/{len(MUTANTS)} killed")
        return 0 if killed == len(MUTANTS) else 3


if __name__ == "__main__":
    sys.exit(main())
